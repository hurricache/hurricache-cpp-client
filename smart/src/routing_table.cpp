#include <routing_table.hxx>
#include <iostream>

void RoutingTable::updateFromCoordinator(
    int32_t max_shards,
    const std::vector<coordinator::PeerRouting>& peer_routings,
    const ChannelConfig& channel_config,
    std::chrono::milliseconds timeout,
    int32_t compression_threshold) {
    
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
    std::set<std::string> old_targets;
    for (const auto& [target, _] : routing_table_target_) {
        if (new_targets.find(target) == new_targets.end()) {
            old_targets.insert(target);
            std::cout << "[ROUTING] Removing client for target: " << target << std::endl;
        }
    }
    for (const auto& target : old_targets) {
        routing_table_target_.erase(target);
    }
    
    // Build routing table from peer_routings
    routing_table_.clear();
    for (const auto& peer : peer_routings) {
        auto client_it = routing_table_target_.find(peer.target());
        if (client_it == routing_table_target_.end()) continue;
        
        for (auto partition_id : peer.partition_ids()) {
            auto key = std::make_pair(peer.role(), partition_id);
            routing_table_[key] = client_it->second;
        }
    }
    
    std::cout << "[ROUTING] Updated: max_shards=" << max_shards_
              << ", targets=" << routing_table_target_.size()
              << ", routes=" << routing_table_.size() << std::endl;
}

RoutingTable::SharedClient RoutingTable::getRoute(int shard, coordinator::NodeRole role) const {
    auto key = std::make_pair(role, shard);
    auto it = routing_table_.find(key);
    return (it != routing_table_.end()) ? it->second : nullptr;
}

RoutingTable::SharedClient RoutingTable::getRouteByTarget(const std::string& target) const {
    auto it = routing_table_target_.find(target);
    return (it != routing_table_target_.end()) ? it->second : nullptr;
}

void RoutingTable::shutdown() {
    routing_table_.clear();
    routing_table_target_.clear();
}

RoutingTable::SharedClient RoutingTable::createClient(
    const std::string& target,
    const ChannelConfig& config,
    std::chrono::milliseconds timeout,
    int32_t compression_threshold) {
    
    grpc::ChannelArguments args;
    args.SetMaxReceiveMessageSize(config.max_inbound_message_size);
    args.SetFlowControlWindow(config.flow_control_window);
    
    auto channel = grpc::CreateCustomChannel(target, grpc::InsecureChannelCredentials(), args);
    return std::make_shared<FastCacheStandaloneClient>(
        channel, target, timeout, compression_threshold);
}
