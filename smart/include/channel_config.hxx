#pragma once

#include <memory>
#include <chrono>
#include <string>
#include "standalone_client.hxx"

// Configuration for gRPC channel used by FastCacheStandaloneClient
struct ChannelConfig {
    int32_t flow_control_window = 4 * 1024 * 1024;      // 4MB
    int32_t max_inbound_message_size = 64 * 1024 * 1024; // 64MB
};

// Factory function to create SimpleClient with custom channel config
std::shared_ptr<FastCacheStandaloneClient> createSimpleClient(
    const std::string& target,
    const ChannelConfig& config,
    std::chrono::milliseconds timeout,
    int32_t defaultCompressionThreshold);
