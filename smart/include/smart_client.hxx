#pragma once

#include <future>
#include <vector>
#include <map>
#include <string>
#include <chrono>

#include "types.hxx"

// Предполагаемые типы/структуры (из контекста проекта)


class FastCacheSmartClient {
public:
    // Конструкторы
    FastCacheSmartClient(const std::string& host, int32_t port, int32_t defaultClientId,
                               std::chrono::milliseconds timeout, int32_t defaultCompressionThreshold);
    FastCacheSmartClient(const std::string& host, int32_t port, int32_t defaultClientId, std::chrono::milliseconds timeout);
    FastCacheSmartClient(const std::string& host, int32_t port, int32_t clientId);
    FastCacheSmartClient(const std::string& host, int32_t port);
    FastCacheSmartClient(const std::string& host, int32_t port, std::chrono::milliseconds duration);

    // Геттеры и метаинформация
    [[nodiscard]] std::string toString() const;
    [[nodiscard]] std::string getTarget() const;
    [[nodiscard]] int32_t getDefaultClientId() const;
    [[nodiscard]] std::chrono::milliseconds getDefaultTimeout() const;
    [[nodiscard]] int32_t getDefaultCompressionThreshold() const;

    // =========================================================================
    // TTL MANAGEMENT
    // =========================================================================
    [[nodiscard]] std::future<bool> setTtl(const Key &key, const KeyHint *hint, int64_t ttl,
                                           int32_t clientId = 0,
                                           std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int64_t> getTtl(const Key &key, const KeyHint *hint,
                                              int32_t clientId = 0,
                                              std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // KEY-VALUE OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<ValuePtr> getAndDeleteValue(const Key &key, const KeyHint *hint,
                                                          int32_t clientId = 0,
                                                          std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<KeyHint> createKeyValue(const Key &key, const KeyHint *hint, const Value &value,
                                                      std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                      int32_t clientId = 0,
                                                      std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getValue(const Key &key, const KeyHint *hint,
                                                 int32_t clientId = 0,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> updateKeyValue(const Key &key, const KeyHint *hint, const Value &value,
                                                       std::chrono::milliseconds ttl  = std::chrono::milliseconds(0),
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
                                                  std::chrono::milliseconds ttl= std::chrono::milliseconds(0),
                                                  int32_t clientId = 0,
                                                  std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<KeyHint> createVector(const Key &key, const KeyHint *keyHint = nullptr,
                                                    const std::vector<ValuePtr> *initialValue = nullptr,
                                                    std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                    int32_t clientId = 0,
                                                    std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<KeyHint> createSet(const Key &key, const KeyHint *keyHint = nullptr,
                                                 const std::vector<ValuePtr> *initialValue = nullptr,
                                                 std::chrono::milliseconds ttl= std::chrono::milliseconds(0),
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
                                                        const std::map<OrderedKey, OrderedValue> *initialValue = nullptr,
                                                         std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                        int32_t clientId = 0,
                                                        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getElementWithWeight(const Key &key, const KeyHint *hint, int32_t pos,
                                                             int32_t clientId = 0,
                                                             std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getAndRemoveElementWithWeight(const Key &key, const KeyHint *hint, int32_t pos,
                                                                      int32_t clientId = 0,
                                                                      std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int32_t> getSize(const Key &key, const KeyHint *hint,
                                               int32_t clientId = 0,
                                               std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getAndRemoveFront(const Key &key, const KeyHint *hint,
                                                          int32_t clientId = 0,
                                                          std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getHead(const Key &key, const KeyHint *hint,
                                                int32_t clientId = 0,
                                                std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getTail(const Key &key, const KeyHint *hint,
                                                int32_t clientId = 0,
                                                std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getElementAtPosition(const Key &key, const KeyHint *hint, int32_t pos,
                                                             int32_t clientId = 0,
                                                             std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // STREAMING READ OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<std::vector<ValuePtr> > streamList(const Key &key, const KeyHint *hint,
                                                                 int32_t clientId = 0,
                                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::vector<ValuePtr> > streamVector(const Key &key, const KeyHint *hint,
                                                                   int32_t clientId = 0,
                                                                   std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::vector<ValuePtr> > streamSet(const Key &key, const KeyHint *hint,
                                                                int32_t clientId = 0,
                                                                std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::map<KeyPtr, ValuePtr> > streamMap(const Key &key, const KeyHint *hint,
                                                                     int32_t clientId = 0,
                                                                     std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::vector<OrderedValuePtr> > streamOrderedSet(
        const Key &key, const KeyHint *hint, int32_t clientId = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::map<OrderedKey, Value> > streamOrderedMap(
        const Key &key, const KeyHint *hint, int32_t clientId = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::vector<ValuePtr> > streamElementInRangeUnordered(
        const Key &key, const KeyHint *hint, ContainerType containerType,
        int32_t start, int32_t end, int32_t clientId = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::vector<OrderedValuePtr> > streamElementInRangeOrderedSet(
        const Key &key, const KeyHint *hint, int64_t startWeight,
        int64_t endWeight, bool reverse, int32_t clientId = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<std::map<OrderedKeyPtr, ValuePtr> > streamElementInRangeOrderedMap(
        const Key &key, const KeyHint *hint, int64_t startWeight,
        int64_t endWeight, bool reverse, int32_t clientId = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // INSERTION OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<int32_t> addElementUnordered(const Key &key, const KeyHint *hint,
                                                           const std::vector<ValuePtr> *data = nullptr,
                                                           int32_t clientId = 0,
                                                           std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int32_t> addElementWithWeight(const Key &key, const KeyHint *hint,
                                                            const std::vector<OrderedValuePtr> *data = nullptr,
                                                            int32_t clientId = 0,
                                                            std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int32_t> addElementToTail(const Key &key, const KeyHint *hint,
                                                        const std::vector<ValuePtr> *data = nullptr,
                                                        int32_t clientId = 0,
                                                        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int32_t> addElementToHead(const Key &key, const KeyHint *hint,
                                                        const std::vector<ValuePtr> *data = nullptr,
                                                        int32_t clientId = 0,
                                                        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int32_t> addElementToPosition(const Key &key, const KeyHint *hint,
                                                            const std::vector<ValuePtr> *data, int32_t pos,
                                                            int32_t clientId = 0,
                                                            std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int32_t> addElementToPositionBefore(const Key &key, const KeyHint *hint,
                                                                  const std::vector<ValuePtr> *data = nullptr, ValuePtr pivot = nullptr,
                                                                  int32_t clientId = 0,
                                                                  std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int32_t> addElementToPositionAfter(const Key &key, const KeyHint *hint,
                                                                 const std::vector<ValuePtr> *data = nullptr, ValuePtr pivot = nullptr,
                                                                 int32_t clientId = 0,
                                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // POP & DELETION OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<ValuePtr> getAndRemoveTail(const Key &key, const KeyHint *hint,
                                                         int32_t clientId = 0,
                                                         std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<ValuePtr> getAndRemoveElementAtPosition(const Key &key, const KeyHint *hint, int32_t pos,
                                                                      int32_t clientId = 0,
                                                                      std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<bool> removeTail(const Key &key, const KeyHint *hint,
                                               int32_t clientId = 0,
                                               std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<bool> removeHead(const Key &key, const KeyHint *hint,
                                               int32_t clientId = 0,
                                               std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<bool> removeElementAtPosition(const Key &key, const KeyHint *hint, int64_t pos,
                                                            int64_t endPos, int32_t clientId = 0,
                                                            std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int32_t> removeFromContainer(const Key &key, const KeyHint *hint, ContainerType type,
                                                           const std::vector<KeyPtr> *keys,
                                                           const std::vector<ValuePtr> *values,
                                                           int32_t clientId = 0,
                                                           std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // LOCKING OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<LockStatus> lockObject(const Key &key, const KeyHint *hint, LockType type,
                                                     int32_t clientId = 0,
                                                     std::chrono::milliseconds duration = std::chrono::milliseconds(60000),
                                                     std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<LockStatus> unlockObject(const Key &key, const KeyHint *hint,
                                                       int32_t clientId = 0,
                                                       std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // ATOMIC OPERATIONS
    // =========================================================================
    [[nodiscard]] std::future<int64_t> atomicLoad(const Key &key, const KeyHint *hint,
                                                  int32_t clientId = 0,
                                                  std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int64_t> atomicLoadAndDelete(const Key &key, const KeyHint *hint,
                                                           int32_t clientId = 0,
                                                           std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

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
                                                                 std::chrono::milliseconds ttl = std::chrono::milliseconds(0),
                                                                int32_t clientId = 0,
                                                                std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

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

    [[nodiscard]] std::future<int32_t> removeFromContainer(const Key &key, const KeyHint *hint, const Key &elementKey,
                                                           int32_t clientId = 0,
                                                           std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int32_t> addElementHashMap(const Key &key, const KeyHint *hint,
                                                         const std::vector<KeyPtr> *container_keys = nullptr,
                                                         const std::vector<ValuePtr> *container_values = nullptr,
                                                         int32_t clientId = 0,
                                                         std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] std::future<int32_t> addElementOrderedMap(const Key &key, const KeyHint *hint,
                                                            const std::vector<OrderedValuePtr> *container_keys = nullptr,
                                                            const std::vector<ValuePtr> *container_values = nullptr,
                                                            int32_t clientId = 0,
                                                            std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // =========================================================================
    // LIFECYCLE
    // =========================================================================
    void shutdown();
};