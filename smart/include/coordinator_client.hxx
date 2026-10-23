#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "coordinator.grpc.pb.h"
#include "coordinator.pb.h"

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
    CoordinatorClient();
    ~CoordinatorClient();

    // Async provideGlobalRoutingInfo against the given coordinator target ("host:port").
    // Returns safe copy of data (not owned by gRPC).
    std::future<std::shared_ptr<CoordinatorRoutingInfo>> provideGlobalRoutingInfo(
        const std::string& target,
        std::chrono::milliseconds timeout);

    // Currently connected coordinator target (empty if none).
    [[nodiscard]] std::string getActiveTarget() const;

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

    void RunCompletionQueue();
    std::shared_ptr<coordinator::CoordinatorService::Stub> ensureStub(const std::string& target);

    mutable std::mutex channel_mutex_;
    std::string active_target_;
    std::shared_ptr<grpc::Channel> channel_;
    std::shared_ptr<coordinator::CoordinatorService::Stub> stub_;
    std::atomic<bool> shutdown_{false};
    grpc::CompletionQueue cq_;
    std::jthread cq_thread_;
};
