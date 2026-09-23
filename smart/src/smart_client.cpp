#include <smart_client.hxx>

FastCacheSmartClient::FastCacheSmartClient(const std::string &host, int32_t port, int32_t defaultClientId,
    std::chrono::milliseconds timeout, int32_t defaultCompressionThreshold) {
}

FastCacheSmartClient::FastCacheSmartClient(const std::string &host, int32_t port, int32_t defaultClientId,
    std::chrono::milliseconds timeout) {
}

FastCacheSmartClient::FastCacheSmartClient(const std::string &host, int32_t port, int32_t clientId) {
}

FastCacheSmartClient::FastCacheSmartClient(const std::string &host, int32_t port) {
}

FastCacheSmartClient::FastCacheSmartClient(const std::string &host, int32_t port, std::chrono::milliseconds duration) {
}

std::string FastCacheSmartClient::toString() const {
}

std::string FastCacheSmartClient::getTarget() const {
}

int32_t FastCacheSmartClient::getDefaultClientId() const {
}

std::chrono::milliseconds FastCacheSmartClient::getDefaultTimeout() const {
}

int32_t FastCacheSmartClient::getDefaultCompressionThreshold() const {
}

std::future<bool> FastCacheSmartClient::setTtl(const Key &key, const KeyHint *hint, int64_t ttl, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<int64_t> FastCacheSmartClient::getTtl(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<ValuePtr> FastCacheSmartClient::getAndDeleteValue(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<KeyHint> FastCacheSmartClient::createKeyValue(const Key &key, const KeyHint *hint, const Value &value,
    std::chrono::milliseconds ttl, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<ValuePtr> FastCacheSmartClient::getValue(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<ValuePtr> FastCacheSmartClient::updateKeyValue(const Key &key, const KeyHint *hint, const Value &value,
    std::chrono::milliseconds ttl, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<bool> FastCacheSmartClient::existKey(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<bool> FastCacheSmartClient::remove(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<KeyHint> FastCacheSmartClient::createQueue(const Key &key, const KeyHint *keyHint,
    const std::vector<ValuePtr> *initialValue, std::chrono::milliseconds ttl, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<KeyHint> FastCacheSmartClient::createList(const Key &key, const KeyHint *keyHint,
    const std::vector<ValuePtr> *initialValue, std::chrono::milliseconds ttl, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<KeyHint> FastCacheSmartClient::createVector(const Key &key, const KeyHint *keyHint,
    const std::vector<ValuePtr> *initialValue, std::chrono::milliseconds ttl, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<KeyHint> FastCacheSmartClient::createSet(const Key &key, const KeyHint *keyHint,
    const std::vector<ValuePtr> *initialValue, std::chrono::milliseconds ttl, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<KeyHint> FastCacheSmartClient::createOrderedSet(const Key &key, const KeyHint *keyHint,
    const std::vector<OrderedValuePtr> *initialValue, std::chrono::milliseconds ttl, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<KeyHint> FastCacheSmartClient::createMap(const Key &key, const KeyHint *keyHint,
    const std::map<KeyPtr, ValuePtr> *initialValue, std::chrono::milliseconds ttl, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<KeyHint> FastCacheSmartClient::createOrderedMap(const Key &key, const KeyHint *keyHint,
    const std::map<OrderedKey, OrderedValue> *initialValue, std::chrono::milliseconds ttl, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<ValuePtr> FastCacheSmartClient::getElementWithWeight(const Key &key, const KeyHint *hint, int32_t pos,
    int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveElementWithWeight(const Key &key, const KeyHint *hint,
    int32_t pos, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int32_t> FastCacheSmartClient::getSize(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveFront(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<ValuePtr> FastCacheSmartClient::getHead(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<ValuePtr> FastCacheSmartClient::getTail(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<ValuePtr> FastCacheSmartClient::getElementAtPosition(const Key &key, const KeyHint *hint, int32_t pos,
    int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<std::vector<ValuePtr>> FastCacheSmartClient::streamList(const Key &key, const KeyHint *hint,
    int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<std::vector<ValuePtr>> FastCacheSmartClient::streamVector(const Key &key, const KeyHint *hint,
    int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<std::vector<ValuePtr>> FastCacheSmartClient::streamSet(const Key &key, const KeyHint *hint,
    int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<std::map<KeyPtr, ValuePtr>> FastCacheSmartClient::streamMap(const Key &key, const KeyHint *hint,
    int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<std::vector<OrderedValuePtr>> FastCacheSmartClient::streamOrderedSet(const Key &key, const KeyHint *hint,
    int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<std::map<OrderedKey, Value>> FastCacheSmartClient::streamOrderedMap(const Key &key, const KeyHint *hint,
    int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<std::vector<ValuePtr>> FastCacheSmartClient::streamElementInRangeUnordered(const Key &key,
    const KeyHint *hint, ContainerType containerType, int32_t start, int32_t end, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<std::vector<OrderedValuePtr>> FastCacheSmartClient::streamElementInRangeOrderedSet(const Key &key,
    const KeyHint *hint, int64_t startWeight, int64_t endWeight, bool reverse, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<std::map<OrderedKeyPtr, ValuePtr>> FastCacheSmartClient::streamElementInRangeOrderedMap(const Key &key,
    const KeyHint *hint, int64_t startWeight, int64_t endWeight, bool reverse, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<int32_t> FastCacheSmartClient::addElementUnordered(const Key &key, const KeyHint *hint,
    const std::vector<ValuePtr> *data, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int32_t> FastCacheSmartClient::addElementWithWeight(const Key &key, const KeyHint *hint,
    const std::vector<OrderedValuePtr> *data, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int32_t> FastCacheSmartClient::addElementToTail(const Key &key, const KeyHint *hint,
    const std::vector<ValuePtr> *data, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int32_t> FastCacheSmartClient::addElementToHead(const Key &key, const KeyHint *hint,
    const std::vector<ValuePtr> *data, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int32_t> FastCacheSmartClient::addElementToPosition(const Key &key, const KeyHint *hint,
    const std::vector<ValuePtr> *data, int32_t pos, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int32_t> FastCacheSmartClient::addElementToPositionBefore(const Key &key, const KeyHint *hint,
    const std::vector<ValuePtr> *data, ValuePtr pivot, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int32_t> FastCacheSmartClient::addElementToPositionAfter(const Key &key, const KeyHint *hint,
    const std::vector<ValuePtr> *data, ValuePtr pivot, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveTail(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveElementAtPosition(const Key &key, const KeyHint *hint,
    int32_t pos, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<bool> FastCacheSmartClient::removeTail(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<bool> FastCacheSmartClient::removeHead(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<bool> FastCacheSmartClient::removeElementAtPosition(const Key &key, const KeyHint *hint, int64_t pos,
    int64_t endPos, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int32_t> FastCacheSmartClient::removeFromContainer(const Key &key, const KeyHint *hint, ContainerType type,
    const std::vector<KeyPtr> *keys, const std::vector<ValuePtr> *values, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<LockStatus> FastCacheSmartClient::lockObject(const Key &key, const KeyHint *hint, LockType type,
    int32_t clientId, std::chrono::milliseconds duration, std::chrono::milliseconds timeout) {
}

std::future<LockStatus> FastCacheSmartClient::unlockObject(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<int64_t> FastCacheSmartClient::atomicLoad(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<int64_t> FastCacheSmartClient::atomicLoadAndDelete(const Key &key, const KeyHint *hint, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<KeyHint> FastCacheSmartClient::atomicCreate(const Key &key, const KeyHint *hint, int64_t value,
    std::chrono::milliseconds ttl, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<KeyHint> FastCacheSmartClient::atomicStore(const Key &key, const KeyHint *hint, int64_t value,
    std::chrono::milliseconds ttl, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int64_t> FastCacheSmartClient::atomicExchange(const Key &key, const KeyHint *hint, int64_t value,
    std::chrono::milliseconds ttl, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int64_t> FastCacheSmartClient::atomicAdd(const Key &key, const KeyHint *hint, int64_t delta,
    std::chrono::milliseconds ttl, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int64_t> FastCacheSmartClient::atomicSub(const Key &key, const KeyHint *hint, int64_t delta,
    std::chrono::milliseconds ttl, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int64_t> FastCacheSmartClient::atomicAnd(const Key &key, const KeyHint *hint, int64_t mask,
    std::chrono::milliseconds ttl, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int64_t> FastCacheSmartClient::atomicOr(const Key &key, const KeyHint *hint, int64_t mask,
    std::chrono::milliseconds ttl, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int64_t> FastCacheSmartClient::atomicXor(const Key &key, const KeyHint *hint, int64_t mask,
    std::chrono::milliseconds ttl, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<AtomicCasRes> FastCacheSmartClient::atomicCompareAndSet(const Key &key, const KeyHint *hint,
    int64_t expectedValue, int64_t newValue, std::chrono::milliseconds ttl, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<ValuePtr> FastCacheSmartClient::getContainerValue(const Key &key, const KeyHint *hint,
    const Key &elementKey, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveContainerValue(const Key &key, const KeyHint *hint,
    const Key &elementKey, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<bool> FastCacheSmartClient::containsContainerKey(const Key &key, const KeyHint *hint, const Key &elementKey,
    int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<ValuePtr> FastCacheSmartClient::updateContainerValue(const Key &key, const KeyHint *hint,
    const Key &elementKey, const Value &value, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int32_t> FastCacheSmartClient::removeFromContainer(const Key &key, const KeyHint *hint,
    const Key &elementKey, int32_t clientId, std::chrono::milliseconds timeout) {
}

std::future<int32_t> FastCacheSmartClient::addElementHashMap(const Key &key, const KeyHint *hint,
    const std::vector<KeyPtr> *container_keys, const std::vector<ValuePtr> *container_values, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

std::future<int32_t> FastCacheSmartClient::addElementOrderedMap(const Key &key, const KeyHint *hint,
    const std::vector<OrderedValuePtr> *container_keys, const std::vector<ValuePtr> *container_values, int32_t clientId,
    std::chrono::milliseconds timeout) {
}

void FastCacheSmartClient::shutdown() {
}
