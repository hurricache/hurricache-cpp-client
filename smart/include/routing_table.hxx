#pragma once

#include <memory>
#include <map>
#include <shared_mutex>
#include <string>
#include <vector>
#include <set>
#include <chrono>
#include "standalone_client.hxx"
#include "coordinator.pb.h"
#include "channel_config.hxx"

class RoutingTable {
public:
    using SharedClient = std::shared_ptr<FastCacheStandaloneClient>;

    RoutingTable() : max_shards_(1024) {}

    // Обновление из RoutingInfoData
    void updateFromCoordinator(
        int32_t max_shards,
        const std::vector<coordinator::PeerRouting>& peer_routings,
        const ChannelConfig& channel_config,
        std::chrono::milliseconds timeout,
        int32_t compression_threshold);

    // Получение клиента для shard + role
    SharedClient getRoute(int shard, coordinator::NodeRole role) const;

    // Получение клиента по target
    SharedClient getRouteByTarget(const std::string& target) const;

    // Reroute-кейс: вернуть существующий клиент или создать новый для target
    // (аналог routingTableTarget.computeIfAbsent в Java)
    SharedClient getOrCreateByTarget(const std::string& target,
                                     const ChannelConfig& channel_config,
                                     std::chrono::milliseconds timeout,
                                     int32_t compression_threshold);

    void shutdown();

    [[nodiscard]] int32_t max_shards() const {
        std::shared_lock lock(mutex_);
        return max_shards_;
    }

private:
    mutable std::shared_mutex mutex_;
    int32_t max_shards_;
    std::map<std::pair<coordinator::NodeRole, int32_t>, SharedClient> routing_table_;
    std::map<std::string, SharedClient> routing_table_target_;

    SharedClient createClient(const std::string& target,
                             const ChannelConfig& config,
                             std::chrono::milliseconds timeout,
                             int32_t compression_threshold);
};
