#pragma once

#include <future>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

#include "types.hxx"
#include "standalone_client.hxx"
#include "coordinator_client.hxx"
#include "routing_table.hxx"
#include "channel_config.hxx"

// Routing mode (analog of com.hurricache.client.intf.Mode)
enum class Mode {
    MASTER,
    BACKUP,
    MASTER_THAN_BACKUP,
    LB_SMART
};

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
    [[nodiscard]] bool isReady() const { return is_ready_.load(); }

    // Routing mode override (per client; default MASTER_THAN_BACKUP)
    FastCacheSmartClient &setMode(Mode mode);
    FastCacheSmartClient &clearModeOverride();

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

    [[nodiscard]] std::future<uint32_t> removeFromContainer(const Key &key, const KeyHint *hint,
                                                            const std::vector<KeyPtr> *keys,
                                                            const std::vector<ValuePtr> *values = nullptr,
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
    using SharedClient = std::shared_ptr<FastCacheStandaloneClient>;
    using SharedKey = std::shared_ptr<Key>;
    using SharedHint = std::shared_ptr<KeyHint>;

    // Core routing: picks shard by hint (or random), applies Mode, runs action
    // with reroute/failover guard. Completion is delivered via a worker thread,
    // because std::future has no continuations.
    template<typename T>
    std::future<T> execute(SharedHint hint,
                           std::function<std::future<T>(SharedClient)> action);

    // Java fallbackGuard: FAILED_PRECONDITION -> reroute via x-fastcache-route,
    // UNAVAILABLE -> topology refresh + retry on fallback endpoint.
    template<typename T>
    T attemptWithGuard(int shard,
                       const std::function<std::future<T>(SharedClient)> &action,
                       SharedClient current, SharedClient fallback);

    [[nodiscard]] Mode effectiveMode() const;
    void waitReady();
    void refreshRouting();
    void triggerAsyncRefresh();

    // Chunked container creation: create with first chunk, repDelay, then
    // sequentially push remaining chunks addressed by the returned KeyHint.
    std::future<KeyHint> createWithRemainder(
        std::future<KeyHint> create_future,
        std::function<void(std::shared_ptr<KeyHint>)> remainder_sender);

    // Sequential chunked additions with result summation.
    std::future<uint32_t> runAddPipeline(
        std::vector<std::function<std::future<uint32_t>()> > chunk_calls);

    std::future<KeyHint> createUnorderedContainerSmart(
        const Key &key, const KeyHint *hint, const std::vector<ValuePtr> *initialValue,
        std::chrono::milliseconds ttl, int32_t clientId, std::chrono::milliseconds timeout,
        ContainerType type);

    [[nodiscard]] int64_t maxRpcSize() const;
    [[nodiscard]] int32_t effectiveClientId(int32_t clientId) const;
    [[nodiscard]] std::chrono::milliseconds effectiveTtl(std::chrono::milliseconds ttl) const;

    std::vector<std::string> coordinator_addresses_;
    std::unique_ptr<CoordinatorClient> coordinator_client_;
    std::unique_ptr<RoutingTable> routing_table_;
    std::chrono::milliseconds timeout_;
    std::chrono::milliseconds default_ttl_;
    int32_t default_client_id_;
    int32_t compression_threshold_;
    ChannelConfig channel_config_;
    Mode configured_mode_ = Mode::MASTER_THAN_BACKUP;
    std::atomic<int> mode_override_{-1}; // -1 = no override
    std::atomic<bool> is_ready_{false};
    std::atomic<bool> is_updating_{false};
    std::atomic<bool> shutdown_requested_{false};
    std::atomic<uint32_t> random_counter_{0};
    std::atomic<int32_t> active_coordinator_index_{0};
    std::mutex ready_mutex_;
    std::condition_variable ready_cv_;
    std::chrono::milliseconds ready_timeout_{60000};
    std::jthread refresh_thread_;
};
