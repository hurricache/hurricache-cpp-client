#include <standalone_client.hxx>
#include "utils.hxx"
#include "compression.hxx"
#include "grpcpp/security/credentials.h"

// =========================================================================
// CONSTRUCTORS
// =========================================================================

FastCacheStandaloneClient::FastCacheStandaloneClient(
    const std::string& host, int32_t port, int32_t defaultClientId,
    std::chrono::milliseconds timeout, int32_t defaultCompressionThreshold)
    : defaultClientId_(defaultClientId),
      defaultTimeout_(timeout),
      defaultCompressionThreshold_(defaultCompressionThreshold),
      target_(host + ":" + std::to_string(port))
{
    std::cerr << "[DEBUG] Creating channel to " << target_ << std::endl;
    grpc::ChannelArguments args;
    channel_ = grpc::CreateCustomChannel(target_, grpc::InsecureChannelCredentials(), args);
    std::cerr << "[DEBUG] Channel created, creating stub" << std::endl;
    asyncStub_ = hurricache::HurriCacheGrpcService::NewStub(channel_);
    std::cerr << "[DEBUG] Stub created, starting completion queue thread" << std::endl;
    // Start completion queue thread - it will block on cq_.Next() until Shutdown() is called
    completion_queue_thread_ = std::jthread([this] {
        std::cerr << "[DEBUG] Completion queue thread started" << std::endl;
        RunCompletionQueue();
        std::cerr << "[DEBUG] Completion queue thread exiting" << std::endl;
    });
    std::cerr << "[DEBUG] Completion queue thread started successfully" << std::endl;
}

FastCacheStandaloneClient::FastCacheStandaloneClient(
    const std::string& host, int32_t port, int32_t defaultClientId, std::chrono::milliseconds timeout)
    : FastCacheStandaloneClient(host, port, defaultClientId, timeout, kDefaultCompressionThreshold) {}

FastCacheStandaloneClient::FastCacheStandaloneClient(const std::string& host, int32_t port, int32_t clientId)
    : FastCacheStandaloneClient(host, port, clientId, std::chrono::seconds(1), kDefaultCompressionThreshold) {}

FastCacheStandaloneClient::FastCacheStandaloneClient(const std::string& host, int32_t port)
    : FastCacheStandaloneClient(host, port, 0, std::chrono::seconds(1), kDefaultCompressionThreshold) {}

FastCacheStandaloneClient::FastCacheStandaloneClient(const std::string& host, int32_t port, std::chrono::milliseconds duration)
    : FastCacheStandaloneClient(host, port, 0, duration, kDefaultCompressionThreshold) {}

FastCacheStandaloneClient::FastCacheStandaloneClient(
    std::shared_ptr<grpc::Channel> channel, std::string target, int32_t defaultClientId,
    std::chrono::milliseconds duration, int32_t defaultCompressionThreshold)
    : channel_(std::move(channel)),
      defaultClientId_(defaultClientId),
      defaultTimeout_(duration),
      defaultCompressionThreshold_(defaultCompressionThreshold),
      target_(std::move(target))
{
    asyncStub_ = hurricache::HurriCacheGrpcService::NewStub(channel_);
    completion_queue_thread_ = std::jthread([this] { RunCompletionQueue(); });
}

// =========================================================================
// GETTERS
// =========================================================================

std::string FastCacheStandaloneClient::toString() const {
    return "FastCacheStandaloneClient{target='" + target_ + "'}";
}

std::string FastCacheStandaloneClient::getTarget() const { return target_; }
int32_t FastCacheStandaloneClient::getDefaultClientId() const { return defaultClientId_; }
std::chrono::milliseconds FastCacheStandaloneClient::getDefaultTimeout() const { return defaultTimeout_; }
int32_t FastCacheStandaloneClient::getDefaultCompressionThreshold() const { return defaultCompressionThreshold_; }

// =========================================================================
// LIFECYCLE
// =========================================================================

void FastCacheStandaloneClient::shutdown() {
    cq_.Shutdown();
    if (completion_queue_thread_.joinable()) {
        completion_queue_thread_.join();
    }
    asyncStub_.reset();
    channel_.reset();
}

void FastCacheStandaloneClient::RunCompletionQueue() {
    void* tag = nullptr;
    bool ok = false;
    while (cq_.Next(&tag, &ok)) {
        auto* call_data = static_cast<RpcCallDataBase*>(tag);
        call_data->Proceed(ok);
    }
}

// =========================================================================
// TTL MANAGEMENT
// =========================================================================

std::future<bool> FastCacheStandaloneClient::setTtl(const Key& key, const KeyHint& hint, int64_t ttl, int32_t clientId,
                                                     std::chrono::milliseconds timeout) {
    hurricache::TtlRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    request.set_ttl(static_cast<uint64_t>(now_ms) + static_cast<uint64_t>(ttl));

    return SendAsyncRequest<hurricache::TtlRequest, hurricache::BoolResponse, bool>(
        request, timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncsetTtl,
        [](hurricache::BoolResponse& resp) -> bool { return resp.value(); });
}

std::future<int64_t> FastCacheStandaloneClient::getTtl(const Key& key, const KeyHint& hint, int32_t clientId,
                                                        std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::TtlResponse, int64_t>(
        buildGetRequestProto(key, hint, clientId), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetTtl,
        [](hurricache::TtlResponse& resp) -> int64_t {
            if (resp.has_ttl()) {
                auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                int64_t remaining = static_cast<int64_t>(resp.ttl()) - static_cast<int64_t>(now_ms);
                return remaining > 0 ? remaining : -1;
            }
            return -1;
        });
}

// =========================================================================
// KEY-VALUE OPERATIONS
// =========================================================================

std::future<Value> FastCacheStandaloneClient::getAndDeleteValue(const Key& key, const KeyHint& hint, int32_t clientId,
                                                                 std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::ValueResponse, Value>(
        buildGetRequestProto(key, hint, clientId), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetAndDeleteValue,
        [](hurricache::ValueResponse& resp) -> Value {
            if (!resp.has_value_unordered()) return Value{0, nullptr};
            Value* v = valueRequestToValue(resp.value_unordered());
            if (!v) return Value{0, nullptr};
            Value result = *v;
            delete v;
            return result;
        });
}

std::future<KeyHint> FastCacheStandaloneClient::createKeyValue(const Key& key, const KeyHint& hint, const Value& value,
                                                                std::chrono::milliseconds ttl, int32_t clientId,
                                                                std::chrono::milliseconds timeout) {
    hurricache::CreateRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    *request.mutable_value() = buildValueProto(value, ttl, clientId);

    return SendAsyncRequest<hurricache::CreateRequest, hurricache::KeyHintResponse, KeyHint>(
        request, timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateKeyValue,
        [](hurricache::KeyHintResponse& resp) -> KeyHint {
            const auto& kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0,
                          kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<Value> FastCacheStandaloneClient::getValue(const Key& key, const KeyHint& hint, int32_t clientId,
                                                        std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::ValueResponse, Value>(
        buildGetRequestProto(key, hint, clientId), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetValue,
        [](hurricache::ValueResponse& resp) -> Value {
            if (!resp.has_value_unordered()) return Value{0, nullptr};
            Value* v = valueRequestToValue(resp.value_unordered());
            if (!v) return Value{0, nullptr};
            Value result = *v;
            delete v;
            return result;
        });
}

std::future<Value> FastCacheStandaloneClient::updateKeyValue(const Key& key, const KeyHint& hint, const Value& value,
                                                              std::chrono::milliseconds ttl, int32_t clientId,
                                                              std::chrono::milliseconds timeout) {
    hurricache::UpdateRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    *request.mutable_value() = buildValueProto(value, ttl, clientId);

    return SendAsyncRequest<hurricache::UpdateRequest, hurricache::UpdateValueResponse, Value>(
        request, timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncupdateValue,
        [](hurricache::UpdateValueResponse& resp) -> Value {
            if (!resp.has_value()) return Value{0, nullptr};
            Value* v = valueRequestToValue(resp.value());
            if (!v) return Value{0, nullptr};
            Value result = *v;
            delete v;
            return result;
        });
}

std::future<bool> FastCacheStandaloneClient::existKey(const Key& key, const KeyHint& hint, int32_t clientId,
                                                       std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::BoolResponse, bool>(
        buildGetRequestProto(key, hint, clientId), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncexistKey,
        [](hurricache::BoolResponse& resp) -> bool { return resp.value(); });
}

std::future<bool> FastCacheStandaloneClient::remove(const Key& key, const KeyHint& hint, int32_t clientId,
                                                     std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::BoolResponse, bool>(
        buildGetRequestProto(key, hint, clientId), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::Asyncremove,
        [](hurricache::BoolResponse& resp) -> bool { return resp.value(); });
}

// =========================================================================
// CONTAINER CREATION (UNORDERED & ORDERED)
// =========================================================================

static hurricache::CreateContainerRequest buildContainerRequest(
    const Key& key, const KeyHint& hint, int32_t clientId,
    hurricache::ContainerType type, std::chrono::milliseconds ttl,
    const std::vector<Value>& values) {
    hurricache::CreateContainerRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_type(type);
    
    if (ttl.count() > 0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request.set_ttl(static_cast<uint64_t>(now_ms) + static_cast<uint64_t>(ttl.count()));
    }
    
    for (const auto& v : values) {
        *request.add_value_unordered() = buildValueProtoNoTtl(v, clientId);
    }
    
    return request;
}

static hurricache::CreateContainerRequest buildContainerRequest(
    const Key& key, const KeyHint& hint, int32_t clientId,
    hurricache::ContainerType type, std::chrono::milliseconds ttl,
    const std::vector<OrderedValue>& values) {
    hurricache::CreateContainerRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_type(type);
    
    if (ttl.count() > 0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request.set_ttl(static_cast<uint64_t>(now_ms) + static_cast<uint64_t>(ttl.count()));
    }
    
    for (const auto& ov : values) {
        auto* ordered_val = request.add_value_ordered();
        ordered_val->set_order(ov.weight);
        auto* ov_val = ordered_val->mutable_value();
        ov_val->set_size(static_cast<uint32_t>(ov.size));
        ov_val->set_payload(absl::string_view(ov.data, static_cast<size_t>(ov.size)));
    }
    
    return request;
}

std::future<KeyHint> FastCacheStandaloneClient::createQueue(const Key& key, const KeyHint& keyHint,
                                                             const std::vector<Value>& initialValue,
                                                             std::chrono::milliseconds ttl, int32_t clientId,
                                                             std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::CreateContainerRequest, hurricache::KeyHintResponse, KeyHint>(
        buildContainerRequest(key, keyHint, clientId, hurricache::ContainerType::QUEUE, ttl, initialValue),
        timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateContainer,
        [](hurricache::KeyHintResponse& resp) -> KeyHint {
            const auto& kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<KeyHint> FastCacheStandaloneClient::createList(const Key& key, const KeyHint& keyHint,
                                                            const std::vector<Value>& initialValue,
                                                            std::chrono::milliseconds ttl, int32_t clientId,
                                                            std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::CreateContainerRequest, hurricache::KeyHintResponse, KeyHint>(
        buildContainerRequest(key, keyHint, clientId, hurricache::ContainerType::LIST, ttl, initialValue),
        timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateContainer,
        [](hurricache::KeyHintResponse& resp) -> KeyHint {
            const auto& kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<KeyHint> FastCacheStandaloneClient::createVector(const Key& key, const KeyHint& keyHint,
                                                              const std::vector<Value>& initialValue,
                                                              std::chrono::milliseconds ttl, int32_t clientId,
                                                              std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::CreateContainerRequest, hurricache::KeyHintResponse, KeyHint>(
        buildContainerRequest(key, keyHint, clientId, hurricache::ContainerType::VECTOR, ttl, initialValue),
        timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateContainer,
        [](hurricache::KeyHintResponse& resp) -> KeyHint {
            const auto& kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<KeyHint> FastCacheStandaloneClient::createSet(const Key& key, const KeyHint& keyHint,
                                                           const std::vector<Value>& initialValue,
                                                           std::chrono::milliseconds ttl, int32_t clientId,
                                                           std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::CreateContainerRequest, hurricache::KeyHintResponse, KeyHint>(
        buildContainerRequest(key, keyHint, clientId, hurricache::ContainerType::SET, ttl, initialValue),
        timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateContainer,
        [](hurricache::KeyHintResponse& resp) -> KeyHint {
            const auto& kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<KeyHint> FastCacheStandaloneClient::createOrderedSet(const Key& key, const KeyHint& keyHint,
                                                                  const std::vector<OrderedValue>& initialValue,
                                                                  std::chrono::milliseconds ttl, int32_t clientId,
                                                                  std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::CreateContainerRequest, hurricache::KeyHintResponse, KeyHint>(
        buildContainerRequest(key, keyHint, clientId, hurricache::ContainerType::ORDERED_SET, ttl, initialValue),
        timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateContainer,
        [](hurricache::KeyHintResponse& resp) -> KeyHint {
            const auto& kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<KeyHint> FastCacheStandaloneClient::createMap(const Key& key, const KeyHint& keyHint,
                                                           const std::map<Key, Value>& initialValue,
                                                           std::chrono::milliseconds ttl, int32_t clientId,
                                                           std::chrono::milliseconds timeout) {
    hurricache::CreateContainerRequest request;
    *request.mutable_key() = buildKeyProto(key, keyHint, clientId);
    request.set_type(hurricache::ContainerType::MAP);
    
    if (ttl.count() > 0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request.set_ttl(static_cast<uint64_t>(now_ms) + static_cast<uint64_t>(ttl.count()));
    }
    
    for (const auto& [k, v] : initialValue) {
        auto* pk = request.add_key_unordered();
        pk->mutable_payload()->set_size(k.size);
        pk->mutable_payload()->mutable_payload()->assign(k.data, k.size);
        *request.add_value_unordered() = buildValueProtoNoTtl(v, clientId);
    }
    
    return SendAsyncRequest<hurricache::CreateContainerRequest, hurricache::KeyHintResponse, KeyHint>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateContainer,
        [](hurricache::KeyHintResponse& resp) -> KeyHint {
            const auto& kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<KeyHint> FastCacheStandaloneClient::createOrderedMap(const Key& key, const KeyHint& keyHint,
                                                                  const std::map<OrderedKey, OrderedValue>& initialValue,
                                                                  std::chrono::milliseconds ttl, int32_t clientId,
                                                                  std::chrono::milliseconds timeout) {
    hurricache::CreateContainerRequest request;
    *request.mutable_key() = buildKeyProto(key, keyHint, clientId);
    request.set_type(hurricache::ContainerType::ORDERED_MAP);
    
    if (ttl.count() > 0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request.set_ttl(static_cast<uint64_t>(now_ms) + static_cast<uint64_t>(ttl.count()));
    }
    
    for (const auto& [ok, ov] : initialValue) {
        auto* pk = request.add_key_ordered();
        pk->mutable_payload()->set_size(ok.size);
        pk->mutable_payload()->mutable_payload()->assign(ok.data, ok.size);
        pk->set_order(ok.weight);
        auto* pv = request.add_value_unordered();
        pv->mutable_value()->set_size(static_cast<uint32_t>(ov.size));
        pv->mutable_value()->set_payload(absl::string_view(ov.data, static_cast<size_t>(ov.size)));
    }
    
    return SendAsyncRequest<hurricache::CreateContainerRequest, hurricache::KeyHintResponse, KeyHint>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateContainer,
        [](hurricache::KeyHintResponse& resp) -> KeyHint {
            const auto& kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

// =========================================================================
// CONTAINER INFO & BOUNDARY READS
// =========================================================================

std::future<int32_t> FastCacheStandaloneClient::getSize(const Key& key, const KeyHint& hint, int32_t clientId,
                                                         std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::IntResponse, int32_t>(
        buildGetRequestProto(key, hint, clientId), timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetSize,
        [](hurricache::IntResponse& resp) -> int32_t { return static_cast<int32_t>(resp.size()); });
}

std::future<Value> FastCacheStandaloneClient::getHead(const Key& key, const KeyHint& hint, int32_t clientId,
                                                      std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::ValueResponse, Value>(
        buildGetRequestProto(key, hint, clientId), timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetHead,
        [](hurricache::ValueResponse& resp) -> Value {
            if (!resp.has_value_unordered()) return Value{0, nullptr};
            Value* v = valueRequestToValue(resp.value_unordered());
            if (!v) return Value{0, nullptr};
            Value result = *v; delete v; return result;
        });
}

std::future<Value> FastCacheStandaloneClient::getTail(const Key& key, const KeyHint& hint, int32_t clientId,
                                                      std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::ValueResponse, Value>(
        buildGetRequestProto(key, hint, clientId), timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetTail,
        [](hurricache::ValueResponse& resp) -> Value {
            if (!resp.has_value_unordered()) return Value{0, nullptr};
            Value* v = valueRequestToValue(resp.value_unordered());
            if (!v) return Value{0, nullptr};
            Value result = *v; delete v; return result;
        });
}

// =========================================================================
// POSITIONAL READS
// =========================================================================


std::future<Value> FastCacheStandaloneClient::getElementAtPosition(const Key& key, const KeyHint& hint, int32_t pos,
                                                                    int32_t clientId, std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::KeyPositionRequest, hurricache::ValueResponse, Value>(
        buildPositionRequestProto(key, hint, clientId, pos), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetElementAtPosition,
        [](hurricache::ValueResponse& resp) -> Value {
            if (!resp.has_value_unordered()) return Value{0, nullptr};
            Value* v = valueRequestToValue(resp.value_unordered());
            if (!v) return Value{0, nullptr};
            Value result = *v; delete v; return result;
        });
}

std::future<Value> FastCacheStandaloneClient::getElementWithWeight(const Key& key, const KeyHint& hint, int32_t pos,
                                                                    int32_t clientId, std::chrono::milliseconds timeout) {
    return getElementAtPosition(key, hint, pos, clientId, timeout);
}

// =========================================================================
// POP OPERATIONS (Extraction with removal)
// =========================================================================

std::future<Value> FastCacheStandaloneClient::getAndRemoveFront(const Key& key, const KeyHint& hint, int32_t clientId,
                                                                 std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::ValueResponse, Value>(
        buildGetRequestProto(key, hint, clientId), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetAndRemoveFront,
        [](hurricache::ValueResponse& resp) -> Value {
            if (!resp.has_value_unordered()) return Value{0, nullptr};
            Value* v = valueRequestToValue(resp.value_unordered());
            if (!v) return Value{0, nullptr};
            Value result = *v; delete v; return result;
        });
}

std::future<Value> FastCacheStandaloneClient::getAndRemoveTail(const Key& key, const KeyHint& hint, int32_t clientId,
                                                                std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::ValueResponse, Value>(
        buildGetRequestProto(key, hint, clientId), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetAndRemoveTail,
        [](hurricache::ValueResponse& resp) -> Value {
            if (!resp.has_value_unordered()) return Value{0, nullptr};
            Value* v = valueRequestToValue(resp.value_unordered());
            if (!v) return Value{0, nullptr};
            Value result = *v; delete v; return result;
        });
}

std::future<Value> FastCacheStandaloneClient::getAndRemoveElementAtPosition(const Key& key, const KeyHint& hint,
                                                                             int32_t pos, int32_t clientId,
                                                                             std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::KeyPositionRequest, hurricache::ValueResponse, Value>(
        buildPositionRequestProto(key, hint, clientId, pos), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetAndRemoveElementAtPosition,
        [](hurricache::ValueResponse& resp) -> Value {
            if (!resp.has_value_unordered()) return Value{0, nullptr};
            Value* v = valueRequestToValue(resp.value_unordered());
            if (!v) return Value{0, nullptr};
            Value result = *v; delete v; return result;
        });
}

std::future<Value> FastCacheStandaloneClient::getAndRemoveElementWithWeight(const Key& key, const KeyHint& hint,
                                                                             int32_t pos, int32_t clientId,
                                                                             std::chrono::milliseconds timeout) {
    return getAndRemoveElementAtPosition(key, hint, pos, clientId, timeout);
}

// =========================================================================
// STREAMING READ OPERATIONS
// =========================================================================

std::future<std::vector<Value>> FastCacheStandaloneClient::streamList(const Key& key, const KeyHint& hint, int32_t clientId,
                                                                       std::chrono::milliseconds timeout) {
    return SendAsyncStreamRequest<hurricache::GetRequest, hurricache::BatchValueResponse, std::vector<Value>>(
        buildGetRequestProto(key, hint, clientId), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::PrepareAsyncgetContainer,
        [](std::vector<Value>& result, hurricache::BatchValueResponse& chunk) {
            for (int i = 0; i < chunk.value_unordered_size(); ++i) {
                Value* v = valueRequestToValue(chunk.value_unordered(i));
                if (v) { result.push_back(*v); delete v; }
            }
        });
}

std::future<std::vector<Value>> FastCacheStandaloneClient::streamVector(const Key& key, const KeyHint& hint, int32_t clientId,
                                                                         std::chrono::milliseconds timeout) {
    return streamList(key, hint, clientId, timeout);
}

std::future<std::vector<Value>> FastCacheStandaloneClient::streamSet(const Key& key, const KeyHint& hint, int32_t clientId,
                                                                      std::chrono::milliseconds timeout) {
    return streamList(key, hint, clientId, timeout);
}

std::future<std::map<Key, Value>> FastCacheStandaloneClient::streamMap(const Key& key, const KeyHint& hint, int32_t clientId,
                                                                        std::chrono::milliseconds timeout) {
    return SendAsyncStreamRequest<hurricache::GetRequest, hurricache::BatchValueResponse, std::map<Key, Value>>(
        buildGetRequestProto(key, hint, clientId), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::PrepareAsyncgetContainer,
        [](std::map<Key, Value>& result, hurricache::BatchValueResponse& chunk) {
            int count = std::min(chunk.key_unordered_size(), chunk.value_unordered_size());
            for (int i = 0; i < count; ++i) {
                KeyHint kh;
                Key* k = keyRequestToKey(chunk.key_unordered(i), &kh);
                Value* v = valueRequestToValue(chunk.value_unordered(i));
                if (k && v) result[*k] = *v;
                delete k; delete v;
            }
        });
}

std::future<std::vector<OrderedValue>> FastCacheStandaloneClient::streamOrderedSet(const Key& key, const KeyHint& hint,
                                                                                    int32_t clientId,
                                                                                    std::chrono::milliseconds timeout) {
    return SendAsyncStreamRequest<hurricache::GetRequest, hurricache::BatchValueResponse, std::vector<OrderedValue>>(
        buildGetRequestProto(key, hint, clientId), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::PrepareAsyncgetContainer,
        [](std::vector<OrderedValue>& result, hurricache::BatchValueResponse& chunk) {
            for (int i = 0; i < chunk.value_ordered_size(); ++i) {
                OrderedValue* ov = valueRequestToOrderedValue(chunk.value_ordered(i));
                if (ov) { result.push_back(*ov); delete ov; }
            }
        });
}

std::future<std::map<OrderedKey, Value>> FastCacheStandaloneClient::streamOrderedMap(const Key& key, const KeyHint& hint,
                                                                                      int32_t clientId,
                                                                                      std::chrono::milliseconds timeout) {
    return SendAsyncStreamRequest<hurricache::GetRequest, hurricache::BatchValueResponse, std::map<OrderedKey, Value>>(
        buildGetRequestProto(key, hint, clientId), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::PrepareAsyncgetContainer,
        [](std::map<OrderedKey, Value>& result, hurricache::BatchValueResponse& chunk) {
            int count = std::min(chunk.key_ordered_size(), chunk.value_unordered_size());
            for (int i = 0; i < count; ++i) {
                KeyHint kh;
                OrderedKey* ok = keyRequestToKey(chunk.key_ordered(i), &kh);
                Value* v = valueRequestToValue(chunk.value_unordered(i));
                if (ok && v) result[*ok] = *v;
                delete ok; delete v;
            }
        });
}

std::future<std::vector<Value>> FastCacheStandaloneClient::streamElementInRangeUnordered(const Key& key, const KeyHint& hint,
                                                                                          ContainerType containerType,
                                                                                          int32_t start, int32_t end,
                                                                                          int32_t clientId,
                                                                                          std::chrono::milliseconds timeout) {
    hurricache::KeyPositionRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_pos(static_cast<uint64_t>(start));
    request.set_end(static_cast<uint64_t>(end));
    request.set_type(static_cast<hurricache::ContainerType>(containerType));
    
    return SendAsyncStreamRequest<hurricache::KeyPositionRequest, hurricache::BatchValueResponse, std::vector<Value>>(
        request, timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::PrepareAsyncgetElementInRange,
        [](std::vector<Value>& result, hurricache::BatchValueResponse& chunk) {
            for (int i = 0; i < chunk.value_unordered_size(); ++i) {
                Value* v = valueRequestToValue(chunk.value_unordered(i));
                if (v) { result.push_back(*v); delete v; }
            }
        });
}

std::future<std::vector<OrderedValue>> FastCacheStandaloneClient::streamElementInRangeOrderedSet(const Key& key, const KeyHint& hint,
                                                                                                  int64_t startWeight, int64_t endWeight,
                                                                                                  bool reverse, int32_t clientId,
                                                                                                  std::chrono::milliseconds timeout) {
    hurricache::KeyPositionRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_pos(static_cast<uint64_t>(startWeight));
    request.set_end(static_cast<uint64_t>(endWeight));
    request.set_type(hurricache::ContainerType::ORDERED_SET);
    request.set_reverse(reverse);
    
    return SendAsyncStreamRequest<hurricache::KeyPositionRequest, hurricache::BatchValueResponse, std::vector<OrderedValue>>(
        request, timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::PrepareAsyncgetElementInRange,
        [](std::vector<OrderedValue>& result, hurricache::BatchValueResponse& chunk) {
            for (int i = 0; i < chunk.value_ordered_size(); ++i) {
                OrderedValue* ov = valueRequestToOrderedValue(chunk.value_ordered(i));
                if (ov) { result.push_back(*ov); delete ov; }
            }
        });
}

std::future<std::map<OrderedKey, OrderedValue>> FastCacheStandaloneClient::streamElementInRangeOrderedMap(const Key& key, const KeyHint& hint,
                                                                                                      int64_t startWeight, int64_t endWeight,
                                                                                                      bool reverse, int32_t clientId,
                                                                                                      std::chrono::milliseconds timeout) {
    hurricache::KeyPositionRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_pos(static_cast<uint64_t>(startWeight));
    request.set_end(static_cast<uint64_t>(endWeight));
    request.set_type(hurricache::ContainerType::ORDERED_MAP);
    request.set_reverse(reverse);
    
    return SendAsyncStreamRequest<hurricache::KeyPositionRequest, hurricache::BatchValueResponse, std::map<OrderedKey, OrderedValue>>(
        request, timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::PrepareAsyncgetElementInRange,
        [](std::map<OrderedKey, OrderedValue>& result, hurricache::BatchValueResponse& chunk) {
            int count = std::min(chunk.key_ordered_size(), chunk.value_unordered_size());
            for (int i = 0; i < count; ++i) {
                KeyHint kh;
                OrderedKey* ok = keyRequestToKey(chunk.key_ordered(i), &kh);
                Value* v = valueRequestToValue(chunk.value_unordered(i));
                if (ok && v) {
                    OrderedValue ov(ok->weight, v->size, v->data);
                    result[*ok] = ov;
                }
                delete ok; delete v;
            }
        });
}

// =========================================================================
// INSERTION OPERATIONS
// =========================================================================

static hurricache::AddToRequest buildAddRequest(const Key& key, const std::vector<Value>& values) {
    hurricache::AddToRequest request;
    *request.mutable_key() = buildKeyProto(key, {0, 0}, 0);
    for (const auto& v : values) {
        *request.add_value_unordered() = buildValueProtoNoTtl(v, 0);
    }
    return request;
}

std::future<int32_t> FastCacheStandaloneClient::addElementUnordered(const Key& key, const KeyHint& hint,
                                                                     const std::vector<Value>& data, int32_t clientId,
                                                                     std::chrono::milliseconds timeout) {
    hurricache::AddToRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    for (const auto& v : data) {
        *request.add_value_unordered() = buildValueProtoNoTtl(v, clientId);
    }
    return SendAsyncRequest<hurricache::AddToRequest, hurricache::IntResponse, int32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElement,
        [](hurricache::IntResponse& resp) -> int32_t { return static_cast<int32_t>(resp.size()); });
}

std::future<int32_t> FastCacheStandaloneClient::addElementWithWeight(const Key& key, const KeyHint& hint,
                                                                      const std::vector<OrderedValue>& data, int32_t clientId,
                                                                      std::chrono::milliseconds timeout) {
    hurricache::AddToRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    for (const auto& ov : data) {
        auto* ordered_val = request.add_value_ordered();
        ordered_val->set_order(ov.weight);
        ordered_val->mutable_value()->set_size(static_cast<uint32_t>(ov.size));
        ordered_val->mutable_value()->set_payload(absl::string_view(ov.data, static_cast<size_t>(ov.size)));
    }
    return SendAsyncRequest<hurricache::AddToRequest, hurricache::IntResponse, int32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElement,
        [](hurricache::IntResponse& resp) -> int32_t { return static_cast<int32_t>(resp.size()); });
}

std::future<int32_t> FastCacheStandaloneClient::addElementToTail(const Key& key, const KeyHint& hint,
                                                                  const std::vector<Value>& data, int32_t clientId,
                                                                  std::chrono::milliseconds timeout) {
    hurricache::AddToRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    for (const auto& v : data) {
        *request.add_value_unordered() = buildValueProtoNoTtl(v, clientId);
    }
    return SendAsyncRequest<hurricache::AddToRequest, hurricache::IntResponse, int32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElementToTail,
        [](hurricache::IntResponse& resp) -> int32_t { return static_cast<int32_t>(resp.size()); });
}

std::future<int32_t> FastCacheStandaloneClient::addElementToHead(const Key& key, const KeyHint& hint,
                                                                  const std::vector<Value>& data, int32_t clientId,
                                                                  std::chrono::milliseconds timeout) {
    hurricache::AddToRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    for (const auto& v : data) {
        *request.add_value_unordered() = buildValueProtoNoTtl(v, clientId);
    }
    return SendAsyncRequest<hurricache::AddToRequest, hurricache::IntResponse, int32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElementToHead,
        [](hurricache::IntResponse& resp) -> int32_t { return static_cast<int32_t>(resp.size()); });
}

std::future<int32_t> FastCacheStandaloneClient::addElementToPosition(const Key& key, const KeyHint& hint,
                                                                      const std::vector<Value>& data, int32_t pos,
                                                                      int32_t clientId, std::chrono::milliseconds timeout) {
    hurricache::AddToRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_pos(static_cast<uint32_t>(pos));
    for (const auto& v : data) {
        *request.add_value_unordered() = buildValueProtoNoTtl(v, clientId);
    }
    return SendAsyncRequest<hurricache::AddToRequest, hurricache::IntResponse, int32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElement,
        [](hurricache::IntResponse& resp) -> int32_t { return static_cast<int32_t>(resp.size()); });
}

std::future<int32_t> FastCacheStandaloneClient::addElementToPositionBefore(const Key& key, const KeyHint& hint,
                                                                            const std::vector<Value>& data, const Value& pivot,
                                                                            int32_t clientId, std::chrono::milliseconds timeout) {
    hurricache::AddToValRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_isbefore(true);
    *request.mutable_pos() = buildValueProtoNoTtl(pivot, clientId);
    for (const auto& v : data) {
        *request.add_value() = buildValueProtoNoTtl(v, clientId);
    }
    return SendAsyncRequest<hurricache::AddToValRequest, hurricache::IntResponse, int32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElementToPositionByValue,
        [](hurricache::IntResponse& resp) -> int32_t { return static_cast<int32_t>(resp.size()); });
}

std::future<int32_t> FastCacheStandaloneClient::addElementToPositionAfter(const Key& key, const KeyHint& hint,
                                                                           const std::vector<Value>& data, const Value& pivot,
                                                                           int32_t clientId, std::chrono::milliseconds timeout) {
    hurricache::AddToValRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_isbefore(false);
    *request.mutable_pos() = buildValueProtoNoTtl(pivot, clientId);
    for (const auto& v : data) {
        *request.add_value() = buildValueProtoNoTtl(v, clientId);
    }
    return SendAsyncRequest<hurricache::AddToValRequest, hurricache::IntResponse, int32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElementToPositionByValue,
        [](hurricache::IntResponse& resp) -> int32_t { return static_cast<int32_t>(resp.size()); });
}

// =========================================================================
// DELETION OPERATIONS
// =========================================================================

std::future<bool> FastCacheStandaloneClient::removeHead(const Key& key, const KeyHint& hint, int32_t clientId,
                                                         std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::BoolResponse, bool>(
        buildGetRequestProto(key, hint, clientId), timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncremoveHead,
        [](hurricache::BoolResponse& resp) -> bool { return resp.value(); });
}

std::future<bool> FastCacheStandaloneClient::removeTail(const Key& key, const KeyHint& hint, int32_t clientId,
                                                         std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::BoolResponse, bool>(
        buildGetRequestProto(key, hint, clientId), timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncremoveTail,
        [](hurricache::BoolResponse& resp) -> bool { return resp.value(); });
}

std::future<bool> FastCacheStandaloneClient::removeElementAtPosition(const Key& key, const KeyHint& hint, int64_t pos,
                                                                      int64_t endPos, int32_t clientId,
                                                                      std::chrono::milliseconds timeout) {
    hurricache::KeyPositionRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_pos(static_cast<uint64_t>(pos));
    if (endPos > pos) {
        request.set_end(static_cast<uint64_t>(endPos));
    }
    return SendAsyncRequest<hurricache::KeyPositionRequest, hurricache::BoolResponse, bool>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncremoveElementAtPosition,
        [](hurricache::BoolResponse& resp) -> bool { return resp.value(); });
}

std::future<int32_t> FastCacheStandaloneClient::removeFromContainer(const Key& key, const KeyHint& hint,
                                                                     ContainerType type,
                                                                     const std::vector<Key>& keys,
                                                                     const std::vector<Value>& values,
                                                                     int32_t clientId, std::chrono::milliseconds timeout) {
    hurricache::RemoveFromContainerRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_type(static_cast<hurricache::ContainerType>(type));
    for (const auto& k : keys) {
        auto* pk = request.add_keys();
        pk->mutable_payload()->set_size(k.size);
        pk->mutable_payload()->mutable_payload()->assign(k.data, k.size);
    }
    for (const auto& v : values) {
        *request.add_values() = buildValueProtoNoTtl(v, clientId);
    }
    return SendAsyncRequest<hurricache::RemoveFromContainerRequest, hurricache::IntResponse, int32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncremoveFromContainerByKeyValue,
        [](hurricache::IntResponse& resp) -> int32_t { return static_cast<int32_t>(resp.size()); });
}

// =========================================================================
// LOCKING OPERATIONS
// =========================================================================

std::future<LockStatus> FastCacheStandaloneClient::lockObject(const Key& key, const KeyHint& hint, LockType type,
                                                               int32_t clientId, std::chrono::milliseconds duration,
                                                               std::chrono::milliseconds timeout) {
    hurricache::LockRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_locktype(static_cast<hurricache::LockType>(type));
    request.set_clientid(clientId);
    if (duration.count() > 0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request.set_lockduration(static_cast<uint32_t>(now_ms + duration.count()));
    }
    return SendAsyncRequest<hurricache::LockRequest, hurricache::LockResponse, LockStatus>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynclockObject,
        [](hurricache::LockResponse& resp) -> LockStatus {
            return static_cast<LockStatus>(resp.result());
        });
}

std::future<LockStatus> FastCacheStandaloneClient::unlockObject(const Key& key, const KeyHint& hint, int32_t clientId,
                                                                std::chrono::milliseconds timeout) {
    hurricache::UnLockRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_clientid(clientId);
    return SendAsyncRequest<hurricache::UnLockRequest, hurricache::UnlockResponse, LockStatus>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncunlockObject,
        [](hurricache::UnlockResponse& resp) -> LockStatus {
            return static_cast<LockStatus>(resp.result());
        });
}

// =========================================================================
// ATOMIC OPERATIONS
// =========================================================================


std::future<int64_t> FastCacheStandaloneClient::atomicLoad(const Key& key, const KeyHint& hint, int32_t clientId,
                                                            std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::AtomicValue, int64_t>(
        buildGetRequestProto(key, hint, clientId), timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicLoad,
        [](hurricache::AtomicValue& resp) -> int64_t { return resp.val(); });
}

std::future<int64_t> FastCacheStandaloneClient::atomicLoadAndDelete(const Key& key, const KeyHint& hint, int32_t clientId,
                                                                     std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::AtomicValue, int64_t>(
        buildGetRequestProto(key, hint, clientId), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicLoadAndDelete,
        [](hurricache::AtomicValue& resp) -> int64_t { return static_cast<int64_t>(resp.val()); });
}

std::future<KeyHint> FastCacheStandaloneClient::atomicCreate(const Key& key, const KeyHint& hint, int64_t value,
                                                             std::chrono::milliseconds ttl, int32_t clientId,
                                                             std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::KeyHintResponse, KeyHint>(
        buildAtomicCreateProto(key, hint, clientId, value, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicCreate,
        [](hurricache::KeyHintResponse& resp) -> KeyHint {
            const auto& kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<KeyHint> FastCacheStandaloneClient::atomicStore(const Key& key, const KeyHint& hint, int64_t value,
                                                            std::chrono::milliseconds ttl, int32_t clientId,
                                                            std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::KeyHintResponse, KeyHint>(
        buildAtomicCreateProto(key, hint, clientId, value, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicStore,
        [](hurricache::KeyHintResponse& resp) -> KeyHint {
            const auto& kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<int64_t> FastCacheStandaloneClient::atomicExchange(const Key& key, const KeyHint& hint, int64_t value,
                                                               std::chrono::milliseconds ttl, int32_t clientId,
                                                               std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::AtomicValue, int64_t>(
        buildAtomicCreateProto(key, hint, clientId, value, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicExchange,
        [](hurricache::AtomicValue& resp) -> int64_t { return resp.val(); });
}

std::future<int64_t> FastCacheStandaloneClient::atomicAdd(const Key& key, const KeyHint& hint, int64_t delta,
                                                          std::chrono::milliseconds ttl, int32_t clientId,
                                                          std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::AtomicValue, int64_t>(
        buildAtomicCreateProto(key, hint, clientId, delta, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicAdd,
        [](hurricache::AtomicValue& resp) -> int64_t { return resp.val(); });
}

std::future<int64_t> FastCacheStandaloneClient::atomicSub(const Key& key, const KeyHint& hint, int64_t delta,
                                                          std::chrono::milliseconds ttl, int32_t clientId,
                                                          std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::AtomicValue, int64_t>(
        buildAtomicCreateProto(key, hint, clientId, delta, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicSub,
        [](hurricache::AtomicValue& resp) -> int64_t { return resp.val(); });
}

std::future<int64_t> FastCacheStandaloneClient::atomicAnd(const Key& key, const KeyHint& hint, int64_t mask,
                                                          std::chrono::milliseconds ttl, int32_t clientId,
                                                          std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::AtomicValue, int64_t>(
        buildAtomicCreateProto(key, hint, clientId, mask, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicAnd,
        [](hurricache::AtomicValue& resp) -> int64_t { return resp.val(); });
}

std::future<int64_t> FastCacheStandaloneClient::atomicOr(const Key& key, const KeyHint& hint, int64_t mask,
                                                         std::chrono::milliseconds ttl, int32_t clientId,
                                                         std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::AtomicValue, int64_t>(
        buildAtomicCreateProto(key, hint, clientId, mask, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicOr,
        [](hurricache::AtomicValue& resp) -> int64_t { return resp.val(); });
}

std::future<int64_t> FastCacheStandaloneClient::atomicXor(const Key& key, const KeyHint& hint, int64_t mask,
                                                          std::chrono::milliseconds ttl, int32_t clientId,
                                                          std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::AtomicValue, int64_t>(
        buildAtomicCreateProto(key, hint, clientId, mask, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicXor,
        [](hurricache::AtomicValue& resp) -> int64_t { return resp.val(); });
}

std::future<AtomicCasRes> FastCacheStandaloneClient::atomicCompareAndSet(const Key& key, const KeyHint& hint,
                                                                         int64_t expectedValue, int64_t newValue,
                                                                         std::chrono::milliseconds ttl, int32_t clientId,
                                                                         std::chrono::milliseconds timeout) {
    hurricache::AtomicCas request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    auto* exp = request.mutable_expected();
    exp->set_val(expectedValue);
    auto* toset = request.mutable_toset();
    toset->set_val(newValue);
    if (ttl.count() > 0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request.set_ttl(static_cast<uint64_t>(now_ms) + static_cast<uint64_t>(ttl.count()));
    }
    return SendAsyncRequest<hurricache::AtomicCas, hurricache::AtomicCasRes, AtomicCasRes>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicCompareAndSet,
        [](hurricache::AtomicCasRes& resp) -> AtomicCasRes {
            return AtomicCasRes{resp.result(), resp.expected().val()};
        });
}

// =========================================================================
// CONTAINER VALUE OPERATIONS
// =========================================================================


std::future<Value> FastCacheStandaloneClient::getContainerValue(const Key& key, const KeyHint& hint, const Key& elementKey,
                                                                 int32_t clientId, std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::ContainerGetRequest, hurricache::ValueResponse, Value>(
        buildContainerGetRequestProto(key, hint, clientId, elementKey), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetValueInContainer,
        [](hurricache::ValueResponse& resp) -> Value {
            if (!resp.has_value_unordered()) return Value{0, nullptr};
            Value* v = valueRequestToValue(resp.value_unordered());
            if (!v) return Value{0, nullptr};
            Value result = *v; delete v; return result;
        });
}

std::future<Value> FastCacheStandaloneClient::getAndRemoveContainerValue(const Key& key, const KeyHint& hint,
                                                                        const Key& elementKey, int32_t clientId,
                                                                        std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::ContainerGetRequest, hurricache::ValueResponse, Value>(
        buildContainerGetRequestProto(key, hint, clientId, elementKey), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetAndDeleteValueInContainer,
        [](hurricache::ValueResponse& resp) -> Value {
            if (!resp.has_value_unordered()) return Value{0, nullptr};
            Value* v = valueRequestToValue(resp.value_unordered());
            if (!v) return Value{0, nullptr};
            Value result = *v; delete v; return result;
        });
}

std::future<bool> FastCacheStandaloneClient::containsContainerKey(const Key& key, const KeyHint& hint, const Key& elementKey,
                                                                  int32_t clientId, std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::ContainerGetRequest, hurricache::BoolResponse, bool>(
        buildContainerGetRequestProto(key, hint, clientId, elementKey), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncexistKeyInContainer,
        [](hurricache::BoolResponse& resp) -> bool { return resp.value(); });
}

std::future<Value> FastCacheStandaloneClient::updateContainerValue(const Key& key, const KeyHint& hint, const Key& elementKey,
                                                                    const Value& value, int32_t clientId,
                                                                    std::chrono::milliseconds timeout) {
    hurricache::UpdateContainerRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    auto* ek = request.mutable_element_key();
    ek->mutable_payload()->set_size(elementKey.size);
    ek->mutable_payload()->mutable_payload()->assign(elementKey.data, elementKey.size);
    *request.mutable_value() = buildValueProtoNoTtl(value, clientId);
    return SendAsyncRequest<hurricache::UpdateContainerRequest, hurricache::UpdateValueResponse, Value>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncupdateValueInContainer,
        [](hurricache::UpdateValueResponse& resp) -> Value {
            if (!resp.has_value()) return Value{0, nullptr};
            Value* v = valueRequestToValue(resp.value());
            if (!v) return Value{0, nullptr};
            Value result = *v; delete v; return result;
        });
}

std::future<int32_t> FastCacheStandaloneClient::removeFromContainer(const Key& key, const KeyHint& hint,
                                                                     const Key& elementKey, int32_t clientId,
                                                                     std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::ContainerGetRequest, hurricache::IntResponse, int32_t>(
        buildContainerGetRequestProto(key, hint, clientId, elementKey), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncremoveInContainer,
        [](hurricache::IntResponse& resp) -> int32_t { return static_cast<int32_t>(resp.size()); });
}

std::future<int32_t> FastCacheStandaloneClient::addElementHashMap(const Key& key, const KeyHint& hint,
                                                                   const std::vector<Key>& container_keys,
                                                                   const std::vector<Value>& container_values,
                                                                   int32_t clientId, std::chrono::milliseconds timeout) {
    hurricache::AddToRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_type(hurricache::ContainerType::MAP);
    for (size_t i = 0; i < container_keys.size(); ++i) {
        auto* pk = request.add_key_unordered();
        pk->mutable_payload()->set_size(container_keys[i].size);
        pk->mutable_payload()->mutable_payload()->assign(container_keys[i].data, container_keys[i].size);
        *request.add_value_unordered() = buildValueProtoNoTtl(container_values[i], clientId);
    }
    return SendAsyncRequest<hurricache::AddToRequest, hurricache::IntResponse, int32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElement,
        [](hurricache::IntResponse& resp) -> int32_t { return static_cast<int32_t>(resp.size()); });
}

std::future<int32_t> FastCacheStandaloneClient::addElementOrderedMap(const Key& key, const KeyHint& hint,
                                                                      const std::vector<OrderedValue>& container_keys,
                                                                      const std::vector<Value>& container_values,
                                                                      int32_t clientId, std::chrono::milliseconds timeout) {
    hurricache::AddToRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_type(hurricache::ContainerType::ORDERED_MAP);
    for (size_t i = 0; i < container_keys.size(); ++i) {
        auto* pk = request.add_key_ordered();
        pk->mutable_payload()->set_size(container_keys[i].size);
        pk->mutable_payload()->mutable_payload()->assign(container_keys[i].data, container_keys[i].size);
        pk->set_order(container_keys[i].weight);
        *request.add_value_unordered() = buildValueProtoNoTtl(container_values[i], clientId);
    }
    return SendAsyncRequest<hurricache::AddToRequest, hurricache::IntResponse, int32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElement,
        [](hurricache::IntResponse& resp) -> int32_t { return static_cast<int32_t>(resp.size()); });
}
