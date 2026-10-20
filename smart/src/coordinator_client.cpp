#include <coordinator_client.hxx>
#include <grpcpp/create_channel.h>
#include <thread>

CoordinatorClient::~CoordinatorClient() {
    shutdown();
}

void CoordinatorClient::shutdown() {
    bool expected = false;
    if (!shutdown_.compare_exchange_strong(expected, true)) {
        return;
    }
    channel_.reset();
    stub_.reset();
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
            std::string err_msg = "Coordinator stream failed. Code: " + std::to_string(status.error_code()) +
                                  ", Message: " + status.error_message();
            promise.set_exception(std::make_exception_ptr(std::runtime_error(err_msg)));
        }
        delete this;
    }
}

std::future<std::shared_ptr<CoordinatorRoutingInfo>> CoordinatorClient::provideGlobalRoutingInfo(
    const std::string& host,
    int32_t port,
    std::chrono::milliseconds timeout) {
    
    std::string target = host + ":" + std::to_string(port);
    channel_ = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    stub_ = coordinator::CoordinatorService::NewStub(channel_);
    
    auto *call = new RoutingStreamCallData();
    call->accumulated_result = nullptr;
    
    std::chrono::milliseconds effectiveTimeout = timeout;
    if (effectiveTimeout.count() > 0) {
        call->context.set_deadline(std::chrono::system_clock::now() + effectiveTimeout);
    }
    
    coordinator::Void request;
    call->reader = stub_->PrepareAsyncprovideGlobalRoutingInfo(&call->context, request, &cq_);
    
    call->reader->StartCall(call);
    
    return call->promise.get_future();
}
