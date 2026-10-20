#pragma once

#include <future>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <memory>
#include <atomic>
#include <thread>

#include "types.hxx"
#include "standalone_client.hxx"
#include "coordinator_client.hxx"
#include "routing_table.hxx"
#include "channel_config.hxx"

class FastCacheSmartClient {
public:
    // Constructors
    FastCacheSmartClient(std::vector<std::string> coordinators,
                        std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                        int32_t client_id = 0,
                        std::chrono::milliseconds timeout = std::chrono::milliseconds(0),
                        int32_t defaultCompressionThreshold = 1024);

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
                                                          std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

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
                                                       std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

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
                                                        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<KeyHint> createMap(const Key &key, const KeyHint *keyHint = nullptr,
                                                 const std::map<KeyPtr, ValuePtr> *initialValue = nullptr,
                                                 std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<KeyHint> createOrderedMap(const Key &key, const KeyHint *keyHint = nullptr,
                                                        const std::map<OrderedKeyPtr, OrderedValuePtr> *initialValue = nullptr,
                                                        std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                        int32_t clientId = 0,
                                                        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getElementWithWeight(const Key &key, const KeyHint *hint, uint64_t pos,
                                                             int32_t clientId = 0,
                                                             std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getAndRemoveElementWithWeight(const Key &key, const KeyHint *hint, uint64_t pos,
                                                                      int32_t clientId = 0,
                                                                      std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> getSize(const Key &key, const KeyHint *hint = nullptr,
                                               int32_t clientId = 0,
                                               std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getAndRemoveFront(const Key &key, const KeyHint *hint = nullptr,
                                                          int32_t clientId = 0,
                                                          std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getHead(const Key &key, const KeyHint *hint = nullptr,
                                                int32_t clientId = 0,
                                                std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getTail(const Key &key, const KeyHint *hint = nullptr,
                                                int32_t clientId = 0,
                                                std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getElementAtPosition(const Key &key, const KeyHint *hint, uint64_t pos,
                                                             int32_t clientId = 0,
                                                             std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // STREAMING READ OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<std::vector<ValuePtr> > streamList(const Key &key, const KeyHint *hint = nullptr,
                                                                 int32_t clientId = 0,
                                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::vector<ValuePtr> > streamVector(const Key &key, const KeyHint *hint = nullptr,
                                                                   int32_t clientId = 0,
                                                                   std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::vector<ValuePtr> > streamSet(const Key &key, const KeyHint *hint = nullptr,
                                                                int32_t clientId = 0,
                                                                std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::map<KeyPtr, ValuePtr> > streamMap(const Key &key, const KeyHint *hint = nullptr,
                                                                     int32_t clientId = 0,
                                                                     std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

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
                                                           std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementWithWeight(const Key &key, const KeyHint *hint = nullptr,
                                                            const std::vector<OrderedValuePtr> *data = nullptr,
                                                            int32_t clientId = 0,
                                                            std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementToTail(const Key &key, const KeyHint *hint = nullptr,
                                                        const std::vector<ValuePtr> *data = nullptr,
                                                        int32_t clientId = 0,
                                                        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementToHead(const Key &key, const KeyHint *hint = nullptr,
                                                        const std::vector<ValuePtr> *data = nullptr,
                                                        int32_t clientId = 0,
                                                        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementToPosition(const Key &key, const KeyHint *hint,
                                                            const std::vector<ValuePtr> *data, uint32_t pos,
                                                            int32_t clientId = 0,
                                                            std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementToPositionBefore(const Key &key, const KeyHint *hint = nullptr,
                                                                  const std::vector<ValuePtr> *data = nullptr,
                                                                  ValuePtr pivot = nullptr,
                                                                  int32_t clientId = 0,
                                                                  std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementToPositionAfter(const Key &key, const KeyHint *hint = nullptr,
                                                                 const std::vector<ValuePtr> *data = nullptr,
                                                                 ValuePtr pivot = nullptr,
                                                                 int32_t clientId = 0,
                                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // POP & DELETION OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<ValuePtr> getAndRemoveTail(const Key &key, const KeyHint *hint = nullptr,
                                                         int32_t clientId = 0,
                                                         std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getAndRemoveElementAtPosition(const Key &key, const KeyHint *hint, uint64_t pos,
                                                                      int32_t clientId = 0,
                                                                      std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> removeTail(const Key &key, const KeyHint *hint,
                                               int32_t clientId = 0,
                                               std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> removeHead(const Key &key, const KeyHint *hint,
                                               int32_t clientId = 0,
                                               std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> removeElementAtPosition(const Key &key, const KeyHint *hint, uint64_t pos,
                                                            uint64_t endPos, int32_t clientId = 0,
                                                            std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // LOCKING OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<LockStatus> lockObject(const Key &key, const KeyHint *hint, LockType type,
                                                     int32_t clientId = 0,
                                                     std::chrono::milliseconds duration = std::chrono::milliseconds(60000),
                                                     std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<LockStatus> unlockObject(const Key &key, const KeyHint *hint = nullptr,
                                                       int32_t clientId = 0,
                                                       std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // ATOMIC OPERATIONS
    // =========================================================================

    [[nodiscard]] std::future<ValuePtr> atomicExchange(const Key &key, const KeyHint *hint, int64_t value,
                                                      std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                      int32_t clientId = 0,
                                                      std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> atomicAdd(const Key &key, const KeyHint *hint, int64_t delta,
                                                 std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> atomicSub(const Key &key, const KeyHint *hint, int64_t delta,
                                                 std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> atomicAdd(const Key &key, const KeyHint *hint, double delta,
                                                 std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> atomicSub(const Key &key, const KeyHint *hint, double delta,
                                                 std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));


    [[nodiscard]] std::future<ValuePtr> atomicAnd(const Key &key, const KeyHint *hint, int64_t mask,
                                                 std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> atomicOr(const Key &key, const KeyHint *hint, int64_t mask,
                                                std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                int32_t clientId = 0,
                                                std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> atomicXor(const Key &key, const KeyHint *hint, int64_t mask,
                                                 std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<AtomicCasRes> atomicCompareAndSet(const Key &key, const KeyHint *hint,
                                                                int64_t expectedValue, int64_t newValue,
                                                                std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                                int32_t clientId = 0,
                                                                std::chrono::milliseconds timeout =
                                                                        std::chrono::milliseconds(0));

    // =========================================================================
    // CONTAINER VALUE OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<ValuePtr> getContainerValue(const Key &key, const KeyHint *hint, const Key &elementKey,
                                                          int32_t clientId = 0,
                                                          std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getAndRemoveContainerValue(const Key &key, const KeyHint *hint,
                                                                   const Key &elementKey,
                                                                   int32_t clientId = 0,
                                                                   std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<bool> containsContainerKey(const Key &key, const KeyHint *hint, const Key &elementKey,
                                                         int32_t clientId = 0,
                                                         std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> updateContainerValue(const Key &key, const KeyHint *hint, const Key &elementKey,
                                                             const Value &value,
                                                             int32_t clientId = 0,
                                                             std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> removeFromContainer(const Key &key, const KeyHint *hint, const Key &elementKey,
                                                           int32_t clientId = 0,
                                                           std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementHashMap(const Key &key, const KeyHint *hint,
                                                         const std::vector<KeyPtr> *container_keys = nullptr,
                                                         const std::vector<ValuePtr> *container_values = nullptr,
                                                         int32_t clientId = 0,
                                                         std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<uint32_t> addElementOrderedMap(const Key &key, const KeyHint *hint,
                                                            const std::vector<OrderedValuePtr> *container_keys = nullptr,
                                                            const std::vector<ValuePtr> *container_values = nullptr,
                                                            int32_t clientId = 0,
                                                            std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // LIFECYCLE
    // =========================================================================
    void shutdown();

    ~FastCacheSmartClient();

private:
    // Core routing method
    template<typename T>
    std::future<T> execute(
        const KeyHint *hint,
        std::function<std::future<T>(std::shared_ptr<FastCacheStandaloneClient>)> action);

    // Failover
    template<typename T>
    std::future<T> failover(
        std::future<T> future,
        std::shared_ptr<FastCacheStandaloneClient> primary,
        std::shared_ptr<FastCacheStandaloneClient> fallback);

    // Coordinator refresh
    void refreshRouting();

    std::vector<std::string> coordinator_addresses_;
    std::unique_ptr<CoordinatorClient> coordinator_client_;
    std::unique_ptr<RoutingTable> routing_table_;
    std::chrono::milliseconds timeout_;
    int32_t compression_threshold_;
    ChannelConfig channel_config_;
    std::atomic<bool> is_ready_{false};
    std::atomic<bool> shutdown_requested_{false};
    std::jthread refresh_thread_;
};
