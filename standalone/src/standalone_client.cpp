#include <standalone_client.hxx>
#include "utils.hxx"
#include "grpcpp/security/credentials.h"

// =========================================================================
// CONSTRUCTORS
// =========================================================================

FastCacheStandaloneClient::FastCacheStandaloneClient(
    const std::string &host, int32_t port, std::chrono::milliseconds timeout, int32_t defaultCompressionThreshold)
    : defaultTimeout_(timeout),
      defaultCompressionThreshold_(defaultCompressionThreshold),
      target_(host + ":" + std::to_string(port)) {
    grpc::ChannelArguments args;
    channel_ = grpc::CreateCustomChannel(target_, grpc::InsecureChannelCredentials(), args);
    asyncStub_ = hurricache::HurriCacheGrpcService::NewStub(channel_);

    completion_queue_thread_ = std::jthread([this] {
        RunCompletionQueue();
    });
}

FastCacheStandaloneClient::FastCacheStandaloneClient(
    const std::string &host, int32_t port, std::chrono::milliseconds timeout)
    : FastCacheStandaloneClient(host, port, timeout, kDefaultCompressionThreshold) {
}

FastCacheStandaloneClient::FastCacheStandaloneClient(const std::string &host, int32_t port)
    : FastCacheStandaloneClient(host, port, std::chrono::seconds(1), kDefaultCompressionThreshold) {
}


FastCacheStandaloneClient::FastCacheStandaloneClient(
    std::shared_ptr<grpc::Channel> channel, std::string target,
    std::chrono::milliseconds duration, int32_t defaultCompressionThreshold)
    : channel_(std::move(channel)),
      defaultTimeout_(duration),
      defaultCompressionThreshold_(defaultCompressionThreshold),
      target_(std::move(target)) {
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
std::chrono::milliseconds FastCacheStandaloneClient::getDefaultTimeout() const { return defaultTimeout_; }
int32_t FastCacheStandaloneClient::getDefaultCompressionThreshold() const { return defaultCompressionThreshold_; }

// =========================================================================
// LIFECYCLE
// =========================================================================

void FastCacheStandaloneClient::shutdown() {
    bool expected = false;
    if (!shutdown_called_.compare_exchange_strong(expected, true)) {
        return; // Already shut down
    }
    // Reset stub/channel BEFORE shutdown to reject new calls immediately
    asyncStub_.reset();
    channel_.reset();
    // Now shutdown completion queue so RunCompletionQueue returns
    cq_.Shutdown();
    if (completion_queue_thread_.joinable()) {
        completion_queue_thread_.join();
    }
}

void FastCacheStandaloneClient::RunCompletionQueue() {
    void *tag = nullptr;
    bool ok = false;
    while (cq_.Next(&tag, &ok)) {
        auto *call_data = static_cast<RpcCallDataBase *>(tag);
        call_data->Proceed(ok);
    }
}

FastCacheStandaloneClient::~FastCacheStandaloneClient() {
    shutdown();
}

// =========================================================================
// TTL MANAGEMENT
// =========================================================================

std::future<bool> FastCacheStandaloneClient::setTtl(const Key &key, const KeyHint *hint, uint64_t ttl,
                                                    int32_t clientId, std::chrono::milliseconds timeout) {
    hurricache::TtlRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);

    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    request.set_ttl(static_cast<uint64_t>(now_ms) + ttl);

    return SendAsyncRequest<hurricache::TtlRequest, hurricache::BoolResponse, bool>(
        request, timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncsetTtl,
        [](hurricache::BoolResponse &resp) -> bool { return resp.value(); });
}

std::future<int64_t> FastCacheStandaloneClient::getTtl(const Key &key, const KeyHint *hint, int32_t clientId,
                                                       std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::TtlResponse, int64_t>(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetTtl,
        [](hurricache::TtlResponse &resp) -> int64_t {
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

std::future<ValuePtr> FastCacheStandaloneClient::getAndDeleteValue(const Key &key, const KeyHint *hint,
                                                                   int32_t clientId,
                                                                   std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::ValueResponse, ValuePtr>(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetAndDeleteValue,
        [](hurricache::ValueResponse &resp) -> ValuePtr {
            if (!resp.has_value_unordered()) {
                return nullptr;
            }
            return valueRequestToValue(resp.value_unordered());
        });
}

std::future<KeyHint> FastCacheStandaloneClient::createKeyValue(const Key &key, const KeyHint *hint, const Value &value,
                                                               std::chrono::milliseconds ttl, int32_t clientId,
                                                               std::chrono::milliseconds timeout) {
    hurricache::CreateRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    *request.mutable_value() = buildValueProto(value, ttl, clientId);

    return SendAsyncRequest<hurricache::CreateRequest, hurricache::KeyHintResponse, KeyHint>(
        request, timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateKeyValue,
        [](hurricache::KeyHintResponse &resp) -> KeyHint {
            const auto &kh = resp.keyhint();
            return KeyHint{
                kh.has_week_hash() ? kh.week_hash() : 0,
                kh.has_strong_hash() ? kh.strong_hash() : 0
            };
        });
}

std::future<ValuePtr> FastCacheStandaloneClient::getValue(const Key &key, const KeyHint *hint, int32_t clientId,
                                                          std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::ValueResponse, ValuePtr>(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetValue,
        [](hurricache::ValueResponse &resp) -> ValuePtr {
            if (!resp.has_value_unordered()) {
                return nullptr;
            }
            return valueRequestToValue(resp.value_unordered());
        });
}

std::future<ValuePtr> FastCacheStandaloneClient::updateKeyValue(const Key &key, const KeyHint *hint, const Value &value,
                                                                std::chrono::milliseconds ttl, int32_t clientId,
                                                                std::chrono::milliseconds timeout) {
    hurricache::UpdateRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    *request.mutable_value() = buildValueProto(value, ttl, clientId);

    return SendAsyncRequest<hurricache::UpdateRequest, hurricache::UpdateValueResponse, ValuePtr>(
        request, timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncupdateValue,
        [](hurricache::UpdateValueResponse &resp) -> ValuePtr {
            if (!resp.has_value()) {
                return nullptr;
            }
            return valueRequestToValue(resp.value());
        });
}

std::future<bool> FastCacheStandaloneClient::existKey(const Key &key, const KeyHint *hint, int32_t clientId,
                                                      std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::BoolResponse, bool>(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncexistKey,
        [](hurricache::BoolResponse &resp) -> bool { return resp.value(); });
}

std::future<bool> FastCacheStandaloneClient::remove(const Key &key, const KeyHint *hint, int32_t clientId,
                                                    std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::BoolResponse, bool>(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::Asyncremove,
        [](hurricache::BoolResponse &resp) -> bool { return resp.value(); });
}

// =========================================================================
// CONTAINER CREATION (UNORDERED & ORDERED)
// =========================================================================


std::future<KeyHint> FastCacheStandaloneClient::createQueue(const Key &key, const KeyHint *hint,
                                                            const std::vector<ValuePtr> *initialValue,
                                                            std::chrono::milliseconds ttl, int32_t clientId,
                                                            std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::CreateContainerRequest, hurricache::KeyHintResponse, KeyHint>(
        buildContainerRequest(key, hint, clientId, hurricache::ContainerType::QUEUE, ttl, initialValue,
                              defaultCompressionThreshold_),
        timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateContainer,
        [](hurricache::KeyHintResponse &resp) -> KeyHint {
            const auto &kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<KeyHint> FastCacheStandaloneClient::createList(const Key &key, const KeyHint *hint,
                                                           const std::vector<ValuePtr> *initialValue,
                                                           std::chrono::milliseconds ttl, int32_t clientId,
                                                           std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::CreateContainerRequest, hurricache::KeyHintResponse, KeyHint>(
        buildContainerRequest(key, hint, clientId, hurricache::ContainerType::LIST, ttl, initialValue,
                              defaultCompressionThreshold_),
        timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateContainer,
        [](hurricache::KeyHintResponse &resp) -> KeyHint {
            const auto &kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<KeyHint> FastCacheStandaloneClient::createVector(const Key &key, const KeyHint *hint,
                                                             const std::vector<ValuePtr> *initialValue,
                                                             std::chrono::milliseconds ttl, int32_t clientId,
                                                             std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::CreateContainerRequest, hurricache::KeyHintResponse, KeyHint>(
        buildContainerRequest(key, hint, clientId, hurricache::ContainerType::VECTOR, ttl, initialValue,
                              defaultCompressionThreshold_),
        timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateContainer,
        [](hurricache::KeyHintResponse &resp) -> KeyHint {
            const auto &kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<KeyHint> FastCacheStandaloneClient::createSet(const Key &key, const KeyHint *hint,
                                                          const std::vector<ValuePtr> *initialValue,
                                                          std::chrono::milliseconds ttl, int32_t clientId,
                                                          std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::CreateContainerRequest, hurricache::KeyHintResponse, KeyHint>(
        buildContainerRequest(key, hint, clientId, hurricache::ContainerType::SET, ttl, initialValue,
                              defaultCompressionThreshold_),
        timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateContainer,
        [](hurricache::KeyHintResponse &resp) -> KeyHint {
            const auto &kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<KeyHint> FastCacheStandaloneClient::createOrderedSet(const Key &key, const KeyHint *hint,
                                                                 const std::vector<OrderedValuePtr> *initialValue,
                                                                 std::chrono::milliseconds ttl, int32_t clientId,
                                                                 std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::CreateContainerRequest, hurricache::KeyHintResponse, KeyHint>(
        buildContainerRequestOrdered(key, hint, clientId, hurricache::ContainerType::ORDERED_SET, ttl, initialValue,
                                     defaultCompressionThreshold_),
        timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateContainer,
        [](hurricache::KeyHintResponse &resp) -> KeyHint {
            const auto &kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<KeyHint> FastCacheStandaloneClient::createMap(const Key &key, const KeyHint *hint,
                                                          const std::map<KeyPtr, ValuePtr> *initialValue,
                                                          std::chrono::milliseconds ttl, int32_t clientId,
                                                          std::chrono::milliseconds timeout) {
    hurricache::CreateContainerRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_type(hurricache::ContainerType::MAP);

    if (ttl.count() > 0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request.set_ttl(static_cast<uint64_t>(now_ms) + static_cast<uint64_t>(ttl.count()));
    }
    if (initialValue != nullptr) {
        for (const auto &[k, v]: *initialValue) {
            auto *pk = request.add_key_unordered();
            pk->mutable_payload()->set_size(k->size);
            pk->mutable_payload()->mutable_payload()->assign(k->data, k->size);
            *request.add_value_unordered() = buildValueProtoNoTtl(v, clientId);
        }
    }
    return SendAsyncRequest<hurricache::CreateContainerRequest, hurricache::KeyHintResponse, KeyHint>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateContainer,
        [](hurricache::KeyHintResponse &resp) -> KeyHint {
            const auto &kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<KeyHint> FastCacheStandaloneClient::createOrderedMap(const Key &key, const KeyHint *hint,
                                                                 const std::map<OrderedKeyPtr, OrderedValuePtr> *
                                                                 initialValue,
                                                                 std::chrono::milliseconds ttl, int32_t clientId,
                                                                 std::chrono::milliseconds timeout) {
    hurricache::CreateContainerRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_type(hurricache::ContainerType::ORDERED_MAP);

    if (ttl.count() > 0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request.set_ttl(static_cast<uint64_t>(now_ms) + static_cast<uint64_t>(ttl.count()));
    }
    if (initialValue != nullptr) {
        for (const auto &[ok, ov]: *initialValue) {
            auto *pk = request.add_key_ordered();
            pk->mutable_payload()->set_size(ok->size);
            pk->mutable_payload()->mutable_payload()->assign(ok->data, ok->size);
            pk->set_order(ok->weight);
            auto *pv = request.add_value_unordered();
            pv->mutable_value()->set_size(static_cast<uint32_t>(ov->size));
            pv->mutable_value()->set_payload(absl::string_view(ov->data, static_cast<size_t>(ov->size)));
        }
    }

    return SendAsyncRequest<hurricache::CreateContainerRequest, hurricache::KeyHintResponse, KeyHint>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynccreateContainer,
        [](hurricache::KeyHintResponse &resp) -> KeyHint {
            const auto &kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

// =========================================================================
// CONTAINER INFO & BOUNDARY READS
// =========================================================================

std::future<uint32_t> FastCacheStandaloneClient::getSize(const Key &key, const KeyHint *hint, int32_t clientId,
                                                         std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::IntResponse, uint32_t>(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetSize,
        [](hurricache::IntResponse &resp) -> uint32_t { return static_cast<uint32_t>(resp.size()); });
}

std::future<ValuePtr> FastCacheStandaloneClient::getHead(const Key &key, const KeyHint *hint, int32_t clientId,
                                                         std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::ValueResponse, ValuePtr>(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetHead,
        [](hurricache::ValueResponse &resp) -> ValuePtr {
            if (!resp.has_value_unordered()) {
                return nullptr;
            }
            return valueRequestToValue(resp.value_unordered());
        });
}

std::future<ValuePtr> FastCacheStandaloneClient::getTail(const Key &key, const KeyHint *hint, int32_t clientId,
                                                         std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::ValueResponse, ValuePtr>(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetTail,
        [](hurricache::ValueResponse &resp) -> ValuePtr {
            if (!resp.has_value_unordered()) {
                return nullptr;
            }
            return valueRequestToValue(resp.value_unordered());
        });
}

// =========================================================================
// POSITIONAL READS
// =========================================================================


std::future<ValuePtr> FastCacheStandaloneClient::getElementAtPosition(const Key &key, const KeyHint *hint, uint64_t pos,
                                                                      int32_t clientId,
                                                                      std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::KeyPositionRequest, hurricache::ValueResponse, ValuePtr>(
        buildPositionRequestProto(key, hint, clientId, pos), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetElementAtPosition,
        [](hurricache::ValueResponse &resp) -> ValuePtr {
            if (!resp.has_value_unordered()) {
                return nullptr;
            }
            return valueRequestToValue(resp.value_unordered());
        });
}

std::future<ValuePtr> FastCacheStandaloneClient::getElementWithWeight(const Key &key, const KeyHint *hint, uint64_t pos,
                                                                      int32_t clientId,
                                                                      std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::KeyPositionRequest, hurricache::ValueResponse, ValuePtr>(
        buildPositionRequestProto(key, hint, clientId, pos), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetElementAtPosition,
        [](hurricache::ValueResponse &resp) -> ValuePtr {
            if (resp.has_value_ordered()) {
                return valueRequestToOrderedValue(resp.value_ordered());
            }
            return nullptr;
        });
}

// =========================================================================
// POP OPERATIONS (Extraction with removal)
// =========================================================================

std::future<ValuePtr> FastCacheStandaloneClient::getAndRemoveFront(const Key &key, const KeyHint *hint,
                                                                   int32_t clientId,
                                                                   std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::ValueResponse, ValuePtr>(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetAndRemoveFront,
        [](hurricache::ValueResponse &resp) -> ValuePtr {
            if (!resp.has_value_unordered()) {
                return nullptr;
            }
            return valueRequestToValue(resp.value_unordered());
        });
}

std::future<ValuePtr> FastCacheStandaloneClient::getAndRemoveTail(const Key &key, const KeyHint *hint, int32_t clientId,
                                                                  std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::ValueResponse, ValuePtr>(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetAndRemoveTail,
        [](hurricache::ValueResponse &resp) -> ValuePtr {
            if (!resp.has_value_unordered()) {
                return nullptr;
            }
            return valueRequestToValue(resp.value_unordered());
        });
}

std::future<ValuePtr> FastCacheStandaloneClient::getAndRemoveElementAtPosition(const Key &key, const KeyHint *hint,
                                                                               uint64_t pos, int32_t clientId,
                                                                               std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::KeyPositionRequest, hurricache::ValueResponse, ValuePtr>(
        buildPositionRequestProto(key, hint, clientId, pos), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetAndRemoveElementAtPosition,
        [](hurricache::ValueResponse &resp) -> ValuePtr {
            if (!resp.has_value_unordered()) {
                return nullptr;
            }
            return valueRequestToValue(resp.value_unordered());
        });
}

std::future<ValuePtr> FastCacheStandaloneClient::getAndRemoveElementWithWeight(const Key &key, const KeyHint *hint,
                                                                               uint64_t pos, int32_t clientId,
                                                                               std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::KeyPositionRequest, hurricache::ValueResponse, ValuePtr>(
        buildPositionRequestProto(key, hint, clientId, pos), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetAndRemoveElementAtPosition,
        [](hurricache::ValueResponse &resp) -> ValuePtr {
            if (resp.has_value_ordered()) {
                return valueRequestToOrderedValue(resp.value_ordered());
            }
            return nullptr;
        });
}

// =========================================================================
// STREAMING READ OPERATIONS
// =========================================================================

std::future<std::vector<ValuePtr> > FastCacheStandaloneClient::streamList(
    const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
    return SendAsyncStreamRequest<hurricache::GetRequest, hurricache::BatchValueResponse, std::vector<ValuePtr> >(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::PrepareAsyncgetContainer,
        [](std::vector<ValuePtr> &result, hurricache::BatchValueResponse &chunk) {
            for (int i = 0; i < chunk.value_unordered_size(); ++i) {
                result.push_back(valueRequestToValue(chunk.value_unordered(i)));
            }
        });
}

std::future<std::vector<ValuePtr> > FastCacheStandaloneClient::streamVector(
    const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
    return streamList(key, hint, clientId, timeout);
}

std::future<std::vector<ValuePtr> > FastCacheStandaloneClient::streamSet(
    const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
    return streamList(key, hint, clientId, timeout);
}

std::future<std::map<KeyPtr, ValuePtr> > FastCacheStandaloneClient::streamMap(
    const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
    return SendAsyncStreamRequest<hurricache::GetRequest, hurricache::BatchValueResponse, std::map<KeyPtr, ValuePtr> >(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::PrepareAsyncgetContainer,
        [](std::map<KeyPtr, ValuePtr> &result, hurricache::BatchValueResponse &chunk) {
            int count = std::min(chunk.key_unordered_size(), chunk.value_unordered_size());
            for (int i = 0; i < count; ++i) {
                KeyPtr k = keyRequestToKey(chunk.key_unordered(i));
                ValuePtr v = valueRequestToValue(chunk.value_unordered(i));
                if (!k) {
                    delete v; // k is null, leak v
                } else if (!v) {
                    delete k; // v is null, leak k
                } else {
                    result[k] = v;
                }
            }
        });
}

std::future<std::vector<OrderedValuePtr> > FastCacheStandaloneClient::streamOrderedSet(
    const Key &key, const KeyHint *hint,
    int32_t clientId,
    std::chrono::milliseconds timeout) {
    return SendAsyncStreamRequest<hurricache::GetRequest, hurricache::BatchValueResponse, std::vector<
        OrderedValuePtr> >(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::PrepareAsyncgetContainer,
        [](std::vector<OrderedValuePtr> &result, hurricache::BatchValueResponse &chunk) {
            for (int i = 0; i < chunk.value_ordered_size(); ++i) {
                result.push_back(valueRequestToOrderedValue(chunk.value_ordered(i)));
            }
        });
}

std::future<std::vector<std::pair<OrderedKeyPtr, ValuePtr> > > FastCacheStandaloneClient::streamOrderedMap(
    const Key &key, const KeyHint *hint,
    int32_t clientId,
    std::chrono::milliseconds timeout) {
    return SendAsyncStreamRequest<hurricache::GetRequest, hurricache::BatchValueResponse, std::vector<std::pair<
        OrderedKeyPtr, ValuePtr> > >(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::PrepareAsyncgetContainer,
        [](std::vector<std::pair<OrderedKeyPtr, ValuePtr> > &result, hurricache::BatchValueResponse &chunk) {
            int count = std::min(chunk.key_ordered_size(), chunk.value_unordered_size());
            for (int i = 0; i < count; ++i) {
                OrderedKeyPtr ok = keyRequestToKey(chunk.key_ordered(i));
                ValuePtr v = valueRequestToValue(chunk.value_unordered(i));
                if (!ok) {
                    delete v;
                } else if (!v) {
                    delete ok;
                } else {
                    result.emplace_back(ok, v);
                }
            }
        });
}

std::future<std::vector<ValuePtr> > FastCacheStandaloneClient::streamElementInRangeUnordered(
    const Key &key, const KeyHint *hint,
    ContainerType containerType,
    uint64_t start, uint64_t end,
    int32_t clientId,
    std::chrono::milliseconds timeout) {
    hurricache::KeyPositionRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_pos(static_cast<uint64_t>(start));
    request.set_end(static_cast<uint64_t>(end));
    request.set_type(static_cast<hurricache::ContainerType>(containerType));

    return SendAsyncStreamRequest<hurricache::KeyPositionRequest, hurricache::BatchValueResponse, std::vector<
        ValuePtr> >(
        request, timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::PrepareAsyncgetElementInRange,
        [](std::vector<ValuePtr> &result, hurricache::BatchValueResponse &chunk) {
            for (int i = 0; i < chunk.value_unordered_size(); ++i) {
                result.push_back(valueRequestToValue(chunk.value_unordered(i)));
            }
        });
}

std::future<std::vector<OrderedValuePtr> > FastCacheStandaloneClient::streamElementInRangeOrderedSet(
    const Key &key, const KeyHint *hint,
    uint64_t startWeight, uint64_t endWeight,
    bool reverse, int32_t clientId,
    std::chrono::milliseconds timeout) {
    hurricache::KeyPositionRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_pos(startWeight);
    request.set_end(endWeight);
    request.set_type(hurricache::ContainerType::ORDERED_SET);
    request.set_reverse(reverse);

    return SendAsyncStreamRequest<hurricache::KeyPositionRequest, hurricache::BatchValueResponse, std::vector<
        OrderedValuePtr> >(
        request, timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::PrepareAsyncgetElementInRange,
        [](std::vector<OrderedValuePtr> &result, hurricache::BatchValueResponse &chunk) {
            for (int i = 0; i < chunk.value_ordered_size(); ++i) {
                result.push_back(valueRequestToOrderedValue(chunk.value_ordered(i)));
            }
        });
}

std::future<std::vector<std::pair<OrderedKeyPtr, ValuePtr> > >
FastCacheStandaloneClient::streamElementInRangeOrderedMap(
    const Key &key, const KeyHint *hint,
    uint64_t startWeight, uint64_t endWeight,
    bool reverse, int32_t clientId,
    std::chrono::milliseconds timeout) {
    hurricache::KeyPositionRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_pos(startWeight);
    request.set_end(endWeight);
    request.set_type(hurricache::ContainerType::ORDERED_MAP);
    request.set_reverse(reverse);

    return SendAsyncStreamRequest<hurricache::KeyPositionRequest, hurricache::BatchValueResponse, std::vector<std::pair<
        OrderedKeyPtr, ValuePtr> > >(
        request, timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::PrepareAsyncgetElementInRange,
        [](std::vector<std::pair<OrderedKeyPtr, ValuePtr> > &result, hurricache::BatchValueResponse &chunk) {
            int count = std::min(chunk.key_ordered_size(), chunk.value_unordered_size());
            for (int i = 0; i < count; ++i) {
                OrderedKeyPtr ok = keyRequestToKey(chunk.key_ordered(i));
                ValuePtr v = valueRequestToValue(chunk.value_unordered(i));
                if (ok && v) {
                    result.emplace_back(ok, v);
                }
            }
        });
}

// =========================================================================
// INSERTION OPERATIONS
// =========================================================================

// static hurricache::AddToRequest buildAddRequest(const Key& key, const std::vector<Value>& values) {
//     hurricache::AddToRequest request;
//     *request.mutable_key() = buildKeyProto(key, {0, 0}, 0);
//     for (const auto& v : values) {
//         *request.add_value_unordered() = buildValueProtoNoTtl(v, 0);
//     }
//     return request;
// }

std::future<uint32_t> FastCacheStandaloneClient::addElementUnordered(const Key &key, const KeyHint *hint,
                                                                     const std::vector<ValuePtr> *data,
                                                                     int32_t clientId,
                                                                     std::chrono::milliseconds timeout) {
    hurricache::AddToRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    if (data != nullptr) {
        for (const auto &v: *data) {
            *request.add_value_unordered() = buildValueProtoNoTtl(v, clientId);
        }
    }
    return SendAsyncRequest<hurricache::AddToRequest, hurricache::IntResponse, uint32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElement,
        [](hurricache::IntResponse &resp) -> uint32_t { return static_cast<uint32_t>(resp.size()); });
}

std::future<uint32_t> FastCacheStandaloneClient::addElementWithWeight(const Key &key, const KeyHint *hint,
                                                                      const std::vector<OrderedValuePtr> *data,
                                                                      int32_t clientId,
                                                                      std::chrono::milliseconds timeout) {
    hurricache::AddToRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    if (data != nullptr) {
        for (const auto &ov: *data) {
            auto *ordered_val = request.add_value_ordered();
            ordered_val->set_order(ov->weight);
            ordered_val->mutable_value()->set_size(static_cast<uint32_t>(ov->size));
            ordered_val->mutable_value()->set_payload(absl::string_view(ov->data, static_cast<size_t>(ov->size)));
        }
    }
    return SendAsyncRequest<hurricache::AddToRequest, hurricache::IntResponse, uint32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElement,
        [](hurricache::IntResponse &resp) -> uint32_t { return static_cast<uint32_t>(resp.size()); });
}

std::future<uint32_t> FastCacheStandaloneClient::addElementToTail(const Key &key, const KeyHint *hint,
                                                                  const std::vector<ValuePtr> *data, int32_t clientId,
                                                                  std::chrono::milliseconds timeout) {
    hurricache::AddToRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    if (data != nullptr) {
        for (const auto &v: *data) {
            *request.add_value_unordered() = buildValueProtoNoTtl(v, clientId);
        }
    }
    return SendAsyncRequest<hurricache::AddToRequest, hurricache::IntResponse, uint32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElementToTail,
        [](hurricache::IntResponse &resp) -> uint32_t { return static_cast<uint32_t>(resp.size()); });
}

std::future<uint32_t> FastCacheStandaloneClient::addElementToHead(const Key &key, const KeyHint *hint,
                                                                  const std::vector<ValuePtr> *data, int32_t clientId,
                                                                  std::chrono::milliseconds timeout) {
    hurricache::AddToRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    if (data != nullptr) {
        for (const auto &v: *data) {
            *request.add_value_unordered() = buildValueProtoNoTtl(v, clientId);
        }
    }
    return SendAsyncRequest<hurricache::AddToRequest, hurricache::IntResponse, uint32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElementToHead,
        [](hurricache::IntResponse &resp) -> uint32_t { return static_cast<uint32_t>(resp.size()); });
}

std::future<uint32_t> FastCacheStandaloneClient::addElementToPosition(const Key &key, const KeyHint *hint,
                                                                      const std::vector<ValuePtr> *data, uint32_t pos,
                                                                      int32_t clientId,
                                                                      std::chrono::milliseconds timeout) {
    hurricache::AddToRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_pos(static_cast<uint32_t>(pos));
    if (data != nullptr) {
        for (const auto &v: *data) {
            *request.add_value_unordered() = buildValueProtoNoTtl(v, clientId);
        }
    }
    return SendAsyncRequest<hurricache::AddToRequest, hurricache::IntResponse, uint32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElement,
        [](hurricache::IntResponse &resp) -> uint32_t { return static_cast<uint32_t>(resp.size()); });
}

std::future<uint32_t> FastCacheStandaloneClient::addElementToPositionBefore(const Key &key, const KeyHint *hint,
                                                                            const std::vector<ValuePtr> *data,
                                                                            ValuePtr pivot,
                                                                            int32_t clientId,
                                                                            std::chrono::milliseconds timeout) {
    if (pivot == nullptr) {
        std::promise<uint32_t> p;
        p.set_value(0);
        return p.get_future();
    }

    hurricache::AddToValRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_isbefore(true);
    *request.mutable_pos() = buildValueProtoNoTtl(pivot, clientId);
    if (data != nullptr) {
        for (const auto &v: *data) {
            *request.add_value() = buildValueProtoNoTtl(v, clientId);
        }
    }
    return SendAsyncRequest<hurricache::AddToValRequest, hurricache::IntResponse, uint32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElementToPositionByValue,
        [](hurricache::IntResponse &resp) -> uint32_t { return static_cast<uint32_t>(resp.size()); });
}

std::future<uint32_t> FastCacheStandaloneClient::addElementToPositionAfter(const Key &key, const KeyHint *hint,
                                                                           const std::vector<ValuePtr> *data,
                                                                           ValuePtr pivot,
                                                                           int32_t clientId,
                                                                           std::chrono::milliseconds timeout) {
    if (pivot == nullptr) {
        std::promise<uint32_t> p;
        p.set_value(0);
        return p.get_future();
    }

    hurricache::AddToValRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_isbefore(false);
    *request.mutable_pos() = buildValueProtoNoTtl(pivot, clientId);
    if (data != nullptr) {
        for (const auto &v: *data) {
            *request.add_value() = buildValueProtoNoTtl(v, clientId);
        }
    }
    return SendAsyncRequest<hurricache::AddToValRequest, hurricache::IntResponse, uint32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElementToPositionByValue,
        [](hurricache::IntResponse &resp) -> uint32_t { return static_cast<uint32_t>(resp.size()); });
}

// =========================================================================
// DELETION OPERATIONS
// =========================================================================

std::future<bool> FastCacheStandaloneClient::removeHead(const Key &key, const KeyHint *hint, int32_t clientId,
                                                        std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::BoolResponse, bool>(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncremoveHead,
        [](hurricache::BoolResponse &resp) -> bool { return resp.value(); });
}

std::future<bool> FastCacheStandaloneClient::removeTail(const Key &key, const KeyHint *hint, int32_t clientId,
                                                        std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::BoolResponse, bool>(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncremoveTail,
        [](hurricache::BoolResponse &resp) -> bool { return resp.value(); });
}

std::future<bool> FastCacheStandaloneClient::removeElementAtPosition(const Key &key, const KeyHint *hint, uint64_t pos,
                                                                     uint64_t endPos, int32_t clientId,
                                                                     std::chrono::milliseconds timeout) {
    hurricache::KeyPositionRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_pos(pos);
    if (endPos > pos) {
        request.set_end(endPos);
    }
    return SendAsyncRequest<hurricache::KeyPositionRequest, hurricache::BoolResponse, bool>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncremoveElementAtPosition,
        [](hurricache::BoolResponse &resp) -> bool { return resp.value(); });
}

std::future<uint32_t> FastCacheStandaloneClient::removeFromContainer(const Key &key, const KeyHint *hint,
                                                                     ContainerType type,
                                                                     const std::vector<KeyPtr> *keys,
                                                                     const std::vector<ValuePtr> *values,
                                                                     int32_t clientId,
                                                                     std::chrono::milliseconds timeout) {
    hurricache::RemoveFromContainerRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_type(static_cast<hurricache::ContainerType>(type));
    if (keys != nullptr) {
        for (const auto &k: *keys) {
            auto *pk = request.add_keys();
            pk->mutable_payload()->set_size(k->size);
            pk->mutable_payload()->mutable_payload()->assign(k->data, k->size);
        }
    }
    if (values != nullptr) {
        for (const auto &v: *values) {
            *request.add_values() = buildValueProtoNoTtl(v, clientId);
        }
    }

    return SendAsyncRequest<hurricache::RemoveFromContainerRequest, hurricache::IntResponse, uint32_t>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncremoveFromContainerByKeyValue,
        [](hurricache::IntResponse &resp) -> uint32_t { return static_cast<uint32_t>(resp.size()); });
}

// =========================================================================
// LOCKING OPERATIONS
// =========================================================================

std::future<LockStatus> FastCacheStandaloneClient::lockObject(const Key &key, const KeyHint *hint, LockType type,
                                                              int32_t clientId, std::chrono::milliseconds duration,
                                                              std::chrono::milliseconds timeout) {
    hurricache::LockRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_locktype(static_cast<hurricache::LockType>(type));
    request.set_clientid(clientId);
    if (duration.count() > 0) {
        auto now_s = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request.set_lockduration(static_cast<uint32_t>(now_s) + static_cast<uint32_t>(duration.count() / 1000));
    }
    return SendAsyncRequest<hurricache::LockRequest, hurricache::LockResponse, LockStatus>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsynclockObject,
        [](hurricache::LockResponse &resp) -> LockStatus {
            return static_cast<LockStatus>(resp.result());
        });
}

std::future<LockStatus> FastCacheStandaloneClient::unlockObject(const Key &key, const KeyHint *hint, int32_t clientId,
                                                                std::chrono::milliseconds timeout) {
    hurricache::UnLockRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_clientid(clientId);
    return SendAsyncRequest<hurricache::UnLockRequest, hurricache::UnlockResponse, LockStatus>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncunlockObject,
        [](hurricache::UnlockResponse &resp) -> LockStatus {
            return static_cast<LockStatus>(resp.result());
        });
}

// =========================================================================
// ATOMIC OPERATIONS
// =========================================================================


std::future<int64_t> FastCacheStandaloneClient::atomicLoad(const Key &key, const KeyHint *hint, int32_t clientId,
                                                           std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::AtomicValue, int64_t>(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicLoad,
        [](hurricache::AtomicValue &resp) -> int64_t { return resp.val(); });
}

std::future<int64_t> FastCacheStandaloneClient::atomicLoadAndDelete(const Key &key, const KeyHint *hint,
                                                                    int32_t clientId,
                                                                    std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::GetRequest, hurricache::AtomicValue, int64_t>(
        buildGetRequestProto(key, hint, clientId, defaultCompressionThreshold_), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicLoadAndDelete,
        [](hurricache::AtomicValue &resp) -> int64_t { return static_cast<int64_t>(resp.val()); });
}

std::future<KeyHint> FastCacheStandaloneClient::atomicCreate(const Key &key, const KeyHint *hint, int64_t value,
                                                             std::chrono::milliseconds ttl, int32_t clientId,
                                                             std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::KeyHintResponse, KeyHint>(
        buildAtomicCreateProto(key, hint, clientId, value, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicCreate,
        [](hurricache::KeyHintResponse &resp) -> KeyHint {
            const auto &kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<KeyHint> FastCacheStandaloneClient::atomicStore(const Key &key, const KeyHint *hint, int64_t value,
                                                            std::chrono::milliseconds ttl, int32_t clientId,
                                                            std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::KeyHintResponse, KeyHint>(
        buildAtomicCreateProto(key, hint, clientId, value, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicStore,
        [](hurricache::KeyHintResponse &resp) -> KeyHint {
            const auto &kh = resp.keyhint();
            return KeyHint{kh.has_week_hash() ? kh.week_hash() : 0, kh.has_strong_hash() ? kh.strong_hash() : 0};
        });
}

std::future<int64_t> FastCacheStandaloneClient::atomicExchange(const Key &key, const KeyHint *hint, int64_t value,
                                                               std::chrono::milliseconds ttl, int32_t clientId,
                                                               std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::AtomicValue, int64_t>(
        buildAtomicCreateProto(key, hint, clientId, value, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicExchange,
        [](hurricache::AtomicValue &resp) -> int64_t { return resp.val(); });
}

std::future<int64_t> FastCacheStandaloneClient::atomicAdd(const Key &key, const KeyHint *hint, int64_t delta,
                                                          std::chrono::milliseconds ttl, int32_t clientId,
                                                          std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::AtomicValue, int64_t>(
        buildAtomicCreateProto(key, hint, clientId, delta, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicAdd,
        [](hurricache::AtomicValue &resp) -> int64_t { return resp.val(); });
}

std::future<int64_t> FastCacheStandaloneClient::atomicSub(const Key &key, const KeyHint *hint, int64_t delta,
                                                          std::chrono::milliseconds ttl, int32_t clientId,
                                                          std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::AtomicValue, int64_t>(
        buildAtomicCreateProto(key, hint, clientId, delta, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicSub,
        [](hurricache::AtomicValue &resp) -> int64_t { return resp.val(); });
}

std::future<int64_t> FastCacheStandaloneClient::atomicAnd(const Key &key, const KeyHint *hint, int64_t mask,
                                                          std::chrono::milliseconds ttl, int32_t clientId,
                                                          std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::AtomicValue, int64_t>(
        buildAtomicCreateProto(key, hint, clientId, mask, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicAnd,
        [](hurricache::AtomicValue &resp) -> int64_t { return resp.val(); });
}

std::future<int64_t> FastCacheStandaloneClient::atomicOr(const Key &key, const KeyHint *hint, int64_t mask,
                                                         std::chrono::milliseconds ttl, int32_t clientId,
                                                         std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::AtomicValue, int64_t>(
        buildAtomicCreateProto(key, hint, clientId, mask, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicOr,
        [](hurricache::AtomicValue &resp) -> int64_t { return resp.val(); });
}

std::future<int64_t> FastCacheStandaloneClient::atomicXor(const Key &key, const KeyHint *hint, int64_t mask,
                                                          std::chrono::milliseconds ttl, int32_t clientId,
                                                          std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::AtomicCreate, hurricache::AtomicValue, int64_t>(
        buildAtomicCreateProto(key, hint, clientId, mask, ttl), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicXor,
        [](hurricache::AtomicValue &resp) -> int64_t { return resp.val(); });
}

std::future<AtomicCasRes> FastCacheStandaloneClient::atomicCompareAndSet(const Key &key, const KeyHint *hint,
                                                                         int64_t expectedValue, int64_t newValue,
                                                                         std::chrono::milliseconds ttl,
                                                                         int32_t clientId,
                                                                         std::chrono::milliseconds timeout) {
    hurricache::AtomicCas request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    auto *exp = request.mutable_expected();
    exp->set_val(expectedValue);
    auto *toset = request.mutable_toset();
    toset->set_val(newValue);
    if (ttl.count() > 0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request.set_ttl(static_cast<uint64_t>(now_ms) + static_cast<uint64_t>(ttl.count()));
    }
    return SendAsyncRequest<hurricache::AtomicCas, hurricache::AtomicCasRes, AtomicCasRes>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncatomicCompareAndSet,
        [](hurricache::AtomicCasRes &resp) -> AtomicCasRes {
            return AtomicCasRes{resp.result(), resp.expected().val()};
        });
}

// =========================================================================
// CONTAINER VALUE OPERATIONS
// =========================================================================


std::future<ValuePtr> FastCacheStandaloneClient::getContainerValue(const Key &key, const KeyHint *hint,
                                                                   const Key &elementKey,
                                                                   int32_t clientId,
                                                                   std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::ContainerGetRequest, hurricache::ValueResponse, ValuePtr>(
        buildContainerGetRequestProto(key, hint, clientId, elementKey), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetValueInContainer,
        [](hurricache::ValueResponse &resp) -> ValuePtr {
            if (!resp.has_value_unordered()) {
                return nullptr;
            }
            return valueRequestToValue(resp.value_unordered());
        });
}

std::future<ValuePtr> FastCacheStandaloneClient::getAndRemoveContainerValue(const Key &key, const KeyHint *hint,
                                                                            const Key &elementKey, int32_t clientId,
                                                                            std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::ContainerGetRequest, hurricache::ValueResponse, ValuePtr>(
        buildContainerGetRequestProto(key, hint, clientId, elementKey), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncgetAndDeleteValueInContainer,
        [](hurricache::ValueResponse &resp) -> ValuePtr {
            if (!resp.has_value_unordered()) {
                return nullptr;
            }
            return valueRequestToValue(resp.value_unordered());
        });
}

std::future<bool> FastCacheStandaloneClient::containsContainerKey(const Key &key, const KeyHint *hint,
                                                                  const Key &elementKey,
                                                                  int32_t clientId, std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::ContainerGetRequest, hurricache::BoolResponse, bool>(
        buildContainerGetRequestProto(key, hint, clientId, elementKey), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncexistKeyInContainer,
        [](hurricache::BoolResponse &resp) -> bool { return resp.value(); });
}

std::future<ValuePtr> FastCacheStandaloneClient::updateContainerValue(const Key &key, const KeyHint *hint,
                                                                      const Key &elementKey,
                                                                      const Value &value, int32_t clientId,
                                                                      std::chrono::milliseconds timeout) {
    hurricache::UpdateContainerRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    auto *ek = request.mutable_element_key();
    ek->mutable_payload()->set_size(elementKey.size);
    ek->mutable_payload()->mutable_payload()->assign(elementKey.data, elementKey.size);
    *request.mutable_value() = buildValueProtoNoTtl(value, clientId);
    return SendAsyncRequest<hurricache::UpdateContainerRequest, hurricache::UpdateValueResponse, ValuePtr>(
        request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncupdateValueInContainer,
        [](hurricache::UpdateValueResponse &resp) -> ValuePtr {
            if (!resp.has_value()) {
                return nullptr;
            }
            return valueRequestToValue(resp.value());
        });
}

std::future<uint32_t> FastCacheStandaloneClient::removeFromContainer(const Key &key, const KeyHint *hint,
                                                                     const Key &elementKey, int32_t clientId,
                                                                     std::chrono::milliseconds timeout) {
    return SendAsyncRequest<hurricache::ContainerGetRequest, hurricache::IntResponse, uint32_t>(
        buildContainerGetRequestProto(key, hint, clientId, elementKey), timeout,
        &hurricache::HurriCacheGrpcService::StubInterface::AsyncremoveInContainer,
        [](hurricache::IntResponse &resp) -> uint32_t { return static_cast<uint32_t>(resp.size()); });
}

std::future<uint32_t> FastCacheStandaloneClient::addElementHashMap(const Key &key, const KeyHint *hint,
                                                                   const std::vector<KeyPtr> *container_keys,
                                                                   const std::vector<ValuePtr> *container_values,
                                                                   int32_t clientId,
                                                                   std::chrono::milliseconds timeout) {
    hurricache::AddToRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_type(hurricache::ContainerType::MAP);
    if (container_keys != nullptr && container_values != nullptr && container_keys->size() == container_values->
        size()) {
        auto size = std::min(container_keys->size(), container_values->size());
        for (size_t i = 0; i < size; ++i) {
            auto *pk = request.add_key_unordered();
            pk->mutable_payload()->set_size(container_keys->at(i)->size);
            pk->mutable_payload()->mutable_payload()->assign(container_keys->at(i)->data, container_keys->at(i)->size);
            *request.add_value_unordered() = buildValueProtoNoTtl(container_values->at(i), clientId);
        }
        return SendAsyncRequest<hurricache::AddToRequest, hurricache::IntResponse, uint32_t>(
            request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElement,
            [](hurricache::IntResponse &resp) -> uint32_t { return static_cast<uint32_t>(resp.size()); });
    }

    std::promise<uint32_t> p;
    p.set_exception(std::make_exception_ptr(
        std::invalid_argument("container_keys and container_values must have same size")));
    return p.get_future();
}

std::future<uint32_t> FastCacheStandaloneClient::addElementOrderedMap(const Key &key, const KeyHint *hint,
                                                                      const std::vector<OrderedValuePtr> *
                                                                      container_keys,
                                                                      const std::vector<ValuePtr> *container_values,
                                                                      int32_t clientId,
                                                                      std::chrono::milliseconds timeout) {
    hurricache::AddToRequest request;

    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_type(hurricache::ContainerType::ORDERED_MAP);
    if (container_keys != nullptr && container_values != nullptr && container_keys->size() != container_values->
        size()) {
        auto size = std::min(container_keys->size(), container_values->size());
        for (size_t i = 0; i < size; ++i) {
            auto *pk = request.add_key_ordered();
            pk->mutable_payload()->set_size(container_keys->at(i)->size);
            pk->mutable_payload()->mutable_payload()->assign(container_keys->at(i)->data, container_keys->at(i)->size);
            pk->set_order(container_keys->at(i)->weight);
            *request.add_value_unordered() = buildValueProtoNoTtl(container_values->at(i), clientId);
        }
        return SendAsyncRequest<hurricache::AddToRequest, hurricache::IntResponse, uint32_t>(
            request, timeout, &hurricache::HurriCacheGrpcService::StubInterface::AsyncaddElement,
            [](hurricache::IntResponse &resp) -> uint32_t { return static_cast<uint32_t>(resp.size()); });
    }

    std::promise<uint32_t> p;
    p.set_exception(std::make_exception_ptr(
        std::invalid_argument("container_keys and container_values must have same size")));
    return p.get_future();
}
