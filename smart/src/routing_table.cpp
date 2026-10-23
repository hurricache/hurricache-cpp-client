#include <routing_table.hxx>
#include <iostream>
#include <grpcpp/create_channel.h>

void RoutingTable::updateFromCoordinator(
    int32_t max_shards,
    const std::vector<coordinator::PeerRouting>& peer_routings,
    const ChannelConfig& channel_config,
    std::chrono::milliseconds timeout,
    int32_t compression_threshold) {

    std::vector<SharedClient> removed_clients;
    {
        std::unique_lock lock(mutex_);

        max_shards_ = max_shards;

        // Collect new targets
        std::set<std::string> new_targets;
        for (const auto& peer : peer_routings) {
            new_targets.insert(peer.target());
        }

        // Create new clients for new targets
        for (const auto& target : new_targets) {
            if (routing_table_target_.find(target) == routing_table_target_.end()) {
                auto client = createClient(target, channel_config, timeout, compression_threshold);
                routing_table_target_[target] = client;
                std::cout << "[ROUTING] Created client for target: " << target << std::endl;
            }
        }

        // Remove old clients
        for (auto it = routing_table_target_.begin(); it != routing_table_target_.end();) {
            if (new_targets.find(it->first) == new_targets.end()) {
                std::cout << "[ROUTING] Removing client for target: " << it->first << std::endl;
                removed_clients.push_back(it->second);
                it = routing_table_target_.erase(it);
            } else {
                ++it;
            }
        }

        // Build routing table from peer_routings
        routing_table_.clear();
        for (const auto& peer : peer_routings) {
            auto client_it = routing_table_target_.find(peer.target());
            if (client_it == routing_table_target_.end()) continue;

            for (auto partition_id : peer.partitionids()) {
                auto key = std::make_pair(peer.role(), partition_id);
                routing_table_[key] = client_it->second;
            }
        }

        std::cout << "[ROUTING] Updated: max_shards=" << max_shards_
                  << ", targets=" << routing_table_target_.size()
                  << ", routes=" << routing_table_.size() << std::endl;
    }

    // Shutdown removed clients outside the lock
    for (const auto& client : removed_clients) {
        if (client) client->shutdown();
    }
}

RoutingTable::SharedClient RoutingTable::getRoute(int shard, coordinator::NodeRole role) const {
    std::shared_lock lock(mutex_);
    auto key = std::make_pair(role, shard);
    auto it = routing_table_.find(key);
    return (it != routing_table_.end()) ? it->second : nullptr;
}

RoutingTable::SharedClient RoutingTable::getRouteByTarget(const std::string& target) const {
    std::shared_lock lock(mutex_);
    auto it = routing_table_target_.find(target);
    return (it != routing_table_target_.end()) ? it->second : nullptr;
}

RoutingTable::SharedClient RoutingTable::getOrCreateByTarget(
    const std::string& target,
    const ChannelConfig& channel_config,
    std::chrono::milliseconds timeout,
    int32_t compression_threshold) {

    {
        std::shared_lock lock(mutex_);
        auto it = routing_table_target_.find(target);
        if (it != routing_table_target_.end()) {
            return it->second;
        }
    }
    std::unique_lock lock(mutex_);
    auto it = routing_table_target_.find(target);
    if (it != routing_table_target_.end()) {
        return it->second;
    }
    auto client = createClient(target, channel_config, timeout, compression_threshold);
    routing_table_target_[target] = client;
    std::cout << "[ROUTING] Reroute: created client for target: " << target << std::endl;
    return client;
}

void RoutingTable::shutdown() {
    std::vector<SharedClient> clients;
    {
        std::unique_lock lock(mutex_);
        routing_table_.clear();
        for (auto& [_, client] : routing_table_target_) {
            clients.push_back(std::move(client));
        }
        routing_table_target_.clear();
    }
    for (const auto& client : clients) {
        if (client) client->shutdown();
    }
}

RoutingTable::SharedClient RoutingTable::createClient(
    const std::string& target,
    const ChannelConfig& config,
    std::chrono::milliseconds timeout,
    int32_t compression_threshold) {

    return createSimpleClient(target, config, timeout, compression_threshold);
}
