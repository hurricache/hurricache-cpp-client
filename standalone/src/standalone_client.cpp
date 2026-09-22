#include <standalone_client.hxx>

#include "utils.hxx"
#include "grpcpp/security/credentials.h"




// Основной конструктор с хостом и портом
FastCacheStandaloneClient::FastCacheStandaloneClient(
    const std::string& host, int32_t port, int32_t defaultClientId,
    std::chrono::milliseconds timeout, int32_t defaultCompressionThreshold)
    : defaultClientId_(defaultClientId),
      defaultTimeout_(timeout),
      defaultCompressionThreshold_(defaultCompressionThreshold),
      target_(host + ":" + std::to_string(port)) 
{
    grpc::ChannelArguments args;

    channel_ = grpc::CreateCustomChannel(target_, grpc::InsecureChannelCredentials(), args);
    asyncStub_ = hurricache::HurriCacheGrpcService::NewStub(channel_);
    completion_queue_thread_ = std::jthread([this] { RunCompletionQueue(); });
}

// Делегирующие конструкторы
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
    std::shared_ptr<grpc::Channel> channel,std::string target, int32_t defaultClientId,
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

// Геттеры и служебные методы
std::string FastCacheStandaloneClient::toString() const {
    return "FastCacheStandaloneClient{target='" + target_ + "'}";
}

std::string FastCacheStandaloneClient::getTarget() const { return target_; }
int32_t FastCacheStandaloneClient::getDefaultClientId() const { return defaultClientId_; }
std::chrono::milliseconds FastCacheStandaloneClient::getDefaultTimeout() const { return defaultTimeout_; }
int32_t FastCacheStandaloneClient::getDefaultCompressionThreshold() const { return defaultCompressionThreshold_; }

void FastCacheStandaloneClient::shutdown() {
    cq_.Shutdown();
    asyncStub_.reset();
    channel_.reset();
}

void FastCacheStandaloneClient::RunCompletionQueue() {
    void* tag = nullptr;
    bool ok = false;
    // Пока очередь активна, ждем завершенные RPC
    while (cq_.Next(&tag, &ok)) {
        // Уникальный тег — это наш базовый класс-обертка для запроса
        auto* call_data = static_cast<RpcCallDataBase*>(tag);
        call_data->Proceed(ok); // Вызывается обработчик конкретного запроса
    }
}