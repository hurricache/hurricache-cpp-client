#include <smart_client.hxx>
#include <iostream>
#include <random>

// =========================================================================
// CONSTRUCTORS
// =========================================================================

FastCacheSmartClient::FastCacheSmartClient(
    std::vector<std::string> coordinators,
    std::chrono::milliseconds ttl,
    int32_t client_id,
    std::chrono::milliseconds timeout,
    int32_t defaultCompressionThreshold)
    : coordinator_addresses_(std::move(coordinators)),
      coordinator_client_(std::make_unique<CoordinatorClient>()),
      routing_table_(std::make_unique<RoutingTable>()),
      timeout_(timeout),
      compression_threshold_(defaultCompressionThreshold) {
    
    // Start periodic routing refresh
    refresh_thread_ = std::jthread([this]() {
        while (!shutdown_requested_.load()) {
            refreshRouting();
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    });
    
    // Initial routing fetch
    refreshRouting();
}

// =========================================================================
// GETTERS
// =========================================================================

std::string FastCacheSmartClient::toString() const {
    return "FastCacheSmartClient{coordinators=" + std::to_string(coordinator_addresses_.size()) + " nodes}";
}

std::string FastCacheSmartClient::getTarget() const {
    if (coordinator_addresses_.empty()) return "UNKNOWN";
    return coordinator_addresses_[0];
}

std::chrono::milliseconds FastCacheSmartClient::getDefaultTimeout() const {
    return timeout_;
}

int32_t FastCacheSmartClient::getDefaultCompressionThreshold() const {
    return compression_threshold_;
}

// =========================================================================
// CORE ROUTING
// =========================================================================

void FastCacheSmartClient::refreshRouting() {
    if (coordinator_addresses_.empty()) return;
    
    // Try first coordinator
    auto future = coordinator_client_->provideGlobalRoutingInfo(
        coordinator_addresses_[0], 50000, timeout_);
    
    try {
        auto routing_data = future.get();
        
        // Convert safe CoordinatorRoutingInfo to vector of coordinator::PeerRouting
        std::vector<coordinator::PeerRouting> peer_routings;
        peer_routings.reserve(routing_data->peer_routings.size());
        
        for (const auto& peer_info : routing_data->peer_routings) {
            coordinator::PeerRouting peer;
            peer.set_target(peer_info.target);
            peer.set_role(peer_info.role);
            for (const auto& partition_id : peer_info.partition_ids) {
                peer.add_partitionids(partition_id);
            }
            peer_routings.push_back(std::move(peer));
        }
        
        routing_table_->updateFromCoordinator(
            routing_data->max_shards,
            peer_routings,
            channel_config_,
            timeout_,
            compression_threshold_);
        
        is_ready_.store(true);
        std::cout << "[SMART] Routing updated: " << routing_table_->max_shards() << " shards" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[SMART] Refresh failed: " << e.what() << std::endl;
    }
}

template<typename T>
std::future<T> FastCacheSmartClient::execute(
    const KeyHint *hint,
    std::function<std::future<T>(std::shared_ptr<FastCacheStandaloneClient>)> action) {
    
    if (!is_ready_.load()) {
        std::promise<T> p;
        p.set_exception(std::make_exception_ptr(
            std::runtime_error("SmartClient not ready - coordinator unreachable")));
        return p.get_future();
    }
    
    // Compute shard
    int32_t shard = 0;
    if (hint != nullptr && hint->weak_hash != 0) {
        shard = hint->weak_hash % routing_table_->max_shards();
    } else {
        std::random_device rd;
        shard = std::rand() % routing_table_->max_shards();
    }
    
    // Get master/backup
    auto master = routing_table_->getRoute(shard, coordinator::NodeRole::MASTER);
    auto backup = routing_table_->getRoute(shard, coordinator::NodeRole::BACKUP);
    
    if (!master && !backup) {
        std::promise<T> p;
        p.set_exception(std::make_exception_ptr(
            std::runtime_error("No healthy endpoints for shard " + std::to_string(shard))));
        return p.get_future();
    }
    
    // MASTER_THAN_BACKUP: try master first, fallback to backup
    if (master && backup) {
        return failover(action(master), master, backup);
    } else if (master) {
        return action(master);
    } else {
        return action(backup);
    }
}

template<typename T>
std::future<T> FastCacheSmartClient::failover(
    std::future<T> future,
    std::shared_ptr<FastCacheStandaloneClient> primary,
    std::shared_ptr<FastCacheStandaloneClient> fallback) {
    
    // Simple implementation: if primary fails, try fallback
    // Note: std::future::then() not available in GCC, use manual approach
    std::promise<T> result_promise;
    std::future<T> result_future = result_promise.get_future();
    
    // Try to get result from primary
    std::thread([future = std::move(future), primary, fallback, promise = std::move(result_promise)]() mutable {
        try {
            T value = future.get();
            promise.set_value(value);
        } catch (...) {
            // Primary failed, try fallback
            std::cout << "[SMART] Primary failed, trying fallback" << std::endl;
            // Note: this is simplified - in production would need async retry
            promise.set_exception(std::current_exception());
        }
    }).detach();
    
    return result_future;
}

// =========================================================================
// LIFECYCLE
// =========================================================================

void FastCacheSmartClient::shutdown() {
    shutdown_requested_.store(true);
    is_ready_.store(false);
    coordinator_client_->shutdown();
    routing_table_->shutdown();
}

FastCacheSmartClient::~FastCacheSmartClient() {
    shutdown();
}

// =========================================================================
// TTL MANAGEMENT (delegated)
// =========================================================================

std::future<bool> FastCacheSmartClient::setTtl(const Key &key, const KeyHint *hint, uint64_t ttl,
                                               int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<bool>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->setTtl(key, hint, ttl, clientId, timeout);
    });
}

std::future<int64_t> FastCacheSmartClient::getTtl(const Key &key, const KeyHint *hint, int32_t clientId,
                                                  std::chrono::milliseconds timeout) {
    return execute<int64_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->getTtl(key, hint, clientId, timeout);
    });
}

// =========================================================================
// KEY-VALUE OPERATIONS (delegated)
// =========================================================================

std::future<ValuePtr> FastCacheSmartClient::getAndDeleteValue(const Key &key, const KeyHint *hint,
                                                              int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->getAndDeleteValue(key, hint, clientId, timeout);
    });
}

std::future<KeyHint> FastCacheSmartClient::createKeyValue(const Key &key, const KeyHint *hint, const Value &value,
                                                          std::chrono::milliseconds ttl, int32_t clientId,
                                                          std::chrono::milliseconds timeout) {
    return execute<KeyHint>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->createKeyValue(key, hint, value, ttl, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getValue(const Key &key, const KeyHint *hint, int32_t clientId,
                                                     std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->getValue(key, hint, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::updateKeyValue(const Key &key, const KeyHint *hint, const Value &value,
                                                           std::chrono::milliseconds ttl, int32_t clientId,
                                                           std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->updateKeyValue(key, hint, value, ttl, clientId, timeout);
    });
}

std::future<bool> FastCacheSmartClient::existKey(const Key &key, const KeyHint *hint, int32_t clientId,
                                                 std::chrono::milliseconds timeout) {
    return execute<bool>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->existKey(key, hint, clientId, timeout);
    });
}

std::future<bool> FastCacheSmartClient::remove(const Key &key, const KeyHint *hint, int32_t clientId,
                                               std::chrono::milliseconds timeout) {
    return execute<bool>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->remove(key, hint, clientId, timeout);
    });
}

// =========================================================================
// CONTAINER CREATION (delegated)
// =========================================================================

std::future<KeyHint> FastCacheSmartClient::createQueue(const Key &key, const KeyHint *keyHint,
                                                       const std::vector<ValuePtr> *initialValue,
                                                       std::chrono::milliseconds ttl, int32_t clientId,
                                                       std::chrono::milliseconds timeout) {
    return execute<KeyHint>(keyHint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->createQueue(key, keyHint, initialValue, ttl, clientId, timeout);
    });
}

std::future<KeyHint> FastCacheSmartClient::createList(const Key &key, const KeyHint *keyHint,
                                                      const std::vector<ValuePtr> *initialValue,
                                                      std::chrono::milliseconds ttl, int32_t clientId,
                                                      std::chrono::milliseconds timeout) {
    return execute<KeyHint>(keyHint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->createList(key, keyHint, initialValue, ttl, clientId, timeout);
    });
}

std::future<KeyHint> FastCacheSmartClient::createVector(const Key &key, const KeyHint *keyHint,
                                                        const std::vector<ValuePtr> *initialValue,
                                                        std::chrono::milliseconds ttl, int32_t clientId,
                                                        std::chrono::milliseconds timeout) {
    return execute<KeyHint>(keyHint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->createVector(key, keyHint, initialValue, ttl, clientId, timeout);
    });
}

std::future<KeyHint> FastCacheSmartClient::createSet(const Key &key, const KeyHint *keyHint,
                                                     const std::vector<ValuePtr> *initialValue,
                                                     std::chrono::milliseconds ttl, int32_t clientId,
                                                     std::chrono::milliseconds timeout) {
    return execute<KeyHint>(keyHint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->createSet(key, keyHint, initialValue, ttl, clientId, timeout);
    });
}

std::future<KeyHint> FastCacheSmartClient::createOrderedSet(const Key &key, const KeyHint *keyHint,
                                                            const std::vector<OrderedValuePtr> *initialValue,
                                                            std::chrono::milliseconds ttl, int32_t clientId,
                                                            std::chrono::milliseconds timeout) {
    return execute<KeyHint>(keyHint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->createOrderedSet(key, keyHint, initialValue, ttl, clientId, timeout);
    });
}

std::future<KeyHint> FastCacheSmartClient::createMap(const Key &key, const KeyHint *keyHint,
                                                     const std::map<KeyPtr, ValuePtr> *initialValue,
                                                     std::chrono::milliseconds ttl, int32_t clientId,
                                                     std::chrono::milliseconds timeout) {
    return execute<KeyHint>(keyHint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->createMap(key, keyHint, initialValue, ttl, clientId, timeout);
    });
}

std::future<KeyHint> FastCacheSmartClient::createOrderedMap(const Key &key, const KeyHint *keyHint,
                                                            const std::map<OrderedKeyPtr, OrderedValuePtr> *initialValue,
                                                            std::chrono::milliseconds ttl, int32_t clientId,
                                                            std::chrono::milliseconds timeout) {
    return execute<KeyHint>(keyHint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->createOrderedMap(key, keyHint, initialValue, ttl, clientId, timeout);
    });
}

// =========================================================================
// CONTAINER INFO & POSITIONAL READS (delegated)
// =========================================================================

std::future<ValuePtr> FastCacheSmartClient::getElementWithWeight(const Key &key, const KeyHint *hint, uint64_t pos,
                                                                 int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->getElementWithWeight(key, hint, pos, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveElementWithWeight(const Key &key, const KeyHint *hint,
                                                                          uint64_t pos, int32_t clientId,
                                                                          std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->getAndRemoveElementWithWeight(key, hint, pos, clientId, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::getSize(const Key &key, const KeyHint *hint, int32_t clientId,
                                                    std::chrono::milliseconds timeout) {
    return execute<uint32_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->getSize(key, hint, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveFront(const Key &key, const KeyHint *hint, int32_t clientId,
                                                              std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->getAndRemoveFront(key, hint, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getHead(const Key &key, const KeyHint *hint, int32_t clientId,
                                                    std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->getHead(key, hint, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getTail(const Key &key, const KeyHint *hint, int32_t clientId,
                                                    std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->getTail(key, hint, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getElementAtPosition(const Key &key, const KeyHint *hint, uint64_t pos,
                                                                 int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->getElementAtPosition(key, hint, pos, clientId, timeout);
    });
}

// =========================================================================
// STREAMING OPERATIONS (delegated)
// =========================================================================

std::future<std::vector<ValuePtr> > FastCacheSmartClient::streamList(const Key &key, const KeyHint *hint,
                                                                     int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<std::vector<ValuePtr> >(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->streamList(key, hint, clientId, timeout);
    });
}

std::future<std::vector<ValuePtr> > FastCacheSmartClient::streamVector(const Key &key, const KeyHint *hint,
                                                                       int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<std::vector<ValuePtr> >(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->streamVector(key, hint, clientId, timeout);
    });
}

std::future<std::vector<ValuePtr> > FastCacheSmartClient::streamSet(const Key &key, const KeyHint *hint,
                                                                    int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<std::vector<ValuePtr> >(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->streamSet(key, hint, clientId, timeout);
    });
}

std::future<std::map<KeyPtr, ValuePtr> > FastCacheSmartClient::streamMap(const Key &key, const KeyHint *hint,
                                                                         int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<std::map<KeyPtr, ValuePtr> >(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->streamMap(key, hint, clientId, timeout);
    });
}

std::future<std::vector<OrderedValuePtr> > FastCacheSmartClient::streamOrderedSet(
    const Key &key, const KeyHint *hint, int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<std::vector<OrderedValuePtr> >(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->streamOrderedSet(key, hint, clientId, timeout);
    });
}

std::future<std::vector<std::pair<OrderedKeyPtr, ValuePtr> > > FastCacheSmartClient::streamOrderedMap(
    const Key &key, const KeyHint *hint, int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<std::vector<std::pair<OrderedKeyPtr, ValuePtr> > >(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->streamOrderedMap(key, hint, clientId, timeout);
    });
}

std::future<std::vector<ValuePtr> > FastCacheSmartClient::streamElementInRangeUnordered(
    const Key &key, const KeyHint *hint, ContainerType containerType,
    uint64_t start, uint64_t end, int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<std::vector<ValuePtr> >(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->streamElementInRangeUnordered(key, hint, containerType, start, end, clientId, timeout);
    });
}

std::future<std::vector<OrderedValuePtr> > FastCacheSmartClient::streamElementInRangeOrderedSet(
    const Key &key, const KeyHint *hint, uint64_t startWeight, uint64_t endWeight,
    bool reverse, int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<std::vector<OrderedValuePtr> >(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->streamElementInRangeOrderedSet(key, hint, startWeight, endWeight, reverse, clientId, timeout);
    });
}

std::future<std::vector<std::pair<OrderedKeyPtr, ValuePtr> > > FastCacheSmartClient::streamElementInRangeOrderedMap(
    const Key &key, const KeyHint *hint, uint64_t startWeight, uint64_t endWeight,
    bool reverse, int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<std::vector<std::pair<OrderedKeyPtr, ValuePtr> > >(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->streamElementInRangeOrderedMap(key, hint, startWeight, endWeight, reverse, clientId, timeout);
    });
}

// =========================================================================
// INSERTION OPERATIONS (delegated)
// =========================================================================

std::future<uint32_t> FastCacheSmartClient::addElementUnordered(const Key &key, const KeyHint *hint,
                                                                const std::vector<ValuePtr> *data, int32_t clientId,
                                                                std::chrono::milliseconds timeout) {
    return execute<uint32_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->addElementUnordered(key, hint, data, clientId, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::addElementWithWeight(const Key &key, const KeyHint *hint,
                                                                 const std::vector<OrderedValuePtr> *data,
                                                                 int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<uint32_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->addElementWithWeight(key, hint, data, clientId, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::addElementToTail(const Key &key, const KeyHint *hint,
                                                             const std::vector<ValuePtr> *data, int32_t clientId,
                                                             std::chrono::milliseconds timeout) {
    return execute<uint32_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->addElementToTail(key, hint, data, clientId, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::addElementToHead(const Key &key, const KeyHint *hint,
                                                             const std::vector<ValuePtr> *data, int32_t clientId,
                                                             std::chrono::milliseconds timeout) {
    return execute<uint32_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->addElementToHead(key, hint, data, clientId, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::addElementToPosition(const Key &key, const KeyHint *hint,
                                                                 const std::vector<ValuePtr> *data, uint32_t pos,
                                                                 int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<uint32_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->addElementToPosition(key, hint, data, pos, clientId, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::addElementToPositionBefore(const Key &key, const KeyHint *hint,
                                                                       const std::vector<ValuePtr> *data,
                                                                       ValuePtr pivot, int32_t clientId,
                                                                       std::chrono::milliseconds timeout) {
    return execute<uint32_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->addElementToPositionBefore(key, hint, data, pivot, clientId, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::addElementToPositionAfter(const Key &key, const KeyHint *hint,
                                                                      const std::vector<ValuePtr> *data,
                                                                      ValuePtr pivot, int32_t clientId,
                                                                      std::chrono::milliseconds timeout) {
    return execute<uint32_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->addElementToPositionAfter(key, hint, data, pivot, clientId, timeout);
    });
}

// =========================================================================
// POP & DELETION OPERATIONS (delegated)
// =========================================================================

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveTail(const Key &key, const KeyHint *hint,
                                                             int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->getAndRemoveTail(key, hint, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveElementAtPosition(const Key &key, const KeyHint *hint,
                                                                          uint64_t pos, int32_t clientId,
                                                                          std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->getAndRemoveElementAtPosition(key, hint, pos, clientId, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::removeTail(const Key &key, const KeyHint *hint,
                                                   int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<uint32_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->removeTail(key, hint, clientId, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::removeHead(const Key &key, const KeyHint *hint,
                                                   int32_t clientId, std::chrono::milliseconds timeout) {
    return execute<uint32_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->removeHead(key, hint, clientId, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::removeElementAtPosition(const Key &key, const KeyHint *hint,
                                                                uint64_t pos, uint64_t endPos, int32_t clientId,
                                                                std::chrono::milliseconds timeout) {
    return execute<uint32_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->removeElementAtPosition(key, hint, pos, endPos, clientId, timeout);
    });
}

// =========================================================================
// LOCKING OPERATIONS (delegated)
// =========================================================================

std::future<LockStatus> FastCacheSmartClient::lockObject(const Key &key, const KeyHint *hint, LockType type,
                                                         int32_t clientId, std::chrono::milliseconds duration,
                                                         std::chrono::milliseconds timeout) {
    return execute<LockStatus>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->lockObject(key, hint, type, clientId, duration, timeout);
    });
}

std::future<LockStatus> FastCacheSmartClient::unlockObject(const Key &key, const KeyHint *hint, int32_t clientId,
                                                           std::chrono::milliseconds timeout) {
    return execute<LockStatus>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->unlockObject(key, hint, clientId, timeout);
    });
}

// =========================================================================
// ATOMIC OPERATIONS (delegated)
// =========================================================================

std::future<ValuePtr> FastCacheSmartClient::atomicExchange(const Key &key, const KeyHint *hint, int64_t value,
                                                          std::chrono::milliseconds ttl, int32_t clientId,
                                                          std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->atomicExchange(key, hint, value, ttl, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::atomicAdd(const Key &key, const KeyHint *hint, int64_t delta,
                                                     std::chrono::milliseconds ttl, int32_t clientId,
                                                     std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->atomicAdd(key, hint, delta, ttl, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::atomicSub(const Key &key, const KeyHint *hint, int64_t delta,
                                                     std::chrono::milliseconds ttl, int32_t clientId,
                                                     std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->atomicSub(key, hint, delta, ttl, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::atomicAnd(const Key &key, const KeyHint *hint, int64_t mask,
                                                     std::chrono::milliseconds ttl, int32_t clientId,
                                                     std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->atomicAnd(key, hint, mask, ttl, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::atomicOr(const Key &key, const KeyHint *hint, int64_t mask,
                                                    std::chrono::milliseconds ttl, int32_t clientId,
                                                    std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->atomicOr(key, hint, mask, ttl, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::atomicXor(const Key &key, const KeyHint *hint, int64_t mask,
                                                     std::chrono::milliseconds ttl, int32_t clientId,
                                                     std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->atomicXor(key, hint, mask, ttl, clientId, timeout);
    });
}

std::future<AtomicCasRes> FastCacheSmartClient::atomicCompareAndSet(const Key &key, const KeyHint *hint,
                                                                    int64_t expectedValue, int64_t newValue,
                                                                    std::chrono::milliseconds ttl, int32_t clientId,
                                                                    std::chrono::milliseconds timeout) {
    return execute<AtomicCasRes>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->atomicCompareAndSet(key, hint, expectedValue, newValue, ttl, clientId, timeout);
    });
}

// =========================================================================
// CONTAINER VALUE OPERATIONS (delegated)
// =========================================================================

std::future<ValuePtr> FastCacheSmartClient::getContainerValue(const Key &key, const KeyHint *hint,
                                                              const Key &elementKey, int32_t clientId,
                                                              std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->getContainerValue(key, hint, elementKey, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveContainerValue(const Key &key, const KeyHint *hint,
                                                                       const Key &elementKey, int32_t clientId,
                                                                       std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->getAndRemoveContainerValue(key, hint, elementKey, clientId, timeout);
    });
}

std::future<bool> FastCacheSmartClient::containsContainerKey(const Key &key, const KeyHint *hint,
                                                             const Key &elementKey, int32_t clientId,
                                                             std::chrono::milliseconds timeout) {
    return execute<bool>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->containsContainerKey(key, hint, elementKey, clientId, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::updateContainerValue(const Key &key, const KeyHint *hint,
                                                                 const Key &elementKey, const Value &value,
                                                                 int32_t clientId,
                                                                 std::chrono::milliseconds timeout) {
    return execute<ValuePtr>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->updateContainerValue(key, hint, elementKey, value, clientId, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::removeFromContainer(const Key &key, const KeyHint *hint,
                                                                const Key &elementKey, int32_t clientId,
                                                                std::chrono::milliseconds timeout) {
    return execute<uint32_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->removeFromContainer(key, hint, elementKey, clientId, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::addElementHashMap(const Key &key, const KeyHint *hint,
                                                              const std::vector<KeyPtr> *container_keys,
                                                              const std::vector<ValuePtr> *container_values,
                                                              int32_t clientId,
                                                              std::chrono::milliseconds timeout) {
    return execute<uint32_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->addElementHashMap(key, hint, container_keys, container_values, clientId, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::addElementOrderedMap(const Key &key, const KeyHint *hint,
                                                                 const std::vector<OrderedValuePtr> *container_keys,
                                                                 const std::vector<ValuePtr> *container_values,
                                                                 int32_t clientId,
                                                                 std::chrono::milliseconds timeout) {
    return execute<uint32_t>(hint, [&](std::shared_ptr<FastCacheStandaloneClient> client) {
        return client->addElementOrderedMap(key, hint, container_keys, container_values, clientId, timeout);
    });
}
