//
// Created by alex on 22.09.2026.
//

#ifndef HURRICACHE_CPP_CLIENT_UTILS_HXX
#define HURRICACHE_CPP_CLIENT_UTILS_HXX
#include "types.hxx"
#include <cache.grpc.pb.h>
#include <future>

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
    std::promise<ResultType> promise; // Обещание, которое резолвит будущее (std::future) для пользователя
    ResponseType response; // Сюда gRPC запишет ответ от сервера, когда он придет
    grpc::ClientContext context; // Контекст gRPC (включая таймауты, дедлайны и метаданные)
    grpc::Status status; // Статус выполнения запроса (успех/ошибка gRPC)

    // Функция-трансформер для распаковки или конвертации gRPC-ответа в нужный тип
    std::function<ResultType(ResponseType &)> transformer;

    // Этот метод вызывается фоновым потоком, когда событие вытаскивается из cq_.Next()
    void Proceed(bool ok) {
        if (ok && status.ok()) {
            try {
                if (transformer) {
                    promise.set_value(transformer(response));
                } else {
                    // Если трансформатор не задан (когда ResultType == ResponseType)
                    promise.set_value(std::move(response));
                }
            } catch (...) {
                promise.set_exception(std::current_exception());
            }
        } else {
            std::string err_msg = "gRPC call failed. Code: " + std::to_string(status.error_code()) +
                                  ", Message: " + status.error_message();
            promise.set_exception(std::make_exception_ptr(std::runtime_error(err_msg)));
        }
        delete this; // Самоуничтожение объекта после завершения работы с запросом
    }
};


template <typename ResponseChunkType, typename ResultType>
struct StreamCallData : public RpcCallDataBase {
    std::promise<ResultType> promise;
    grpc::ClientContext context;
    grpc::Status status;
    std::unique_ptr<grpc::ClientAsyncReader<ResponseChunkType>> reader;

    ResponseChunkType current_chunk;
    ResultType accumulated_result; // Например, std::vector<Payload>

    // Состояние автомата для стрима
    enum class State { READING, FINISHING };
    State state = State::READING;

    // Функция для конвертации/добавления чанка в итоговый результат
    std::function<void(ResultType&, ResponseChunkType&)> chunk_accumulator;

    void Proceed(bool ok) override {
        if (state == State::READING) {
            if (ok) {
                // Успешно прочитали очередную порцию данных из стрима
                if (chunk_accumulator) {
                    chunk_accumulator(accumulated_result, current_chunk);
                }
                // Запрашиваем чтение следующего элемента стрима
                reader->Read(&current_chunk, this);
            } else {
                // ok == false означает, что сервер закончил отправку данных (EOF)
                state = State::FINISHING;
                reader->Finish(&status, this);
            }
        } else if (state == State::FINISHING) {
            // Стрим полностью завершен, проверяем статус gRPC
            if (status.ok()) {
                promise.set_value(std::move(accumulated_result));
            } else {
                std::string err_msg = "gRPC stream failed. Code: " + std::to_string(status.error_code()) +
                                      ", Message: " + status.error_message();
                promise.set_exception(std::make_exception_ptr(std::runtime_error(err_msg)));
            }
            delete this; // Самоуничтожение объекта
        }
    }
};



#endif //HURRICACHE_CPP_CLIENT_UTILS_HXX
