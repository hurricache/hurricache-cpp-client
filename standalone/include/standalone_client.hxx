#pragma once

#include <cache.grpc.pb.h>
#include <future>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <thread>

#include "types.hxx"

// Expected types/structures (from project context)


class FastCacheStandaloneClient {
public:
    // Constructors
    FastCacheStandaloneClient(const std::string &host, int32_t port, std::chrono::milliseconds timeout,
                              int32_t defaultCompressionThreshold = 1024);

    FastCacheStandaloneClient(const std::string &host, int32_t port, std::chrono::milliseconds timeout);

    FastCacheStandaloneClient(const std::string &host, int32_t port);

    FastCacheStandaloneClient(std::shared_ptr<grpc::Channel> channel, std::string target,
                              std::chrono::milliseconds duration, int32_t defaultCompressionThreshold = 1024);


    // Getters and metadata
    [[nodiscard]] std::string toString() const;

    [[nodiscard]] std::string getTarget() const;



    [[nodiscard]] std::chrono::milliseconds getDefaultTimeout() const;

    [[nodiscard]] int32_t getDefaultCompressionThreshold() const;


    // =========================================================================
    // TTL MANAGEMENT
    // =========================================================================
    [[nodiscard]] std::future<bool> setTtl(const Key &key, const KeyHint *hint, uint64_t ttl,
                                           int32_t clientId = 0,
                                           std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int64_t> getTtl(const Key &key, const KeyHint *hint = nullptr,
                                              int32_t clientId = 0,
                                              std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // KEY-VALUE OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<ValuePtr> getAndDeleteValue(const Key &key, const KeyHint *hint = nullptr,
                                                          int32_t clientId = 0,
                                                          std::chrono::milliseconds timeout = std::chrono::milliseconds(
                                                              0));

    [[nodiscard]] std::future<KeyHint> createKeyValue(const Key &key, const KeyHint *hint, const Value &value,
                                                      std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                      int32_t clientId = 0,
                                                      std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getValue(const Key &key, const KeyHint *hint = nullptr,
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> updateKeyValue(const Key &key, const KeyHint *hint, const Value &value,
                                                       std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                       int32_t clientId = 0,
                                                       std::chrono::milliseconds timeout = std::chrono::milliseconds(
                                                           0));

    [[nodiscard]] std::future<bool> existKey(const Key &key, const KeyHint *hint = nullptr,
                                             int32_t clientId = 0,
                                             std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<bool> remove(const Key &key, const KeyHint *hint = nullptr,
                                           int32_t clientId = 0,
                                           std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // CONTAINER CREATION (UNORDERED & ORDERED)
    // =========================================================================
    [[nodiscard]] std::future<KeyHint> createQueue(const Key &key, const KeyHint *keyHint = nullptr,
                                                   const std::vector<ValuePtr> *initialValue = nullptr,
                                                   std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                   int32_t clientId = 0,
                                                   std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<KeyHint> createList(const Key &key, const KeyHint *keyHint = nullptr,
                                                  const std::vector<ValuePtr> *initialValue = nullptr,
                                                  std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                  int32_t clientId = 0,
                                                  std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<KeyHint> createVector(const Key &key, const KeyHint *keyHint = nullptr,
                                                    const std::vector<ValuePtr> *initialValue = nullptr,
                                                    std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                    int32_t clientId = 0,
                                                    std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<KeyHint> createSet(const Key &key, const KeyHint *keyHint = nullptr,
                                                 const std::vector<ValuePtr> *initialValue = nullptr,
                                                 std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<KeyHint> createOrderedSet(const Key &key, const KeyHint *keyHint = nullptr,
                                                        const std::vector<OrderedValuePtr> *initialValue = nullptr,
                                                        std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                        int32_t clientId = 0,
                                                        std::chrono::milliseconds timeout =
                                                                std::chrono::milliseconds(0));

    [[nodiscard]] std::future<KeyHint> createMap(const Key &key, const KeyHint *keyHint = nullptr,
                                                 const std::map<KeyPtr, ValuePtr> *initialValue = nullptr,
                                                 std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<KeyHint> createOrderedMap(const Key &key, const KeyHint *keyHint = nullptr,
                                                        const std::map<OrderedKeyPtr, OrderedValuePtr> *initialValue =
                                                                nullptr,
                                                        std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                        int32_t clientId = 0,
                                                        std::chrono::milliseconds timeout =
                                                                std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getElementWithWeight(const Key &key, const KeyHint *hint, uint64_t pos,
                                                             int32_t clientId = 0,
                                                             std::chrono::milliseconds timeout =
                                                                     std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getAndRemoveElementWithWeight(const Key &key, const KeyHint *hint, uint64_t pos,
                                                                      int32_t clientId = 0,
                                                                      std::chrono::milliseconds timeout =
                                                                              std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> getSize(const Key &key, const KeyHint *hint = nullptr,
                                               int32_t clientId = 0,
                                               std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getAndRemoveFront(const Key &key, const KeyHint *hint = nullptr,
                                                          int32_t clientId = 0,
                                                          std::chrono::milliseconds timeout = std::chrono::milliseconds(
                                                              0));

    [[nodiscard]] std::future<ValuePtr> getHead(const Key &key, const KeyHint *hint = nullptr,
                                                int32_t clientId = 0,
                                                std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getTail(const Key &key, const KeyHint *hint = nullptr,
                                                int32_t clientId = 0,
                                                std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getElementAtPosition(const Key &key, const KeyHint *hint, uint64_t pos,
                                                             int32_t clientId = 0,
                                                             std::chrono::milliseconds timeout =
                                                                     std::chrono::milliseconds(0));

    // =========================================================================
    // STREAMING READ OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<std::vector<ValuePtr> > streamList(const Key &key, const KeyHint *hint = nullptr,
                                                                 int32_t clientId = 0,
                                                                 std::chrono::milliseconds timeout =
                                                                         std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::vector<ValuePtr> > streamVector(const Key &key, const KeyHint *hint = nullptr,
                                                                   int32_t clientId = 0,
                                                                   std::chrono::milliseconds timeout =
                                                                           std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::vector<ValuePtr> > streamSet(const Key &key, const KeyHint *hint = nullptr,
                                                                int32_t clientId = 0,
                                                                std::chrono::milliseconds timeout =
                                                                        std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::map<KeyPtr, ValuePtr> > streamMap(const Key &key, const KeyHint *hint = nullptr,
                                                                     int32_t clientId = 0,
                                                                     std::chrono::milliseconds timeout =
                                                                             std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::vector<OrderedValuePtr> > streamOrderedSet(
        const Key &key, const KeyHint *hint = nullptr, int32_t clientId = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::vector<std::pair<OrderedKeyPtr, ValuePtr> > > streamOrderedMap(
        const Key &key, const KeyHint *hint = nullptr, int32_t clientId = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::vector<ValuePtr> > streamElementInRangeUnordered(
        const Key &key, const KeyHint *hint, ContainerType containerType,
        uint64_t start, uint64_t end, int32_t clientId = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::vector<OrderedValuePtr> > streamElementInRangeOrderedSet(
        const Key &key, const KeyHint *hint, uint64_t startWeight,
        uint64_t endWeight, bool reverse, int32_t clientId = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::vector<std::pair<OrderedKeyPtr, ValuePtr> > > streamElementInRangeOrderedMap(
        const Key &key, const KeyHint *hint, uint64_t startWeight,
        uint64_t endWeight, bool reverse, int32_t clientId = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // INSERTION OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<uint32_t> addElementUnordered(const Key &key, const KeyHint *hint = nullptr,
                                                           const std::vector<ValuePtr> *data = nullptr,
                                                           int32_t clientId = 0,
                                                           std::chrono::milliseconds timeout =
                                                                   std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementWithWeight(const Key &key, const KeyHint *hint = nullptr,
                                                            const std::vector<OrderedValuePtr> *data = nullptr,
                                                            int32_t clientId = 0,
                                                            std::chrono::milliseconds timeout =
                                                                    std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementToTail(const Key &key, const KeyHint *hint = nullptr,
                                                        const std::vector<ValuePtr> *data = nullptr,
                                                        int32_t clientId = 0,
                                                        std::chrono::milliseconds timeout =
                                                                std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementToHead(const Key &key, const KeyHint *hint = nullptr,
                                                        const std::vector<ValuePtr> *data = nullptr,
                                                        int32_t clientId = 0,
                                                        std::chrono::milliseconds timeout =
                                                                std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementToPosition(const Key &key, const KeyHint *hint,
                                                            const std::vector<ValuePtr> *data, uint32_t pos,
                                                            int32_t clientId = 0,
                                                            std::chrono::milliseconds timeout =
                                                                    std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementToPositionBefore(const Key &key, const KeyHint *hint = nullptr,
                                                                  const std::vector<ValuePtr> *data = nullptr,
                                                                  ValuePtr pivot = nullptr,
                                                                  int32_t clientId = 0,
                                                                  std::chrono::milliseconds timeout =
                                                                          std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementToPositionAfter(const Key &key, const KeyHint *hint = nullptr,
                                                                 const std::vector<ValuePtr> *data = nullptr,
                                                                 ValuePtr pivot = nullptr,
                                                                 int32_t clientId = 0,
                                                                 std::chrono::milliseconds timeout =
                                                                         std::chrono::milliseconds(0));

    // =========================================================================
    // POP & DELETION OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<ValuePtr> getAndRemoveTail(const Key &key, const KeyHint *hint = nullptr,
                                                         int32_t clientId = 0,
                                                         std::chrono::milliseconds timeout = std::chrono::milliseconds(
                                                             0));

    [[nodiscard]] std::future<ValuePtr> getAndRemoveElementAtPosition(const Key &key, const KeyHint *hint, uint64_t pos,
                                                                      int32_t clientId = 0,
                                                                      std::chrono::milliseconds timeout =
                                                                              std::chrono::milliseconds(0));

    [[nodiscard]] std::future<bool> removeTail(const Key &key, const KeyHint *hint,
                                               int32_t clientId = 0,
                                               std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<bool> removeHead(const Key &key, const KeyHint *hint,
                                               int32_t clientId = 0,
                                               std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<bool> removeElementAtPosition(const Key &key, const KeyHint *hint, uint64_t pos,
                                                            uint64_t endPos, int32_t clientId = 0,
                                                            std::chrono::milliseconds timeout =
                                                                    std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> removeFromContainer(const Key &key, const KeyHint *hint, ContainerType type,
                                                           const std::vector<KeyPtr> *keys,
                                                           const std::vector<ValuePtr> *values,
                                                           int32_t clientId = 0,
                                                           std::chrono::milliseconds timeout =
                                                                   std::chrono::milliseconds(0));

    // =========================================================================
    // LOCKING OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<LockStatus> lockObject(const Key &key, const KeyHint *hint, LockType type,
                                                     int32_t clientId = 0,
                                                     std::chrono::milliseconds duration = std::chrono::milliseconds(
                                                         60000),
                                                     std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<LockStatus> unlockObject(const Key &key, const KeyHint *hint = nullptr,
                                                       int32_t clientId = 0,
                                                       std::chrono::milliseconds timeout = std::chrono::milliseconds(
                                                           0));

    // =========================================================================
    // ATOMIC OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<int64_t> atomicLoad(const Key &key, const KeyHint *hint = nullptr,
                                                  int32_t clientId = 0,
                                                  std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int64_t> atomicLoadAndDelete(const Key &key, const KeyHint *hint= nullptr,
                                                           int32_t clientId = 0,
                                                           std::chrono::milliseconds timeout =
                                                                   std::chrono::milliseconds(0));

    [[nodiscard]] std::future<KeyHint> atomicCreate(const Key &key, const KeyHint *hint, int64_t value,
                                                    std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                    int32_t clientId = 0,
                                                    std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<KeyHint> atomicStore(const Key &key, const KeyHint *hint, int64_t value,
                                                   std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                   int32_t clientId = 0,
                                                   std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int64_t> atomicExchange(const Key &key, const KeyHint *hint, int64_t value,
                                                      std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                      int32_t clientId = 0,
                                                      std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int64_t> atomicAdd(const Key &key, const KeyHint *hint, int64_t delta,
                                                 std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int64_t> atomicSub(const Key &key, const KeyHint *hint, int64_t delta,
                                                 std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int64_t> atomicAnd(const Key &key, const KeyHint *hint, int64_t mask,
                                                 std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int64_t> atomicOr(const Key &key, const KeyHint *hint, int64_t mask,
                                                std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                int32_t clientId = 0,
                                                std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int64_t> atomicXor(const Key &key, const KeyHint *hint, int64_t mask,
                                                 std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<AtomicCasRes> atomicCompareAndSet(const Key &key, const KeyHint *hint,
                                                                int64_t expectedValue, int64_t newValue,
                                                                std::chrono::milliseconds ttl =
                                                                        std::chrono::milliseconds(0),
                                                                int32_t clientId = 0,
                                                                std::chrono::milliseconds timeout =
                                                                        std::chrono::milliseconds(0));

    // =========================================================================
    // CONTAINER VALUE OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<ValuePtr> getContainerValue(const Key &key, const KeyHint *hint, const Key &elementKey,
                                                          int32_t clientId = 0,
                                                          std::chrono::milliseconds timeout = std::chrono::milliseconds(
                                                              0));

    [[nodiscard]] std::future<ValuePtr> getAndRemoveContainerValue(const Key &key, const KeyHint *hint,
                                                                   const Key &elementKey,
                                                                   int32_t clientId = 0,
                                                                   std::chrono::milliseconds timeout =
                                                                           std::chrono::milliseconds(0));

    [[nodiscard]] std::future<bool> containsContainerKey(const Key &key, const KeyHint *hint, const Key &elementKey,
                                                         int32_t clientId = 0,
                                                         std::chrono::milliseconds timeout = std::chrono::milliseconds(
                                                             0));

    [[nodiscard]] std::future<ValuePtr> updateContainerValue(const Key &key, const KeyHint *hint, const Key &elementKey,
                                                             const Value &value,
                                                             int32_t clientId = 0,
                                                             std::chrono::milliseconds timeout =
                                                                     std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> removeFromContainer(const Key &key, const KeyHint *hint, const Key &elementKey,
                                                           int32_t clientId = 0,
                                                           std::chrono::milliseconds timeout =
                                                                   std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementHashMap(const Key &key, const KeyHint *hint,
                                                         const std::vector<KeyPtr> *container_keys = nullptr,
                                                         const std::vector<ValuePtr> *container_values = nullptr,
                                                         int32_t clientId = 0,
                                                         std::chrono::milliseconds timeout = std::chrono::milliseconds(
                                                             0));

    [[nodiscard]] std::future<uint32_t> addElementOrderedMap(const Key &key, const KeyHint *hint,
                                                            const std::vector<OrderedValuePtr> *container_keys =
                                                                    nullptr,
                                                            const std::vector<ValuePtr> *container_values = nullptr,
                                                            int32_t clientId = 0,
                                                            std::chrono::milliseconds timeout =
                                                                    std::chrono::milliseconds(0));

    // =========================================================================
    // LIFECYCLE
    // =========================================================================
    void shutdown();

    [[nodiscard]] bool isShutdown() const { return shutdown_called_.load(); }

    ~FastCacheStandaloneClient();

    [[nodiscard]] hurricache::HurriCacheGrpcService::StubInterface* getAsyncStub() const {
        return asyncStub_.get();
    }

    [[nodiscard]] grpc::CompletionQueue& getCompletionQueue() {
        return const_cast<grpc::CompletionQueue&>(cq_); // или не константная ссылка
    }
private:
    // Class members
    std::unique_ptr<hurricache::HurriCacheGrpcService::Stub> asyncStub_;
    std::shared_ptr<grpc::Channel> channel_;

    std::chrono::milliseconds defaultTimeout_;
    int32_t defaultCompressionThreshold_;
    std::string target_;
    grpc::CompletionQueue cq_;
    std::jthread completion_queue_thread_;
    std::atomic<bool> shutdown_called_{false};
    void RunCompletionQueue();

    static constexpr int32_t kDefaultCompressionThreshold = 64 * 1024; // 64KB default

};
