//
// Created by alex on 22.09.2026.
//

#ifndef HURRICACHE_CPP_CLIENT_UTILS_HXX
#define HURRICACHE_CPP_CLIENT_UTILS_HXX
#include "types.hxx"
#include <cache.grpc.pb.h>
#include <future>
#include <chrono>

::hurricache::Key buildKeyProto(const Key& key, const KeyHint& hint, int32_t clientId);
::hurricache::GetRequest buildGetRequestProto(const Key& key, const KeyHint& hint, int32_t clientId);
::hurricache::Value buildValueProto(const Value& value, std::chrono::milliseconds ttl, int32_t clientId);
inline ::hurricache::Value buildValueProtoNoTtl(const Value& value, int32_t clientId) {
    return buildValueProto(value, std::chrono::milliseconds{0}, clientId);
}
::hurricache::AtomicCreate buildAtomicCreateProto(const Key& key, const KeyHint& hint, int32_t clientId,
                                                   int64_t value, std::chrono::milliseconds ttl);
::hurricache::ContainerGetRequest buildContainerGetRequestProto(const Key& key, const KeyHint& hint, int32_t clientId,
                                                                 const Key& elementKey);
::hurricache::KeyPositionRequest buildPositionRequestProto(const Key& key, const KeyHint& hint, int32_t clientId, int32_t pos);

Key *keyRequestToKey(const ::hurricache::Key &request,KeyHint* hint=nullptr);
OrderedKey *keyRequestToKey(const ::hurricache::OrderedKey &request,KeyHint* hint=nullptr);

OrderedValue *valueRequestToOrderedValue(const ::hurricache::OrderedValue &request);
Value *valueRequestToValue(const ::hurricache::Value &request);




struct RpcCallDataBase {
    virtual ~RpcCallDataBase() = default;
    virtual void Proceed(bool ok) = 0;
};

template<typename ResponseType, typename ResultType>
struct RpcCallData : public RpcCallDataBase{
    std::promise<ResultType> promise;
    ResponseType response;
    grpc::ClientContext context;
    grpc::Status status;

    std::function<ResultType(ResponseType &)> transformer;

    void Proceed(bool ok) {
        if (ok && status.ok()) {
            try {
                if constexpr (std::is_same_v<ResultType, ResponseType>) {
                    promise.set_value(std::move(response));
                } else if (transformer) {
                    promise.set_value(transformer(response));
                } else {
                    promise.set_exception(std::make_exception_ptr(
                        std::runtime_error("Transformer not set for non-matching types")));
                }
            } catch (...) {
                promise.set_exception(std::current_exception());
            }
        } else {
            std::string err_msg = "gRPC call failed. Code: " + std::to_string(status.error_code()) +
                                  ", Message: " + status.error_message();
            promise.set_exception(std::make_exception_ptr(std::runtime_error(err_msg)));
        }
        delete this;
    }
};


template <typename ResponseChunkType, typename ResultType>
struct StreamCallData : public RpcCallDataBase {
    std::promise<ResultType> promise;
    grpc::ClientContext context;
    grpc::Status status;
    std::unique_ptr<grpc::ClientAsyncReaderInterface<ResponseChunkType>> reader;

    ResponseChunkType current_chunk;
    ResultType accumulated_result;

    enum class State { READING, FINISHING };
    State state = State::READING;

    std::function<void(ResultType&, ResponseChunkType&)> chunk_accumulator;

    void Proceed(bool ok) override {
        if (state == State::READING) {
            if (ok) {
                if (chunk_accumulator) {
                    chunk_accumulator(accumulated_result, current_chunk);
                }
                reader->Read(&current_chunk, this);
            } else {
                state = State::FINISHING;
                reader->Finish(&status, this);
            }
        } else if (state == State::FINISHING) {
            if (status.ok()) {
                promise.set_value(std::move(accumulated_result));
            } else {
                std::string err_msg = "gRPC stream failed. Code: " + std::to_string(status.error_code()) +
                                      ", Message: " + status.error_message();
                promise.set_exception(std::make_exception_ptr(std::runtime_error(err_msg)));
            }
            delete this;
        }
    }
};


#endif //HURRICACHE_CPP_CLIENT_UTILS_HXX
