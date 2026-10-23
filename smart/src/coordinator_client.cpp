#include <coordinator_client.hxx>
#include <grpcpp/create_channel.h>
#include "utils.hxx"

CoordinatorClient::CoordinatorClient() {
    cq_thread_ = std::jthread([this] { RunCompletionQueue(); });
}

CoordinatorClient::~CoordinatorClient() {
    shutdown();
}

void CoordinatorClient::shutdown() {
    bool expected = false;
    if (!shutdown_.compare_exchange_strong(expected, true)) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(channel_mutex_);
        stub_.reset();
        channel_.reset();
        active_target_.clear();
    }
    cq_.Shutdown();
    if (cq_thread_.joinable()) {
        cq_thread_.join();
    }
}

std::string CoordinatorClient::getActiveTarget() const {
    std::lock_guard<std::mutex> lock(channel_mutex_);
    return active_target_;
}

void CoordinatorClient::RunCompletionQueue() {
    void *tag = nullptr;
    bool ok = false;
    while (cq_.Next(&tag, &ok)) {
        auto *call_data = static_cast<RoutingStreamCallData *>(tag);
        call_data->Proceed(ok);
    }
}

std::shared_ptr<coordinator::CoordinatorService::Stub> CoordinatorClient::ensureStub(const std::string& target) {
    std::lock_guard<std::mutex> lock(channel_mutex_);
    if (stub_ && active_target_ == target) {
        return stub_;
    }
    stub_.reset();
    channel_.reset();
    channel_ = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    stub_ = coordinator::CoordinatorService::NewStub(channel_);
    active_target_ = target;
    return stub_;
}

void CoordinatorClient::RoutingStreamCallData::Proceed(bool ok) {
    if (state == State::STARTED) {
        // This fires after StartCall(tag)
        if (ok) {
            state = State::READING;
            reader->Read(&current_chunk, this);
        } else {
            state = State::FINISHING;
            reader->Finish(&status, this);
        }
    } else if (state == State::READING) {
        if (ok) {
            // Accumulate data from chunk into safe structure
            if (!accumulated_result) {
                accumulated_result = std::make_shared<CoordinatorRoutingInfo>();
            }
            if (accumulated_result->max_shards == 0) {
                accumulated_result->max_shards = current_chunk.max_shards();
            }
            for (const auto& peer : current_chunk.peerrouting()) {
                CoordinatorRoutingInfo::PeerInfo info;
                info.target = peer.target();
                info.role = peer.role();
                for (const auto& partition_id : peer.partitionids()) {
                    info.partition_ids.push_back(partition_id);
                }
                accumulated_result->peer_routings.push_back(std::move(info));
            }
            // Request next chunk
            reader->Read(&current_chunk, this);
        } else {
            // Server finished sending data (ok == false for Read means EOF)
            state = State::FINISHING;
            reader->Finish(&status, this);
        }
    } else if (state == State::FINISHING) {
        if (status.ok()) {
            if (!accumulated_result) {
                accumulated_result = std::make_shared<CoordinatorRoutingInfo>();
            }
            promise.set_value(accumulated_result);
        } else {
            promise.set_exception(std::make_exception_ptr(
                GrpcStatusException(status.error_code(), status.error_message())));
        }
        delete this;
    }
}

std::future<std::shared_ptr<CoordinatorRoutingInfo>> CoordinatorClient::provideGlobalRoutingInfo(
    const std::string& target,
    std::chrono::milliseconds timeout) {

    if (shutdown_.load()) {
        std::promise<std::shared_ptr<CoordinatorRoutingInfo>> p;
        p.set_exception(std::make_exception_ptr(
            std::runtime_error("CoordinatorClient is shut down")));
        return p.get_future();
    }

    auto stub = ensureStub(target);

    auto *call = new RoutingStreamCallData();
    call->accumulated_result = nullptr;
    std::future<std::shared_ptr<CoordinatorRoutingInfo>> future = call->promise.get_future();

    if (timeout.count() > 0) {
        call->context.set_deadline(std::chrono::system_clock::now() + timeout);
    }

    coordinator::Void request;
    call->reader = stub->PrepareAsyncprovideGlobalRoutingInfo(&call->context, request, &cq_);
    call->reader->StartCall(call);

    return future;
}
