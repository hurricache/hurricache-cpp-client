#pragma once

#include <future>
#include <string>
#include <chrono>
#include <memory>
#include <vector>
#include <cstdint>
#include "coordinator.grpc.pb.h"
#include "coordinator.pb.h"

// Forward declaration of NodeRole enum
namespace coordinator {
    enum NodeRole : int;
}

// Safe structure to hold routing info (not owned by gRPC)
struct CoordinatorRoutingInfo {
    int32_t max_shards = 0;
    struct PeerInfo {
        std::string target;
        coordinator::NodeRole role;
        std::vector<uint32_t> partition_ids;
    };
    std::vector<PeerInfo> peer_routings;
};

class CoordinatorClient {
public:
    CoordinatorClient() = default;
    ~CoordinatorClient();

    // Async provideGlobalRoutingInfo using SendAsyncStreamRequest pattern
    // Returns safe copy of data (not owned by gRPC)
    std::future<std::shared_ptr<CoordinatorRoutingInfo>> provideGlobalRoutingInfo(
        const std::string& host,
        int32_t port,
        std::chrono::milliseconds timeout);

    void shutdown();

private:
    // Call data for async stream operations
    struct RoutingStreamCallData {
        std::promise<std::shared_ptr<CoordinatorRoutingInfo>> promise;
        grpc::ClientContext context;
        grpc::Status status;
        std::unique_ptr<grpc::ClientAsyncReaderInterface<coordinator::RoutingInfoData>> reader;
        
        coordinator::RoutingInfoData current_chunk;
        std::shared_ptr<CoordinatorRoutingInfo> accumulated_result;
        
        enum class State { STARTED, READING, FINISHING };
        State state = State::STARTED;
        
        void Proceed(bool ok);
    };

    std::shared_ptr<coordinator::CoordinatorService::Stub> stub_;
    std::shared_ptr<grpc::Channel> channel_;
    std::atomic<bool> shutdown_{false};
    grpc::CompletionQueue cq_;
};
