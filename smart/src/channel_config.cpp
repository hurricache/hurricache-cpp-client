#include <channel_config.hxx>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

std::shared_ptr<FastCacheStandaloneClient> createSimpleClient(
    const std::string& target,
    const ChannelConfig& config,
    std::chrono::milliseconds timeout,
    int32_t defaultCompressionThreshold) {

    grpc::ChannelArguments args;
    args.SetMaxReceiveMessageSize(config.max_inbound_message_size);

    auto channel = grpc::CreateCustomChannel(target, grpc::InsecureChannelCredentials(), args);
    return std::make_shared<FastCacheStandaloneClient>(
        channel, target, timeout, defaultCompressionThreshold);
}
