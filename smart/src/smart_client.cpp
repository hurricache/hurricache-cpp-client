#include <smart_client.hxx>
#include "utils.hxx"

#include <iostream>
#include <random>
#include <stdexcept>

namespace {

using SharedClient = std::shared_ptr<FastCacheStandaloneClient>;
using milliseconds = std::chrono::milliseconds;

// =========================================================================
// DEEP-COPY HELPERS (arguments must outlive the async operation + retries)
// =========================================================================

std::shared_ptr<Key> copyKey(const Key &key) {
    return std::make_shared<Key>(key.size, key.data);
}

std::unique_ptr<Value> copyValue(const Value *v) {
    if (v == nullptr) return nullptr;
    switch (v->type) {
        case INT: return std::make_unique<Value>(v->int_value);
        case DOUBLE: return std::make_unique<Value>(v->double_value);
        case BYTES:
        default:
            if (v->size == 0 || v->data == nullptr) return std::make_unique<Value>();
            return std::make_unique<Value>(v->data, v->size);
    }
}

std::unique_ptr<OrderedValue> copyOrderedValue(const OrderedValue *v) {
    if (v == nullptr) return nullptr;
    switch (v->type) {
        case INT: return std::make_unique<OrderedValue>(v->weight, v->int_value);
        case DOUBLE: return std::make_unique<OrderedValue>(v->weight, v->double_value);
        case BYTES:
        default:
            if (v->size == 0 || v->data == nullptr) {
                auto empty = std::make_unique<OrderedValue>();
                empty->weight = v->weight;
                return empty;
            }
            return std::make_unique<OrderedValue>(v->weight, v->data, v->size);
    }
}

struct OwnedValueList {
    std::vector<std::unique_ptr<Value> > storage;
    std::vector<ValuePtr> ptrs;
};

std::shared_ptr<OwnedValueList> copyValues(const std::vector<ValuePtr> *data) {
    auto owned = std::make_shared<OwnedValueList>();
    if (data != nullptr) {
        owned->storage.reserve(data->size());
        owned->ptrs.reserve(data->size());
        for (const auto *v : *data) {
            owned->storage.push_back(copyValue(v));
            owned->ptrs.push_back(owned->storage.back().get());
        }
    }
    return owned;
}

struct OwnedOrderedValueList {
    std::vector<std::unique_ptr<OrderedValue> > storage;
    std::vector<OrderedValuePtr> ptrs;
};

std::shared_ptr<OwnedOrderedValueList> copyOrderedValues(const std::vector<OrderedValuePtr> *data) {
    auto owned = std::make_shared<OwnedOrderedValueList>();
    if (data != nullptr) {
        owned->storage.reserve(data->size());
        owned->ptrs.reserve(data->size());
        for (const auto *v : *data) {
            owned->storage.push_back(copyOrderedValue(v));
            owned->ptrs.push_back(owned->storage.back().get());
        }
    }
    return owned;
}

struct OwnedMapData {
    std::vector<std::unique_ptr<Key> > key_storage;
    std::vector<std::unique_ptr<Value> > value_storage;
    std::vector<std::pair<KeyPtr, ValuePtr> > entries; // iteration order preserved
};

std::shared_ptr<OwnedMapData> copyMap(const std::map<KeyPtr, ValuePtr> *data) {
    auto owned = std::make_shared<OwnedMapData>();
    if (data != nullptr) {
        owned->key_storage.reserve(data->size());
        owned->value_storage.reserve(data->size());
        owned->entries.reserve(data->size());
        for (const auto &[k, v] : *data) {
            owned->key_storage.push_back(std::make_unique<Key>(k->size, k->data));
            owned->value_storage.push_back(copyValue(v));
            owned->entries.emplace_back(owned->key_storage.back().get(),
                                        owned->value_storage.back().get());
        }
    }
    return owned;
}

struct OwnedOrderedMapData {
    std::vector<std::unique_ptr<OrderedKey> > key_storage;
    std::vector<std::unique_ptr<OrderedValue> > value_storage;
    // Keys re-materialized as OrderedValue (addElementOrderedMap API takes OrderedValuePtr keys)
    std::vector<std::unique_ptr<OrderedValue> > key_as_value_storage;
    std::vector<std::pair<OrderedKeyPtr, OrderedValuePtr> > entries;
};

std::shared_ptr<OwnedOrderedMapData> copyOrderedMap(const std::map<OrderedKeyPtr, OrderedValuePtr> *data) {
    auto owned = std::make_shared<OwnedOrderedMapData>();
    if (data != nullptr) {
        owned->key_storage.reserve(data->size());
        owned->value_storage.reserve(data->size());
        owned->key_as_value_storage.reserve(data->size());
        owned->entries.reserve(data->size());
        for (const auto &[k, v] : *data) {
            owned->key_storage.push_back(std::make_unique<OrderedKey>(k->weight, k->size, k->data));
            owned->value_storage.push_back(copyOrderedValue(v));
            if (k->size == 0 || k->data == nullptr) {
                auto empty_key = std::make_unique<OrderedValue>();
                empty_key->weight = k->weight;
                owned->key_as_value_storage.push_back(std::move(empty_key));
            } else {
                owned->key_as_value_storage.push_back(
                    std::make_unique<OrderedValue>(k->weight, k->data, k->size));
            }
            owned->entries.emplace_back(owned->key_storage.back().get(),
                                        owned->value_storage.back().get());
        }
    }
    return owned;
}

// Compute hint from the key when caller did not provide one
// (week_hash/strong_hash via common calculateKeyHint).
std::shared_ptr<KeyHint> resolveHint(const Key &key, const KeyHint *hint) {
    if (hint != nullptr) return std::make_shared<KeyHint>(*hint);
    return std::shared_ptr<KeyHint>(calculateKeyHint(key));
}

// =========================================================================
// SIZE ESTIMATION FOR CHUNKING (serialized proto sizes, as in Java client)
// =========================================================================

size_t estimateValueSize(const Value *v, int32_t clientId, int32_t threshold) {
    return buildValueProto(*v, milliseconds(0), clientId, threshold).ByteSizeLong();
}

size_t estimateOrderedValueSize(const OrderedValue *v, int32_t clientId, int32_t threshold) {
    hurricache::OrderedValue ov;
    ov.set_order(v->weight);
    auto pv = buildValueProto(*v, milliseconds(0), clientId, threshold);
    *ov.mutable_value() = pv.value();
    if (pv.has_compressioninfo()) *ov.mutable_compressioninfo() = pv.compressioninfo();
    if (pv.has_value_type()) ov.set_value_type(pv.value_type());
    return ov.ByteSizeLong();
}

size_t estimateKeySize(const Key *k, int32_t clientId, int32_t threshold) {
    return buildKeyProto(*k, nullptr, clientId, threshold).ByteSizeLong();
}

size_t estimateOrderedKeySize(const OrderedKey *k, int32_t clientId) {
    hurricache::OrderedKey ok;
    ok.mutable_payload()->set_size(k->size);
    ok.mutable_payload()->set_payload(k->data, k->size);
    ok.set_order(k->weight);
    ok.set_clientid(static_cast<uint32_t>(clientId));
    return ok.ByteSizeLong();
}

// =========================================================================
// CHUNK SPLITTERS (Java: splitVariants / splitOrderedPayloadsVariant /
// splitMapPayloads / splitOrderedMapPayloads / partitionChunks)
// =========================================================================

std::vector<std::vector<ValuePtr> > partitionValueChunks(
    const std::vector<ValuePtr> &data, size_t base_size,
    int32_t clientId, int32_t threshold, int64_t max_rpc_size) {
    std::vector<std::vector<ValuePtr> > chunks;
    size_t current_size = base_size;
    std::vector<ValuePtr> current;
    for (auto *v : data) {
        size_t elem = estimateValueSize(v, clientId, threshold);
        if (!current.empty() && current_size + elem > static_cast<size_t>(max_rpc_size)) {
            chunks.push_back(std::move(current));
            current.clear();
            current_size = base_size;
        }
        current.push_back(v);
        current_size += elem;
    }
    if (!current.empty()) chunks.push_back(std::move(current));
    return chunks;
}

std::vector<std::vector<OrderedValuePtr> > partitionOrderedValueChunks(
    const std::vector<OrderedValuePtr> &data, size_t base_size,
    int32_t clientId, int32_t threshold, int64_t max_rpc_size) {
    std::vector<std::vector<OrderedValuePtr> > chunks;
    size_t current_size = base_size;
    std::vector<OrderedValuePtr> current;
    for (auto *v : data) {
        size_t elem = estimateOrderedValueSize(v, clientId, threshold);
        if (!current.empty() && current_size + elem > static_cast<size_t>(max_rpc_size)) {
            chunks.push_back(std::move(current));
            current.clear();
            current_size = base_size;
        }
        current.push_back(v);
        current_size += elem;
    }
    if (!current.empty()) chunks.push_back(std::move(current));
    return chunks;
}

struct MapChunk {
    std::vector<KeyPtr> keys;
    std::vector<ValuePtr> values;
};

std::vector<MapChunk> partitionMapChunks(
    const std::vector<std::pair<KeyPtr, ValuePtr> > &entries, size_t base_size,
    int32_t clientId, int32_t threshold, int64_t max_rpc_size) {
    std::vector<MapChunk> chunks;
    size_t current_size = base_size;
    MapChunk current;
    for (const auto &[k, v] : entries) {
        size_t pair_size = estimateKeySize(k, clientId, threshold) + 32;
        if (v != nullptr) pair_size += estimateValueSize(v, clientId, threshold);
        if (!current.keys.empty() && current_size + pair_size > static_cast<size_t>(max_rpc_size)) {
            chunks.push_back(std::move(current));
            current = MapChunk{};
            current_size = base_size;
        }
        current.keys.push_back(k);
        if (v != nullptr) current.values.push_back(v);
        current_size += pair_size;
    }
    if (!current.keys.empty()) chunks.push_back(std::move(current));
    return chunks;
}

struct OrderedMapChunk {
    std::vector<OrderedValuePtr> keys;   // C++ API uses OrderedValuePtr as ordered keys
    std::vector<ValuePtr> values;
};

std::vector<OrderedMapChunk> partitionOrderedMapChunks(
    const std::vector<std::pair<OrderedValuePtr, ValuePtr> > &entries, size_t base_size,
    int32_t clientId, int32_t threshold, int64_t max_rpc_size) {
    std::vector<OrderedMapChunk> chunks;
    size_t current_size = base_size;
    OrderedMapChunk current;
    for (const auto &[k, v] : entries) {
        size_t pair_size = estimateOrderedValueSize(k, clientId, threshold) + 32;
        if (v != nullptr) pair_size += estimateValueSize(v, clientId, threshold);
        if (!current.keys.empty() && current_size + pair_size > static_cast<size_t>(max_rpc_size)) {
            chunks.push_back(std::move(current));
            current = OrderedMapChunk{};
            current_size = base_size;
        }
        current.keys.push_back(k);
        if (v != nullptr) current.values.push_back(v);
        current_size += pair_size;
    }
    if (!current.keys.empty()) chunks.push_back(std::move(current));
    return chunks;
}

// Maximal prefix of elements fitting into the first chunk (Java split* with base 128)
template<typename Ptr, typename SizeFn>
size_t firstChunkSplit(const std::vector<Ptr> &items, size_t base_size,
                       int64_t max_rpc_size, SizeFn size_fn) {
    size_t current_size = base_size;
    size_t i = 0;
    for (; i < items.size(); ++i) {
        size_t elem = size_fn(items[i]);
        if (current_size + elem <= static_cast<size_t>(max_rpc_size)) {
            current_size += elem;
        } else {
            break;
        }
    }
    return i;
}

std::string trimCopy(const std::string &s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

} // namespace

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
      default_ttl_(ttl),
      default_client_id_(client_id),
      compression_threshold_(defaultCompressionThreshold) {

    if (coordinator_addresses_.empty()) {
        throw std::invalid_argument("Coordinator addresses list cannot be empty");
    }

    // Periodic routing refresh (Java: scheduleAtFixedRate(init, 0, 30, SECONDS))
    refresh_thread_ = std::jthread([this](std::stop_token st) {
        while (!st.stop_requested()) {
            refreshRouting();
            for (int i = 0; i < 300 && !st.stop_requested(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    });
}

// =========================================================================
// GETTERS
// =========================================================================

std::string FastCacheSmartClient::toString() const {
    return "FastCacheSmartClient{coordinators=" + std::to_string(coordinator_addresses_.size()) + " nodes}";
}

std::string FastCacheSmartClient::getTarget() const {
    auto target = coordinator_client_->getActiveTarget();
    return target.empty() ? "UNKNOWN" : target;
}

std::chrono::milliseconds FastCacheSmartClient::getDefaultTimeout() const {
    return timeout_;
}

int32_t FastCacheSmartClient::getDefaultCompressionThreshold() const {
    return compression_threshold_;
}

FastCacheSmartClient &FastCacheSmartClient::setMode(Mode mode) {
    mode_override_.store(static_cast<int>(mode));
    return *this;
}

FastCacheSmartClient &FastCacheSmartClient::clearModeOverride() {
    mode_override_.store(-1);
    return *this;
}

Mode FastCacheSmartClient::effectiveMode() const {
    int override_mode = mode_override_.load();
    return override_mode >= 0 ? static_cast<Mode>(override_mode) : configured_mode_;
}

int32_t FastCacheSmartClient::effectiveClientId(int32_t clientId) const {
    return clientId != 0 ? clientId : default_client_id_;
}

std::chrono::milliseconds FastCacheSmartClient::effectiveTtl(std::chrono::milliseconds ttl) const {
    return ttl.count() > 0 ? ttl : default_ttl_;
}

// Java MAX_RPC_SIZE = 4MB - 4MB/8, scaled up when the configured channel
// message size is larger than the 4MB baseline.
int64_t FastCacheSmartClient::maxRpcSize() const {
    static constexpr int64_t kBase = 4LL * 1024 * 1024;
    int64_t base = std::max<int64_t>(kBase, channel_config_.max_inbound_message_size);
    return base - base / 8;
}

// =========================================================================
// CORE ROUTING
// =========================================================================

void FastCacheSmartClient::waitReady() {
    if (is_ready_.load()) return;
    std::unique_lock<std::mutex> lock(ready_mutex_);
    ready_cv_.wait_for(lock, ready_timeout_, [&] {
        return is_ready_.load() || shutdown_requested_.load();
    });
    if (is_ready_.load()) return;
    if (shutdown_requested_.load()) {
        throw std::runtime_error("SmartClient is shut down");
    }
    throw std::runtime_error("FastCacheClient boot timeout reached.");
}

void FastCacheSmartClient::refreshRouting() {
    if (shutdown_requested_.load() || coordinator_addresses_.empty()) return;
    bool expected = false;
    if (!is_updating_.compare_exchange_strong(expected, true)) {
        return; // update already in progress (Java isUpdating CAS)
    }
    struct UpdateGuard {
        std::atomic<bool> &flag;
        ~UpdateGuard() { flag.store(false); }
    } guard{is_updating_};

    const auto coord_timeout = timeout_.count() > 0 ? timeout_ : std::chrono::milliseconds(10000);
    const int32_t base = active_coordinator_index_.load();

    for (size_t attempt = 0; attempt < coordinator_addresses_.size(); ++attempt) {
        auto idx = static_cast<int32_t>((base + static_cast<int32_t>(attempt)) %
                                        static_cast<int32_t>(coordinator_addresses_.size()));
        if (attempt > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * (attempt + 1)));
        }
        try {
            auto routing_data = coordinator_client_->provideGlobalRoutingInfo(
                coordinator_addresses_[idx], coord_timeout).get();

            std::vector<coordinator::PeerRouting> peer_routings;
            peer_routings.reserve(routing_data->peer_routings.size());
            for (const auto &peer_info : routing_data->peer_routings) {
                coordinator::PeerRouting peer;
                peer.set_target(peer_info.target);
                peer.set_role(peer_info.role);
                for (const auto &partition_id : peer_info.partition_ids) {
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

            active_coordinator_index_.store(idx);
            {
                std::lock_guard<std::mutex> lock(ready_mutex_);
                is_ready_.store(true);
            }
            ready_cv_.notify_all();
            return;
        } catch (const std::exception &e) {
            std::cerr << "[COORDINATOR FAILOVER] Failed to fetch topology from "
                      << coordinator_addresses_[idx] << ": " << e.what() << std::endl;
        }
    }
    std::cerr << "[COORDINATOR FAILOVER] All coordinator nodes are unavailable!" << std::endl;
}

void FastCacheSmartClient::triggerAsyncRefresh() {
    std::thread([this] { refreshRouting(); }).detach();
}

template<typename T>
std::future<T> FastCacheSmartClient::execute(
    SharedHint hint,
    std::function<std::future<T>(SharedClient)> action) {

    auto promise = std::make_shared<std::promise<T> >();
    auto future = promise->get_future();

    std::thread([this, hint = std::move(hint), action = std::move(action), promise]() mutable {
        try {
            waitReady();

            const int32_t max_shards = routing_table_->max_shards();
            int32_t shard;
            if (hint != nullptr) {
                shard = static_cast<int32_t>(hint->weak_hash % static_cast<uint32_t>(max_shards));
            } else {
                shard = static_cast<int32_t>((random_counter_.fetch_add(1) & 0x7fffffffU) %
                                             static_cast<uint32_t>(max_shards));
            }

            auto master = routing_table_->getRoute(shard, coordinator::NodeRole::MASTER);
            auto backup = routing_table_->getRoute(shard, coordinator::NodeRole::BACKUP);

            if (!master && !backup) {
                throw std::runtime_error("No healthy endpoints available for shard allocation");
            }

            Mode mode = effectiveMode();
            if (!master) {
                mode = Mode::BACKUP;
            } else if (!backup) {
                mode = Mode::MASTER;
            }

            T value;
            switch (mode) {
                case Mode::MASTER:
                    value = attemptWithGuard<T>(shard, action, master, nullptr);
                    break;
                case Mode::BACKUP:
                    value = attemptWithGuard<T>(shard, action, backup, nullptr);
                    break;
                case Mode::LB_SMART: {
                    thread_local std::mt19937 rng(std::random_device{}());
                    const bool master_first = (rng() & 1U) == 0;
                    value = master_first
                                ? attemptWithGuard<T>(shard, action, master, backup)
                                : attemptWithGuard<T>(shard, action, backup, master);
                    break;
                }
                case Mode::MASTER_THAN_BACKUP:
                default:
                    value = attemptWithGuard<T>(shard, action, master, backup);
                    break;
            }
            promise->set_value(std::move(value));
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    }).detach();

    return future;
}

template<typename T>
T FastCacheSmartClient::attemptWithGuard(
    int shard,
    const std::function<std::future<T>(SharedClient)> &action,
    SharedClient current, SharedClient fallback) {

    try {
        return action(current).get();
    } catch (...) {
        std::exception_ptr ex = std::current_exception();

        grpc::StatusCode code = grpc::StatusCode::OK;
        std::string status_message;
        std::string route_trailer;
        bool is_grpc = false;
        try {
            std::rethrow_exception(ex);
        } catch (const GrpcStatusException &g) {
            is_grpc = true;
            code = g.code();
            status_message = g.statusMessage();
            route_trailer = g.routeTrailer();
        } catch (...) {
        }

        // 1. REROUTE (FAILED_PRECONDITION + x-fastcache-route "master,backup")
        if (is_grpc && code == grpc::StatusCode::FAILED_PRECONDITION) {
            std::string route_data = !route_trailer.empty() ? route_trailer : status_message;
            if (!trimCopy(route_data).empty()) {
                const auto comma = route_data.find(',');
                std::string master_target = trimCopy(route_data.substr(0, comma));
                std::string backup_target;
                if (comma != std::string::npos) {
                    backup_target = trimCopy(route_data.substr(comma + 1));
                }

                const std::string current_target = current ? current->getTarget() : "";

                SharedClient new_master;
                if (!master_target.empty() && master_target != current_target) {
                    new_master = routing_table_->getOrCreateByTarget(
                        master_target, channel_config_, timeout_, compression_threshold_);
                }
                SharedClient new_backup;
                if (!backup_target.empty() && backup_target != current_target) {
                    new_backup = routing_table_->getOrCreateByTarget(
                        backup_target, channel_config_, timeout_, compression_threshold_);
                } else {
                    new_backup = fallback;
                }

                std::cout << "[REROUTE] Dynamic redirect from " << current_target
                          << " -> Master: " << master_target << ", Backup: " << backup_target << std::endl;

                if (new_master) {
                    return attemptWithGuard<T>(shard, action, new_master, new_backup);
                }
                if (new_backup && new_backup != current) {
                    std::cerr << "[REROUTE FAILOVER] Master target invalid/dead. Directing straight to Backup: "
                              << new_backup->getTarget() << std::endl;
                    return attemptWithGuard<T>(shard, action, new_backup, nullptr);
                }
            }
        }

        // 2. NODE UNAVAILABLE -> topology refresh + retry on fallback
        if (is_grpc && code == grpc::StatusCode::UNAVAILABLE) {
            std::cerr << "[FAILOVER] Target node is dead: "
                      << (current ? current->getTarget() : "UNKNOWN")
                      << ". Triggering topology update..." << std::endl;
            triggerAsyncRefresh();
            if (fallback && fallback != current) {
                std::cout << "[FAILOVER] Retrying action on backup target: "
                          << fallback->getTarget() << std::endl;
                return attemptWithGuard<T>(shard, action, fallback, nullptr);
            }
        }

        std::rethrow_exception(ex);
    }
}

// =========================================================================
// CHUNK PIPELINES
// =========================================================================

std::future<KeyHint> FastCacheSmartClient::createWithRemainder(
    std::future<KeyHint> create_future,
    std::function<void(std::shared_ptr<KeyHint>)> remainder_sender) {

    auto promise = std::make_shared<std::promise<KeyHint> >();
    auto future = promise->get_future();

    std::thread([f = std::move(create_future), sender = std::move(remainder_sender), promise]() mutable {
        try {
            KeyHint result = f.get();
            if (sender) {
                auto result_hint = std::make_shared<KeyHint>(result);
                // repDelay: give replication time before pushing remaining chunks
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                sender(result_hint);
            }
            promise->set_value(result);
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    }).detach();

    return future;
}

std::future<uint32_t> FastCacheSmartClient::runAddPipeline(
    std::vector<std::function<std::future<uint32_t>()> > chunk_calls) {

    auto promise = std::make_shared<std::promise<uint32_t> >();
    auto future = promise->get_future();

    std::thread([calls = std::move(chunk_calls), promise]() mutable {
        try {
            uint32_t total = 0;
            for (auto &call : calls) {
                total += call().get();
            }
            promise->set_value(total);
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    }).detach();

    return future;
}

std::future<KeyHint> FastCacheSmartClient::createUnorderedContainerSmart(
    const Key &key, const KeyHint *hint, const std::vector<ValuePtr> *initialValue,
    std::chrono::milliseconds ttl, int32_t clientId, std::chrono::milliseconds timeout,
    ContainerType type) {

    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto owned = copyValues(initialValue);
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    const int64_t limit = maxRpcSize();

    const size_t split = firstChunkSplit<ValuePtr>(
        owned->ptrs, 128, limit,
        [&](ValuePtr v) { return estimateValueSize(v, cid, compression_threshold_); });

    std::vector<ValuePtr> first(owned->ptrs.begin(), owned->ptrs.begin() + split);
    std::vector<ValuePtr> rest(owned->ptrs.begin() + split, owned->ptrs.end());

    auto create_future = execute<KeyHint>(
        hint_copy,
        [key_copy, hint_copy, owned, first, eff_ttl, cid, timeout, type](SharedClient c) {
            const std::vector<ValuePtr> *first_ptr = first.empty() ? nullptr : &first;
            switch (type) {
                case QUEUE: return c->createQueue(*key_copy, hint_copy.get(), first_ptr, eff_ttl, cid, timeout);
                case LIST: return c->createList(*key_copy, hint_copy.get(), first_ptr, eff_ttl, cid, timeout);
                case VECTOR: return c->createVector(*key_copy, hint_copy.get(), first_ptr, eff_ttl, cid, timeout);
                case SET: return c->createSet(*key_copy, hint_copy.get(), first_ptr, eff_ttl, cid, timeout);
                default: throw std::invalid_argument("Unsupported unordered container type");
            }
        });

    if (rest.empty()) {
        return create_future;
    }

    auto rest_chunks = partitionValueChunks(rest, 0, cid, compression_threshold_, limit);
    auto remainder = [this, key_copy, owned, rest_chunks = std::move(rest_chunks), type, cid, timeout](
                         std::shared_ptr<KeyHint> res_hint) {
        for (const auto &chunk : rest_chunks) {
            execute<uint32_t>(res_hint,
                              [key_copy, res_hint, chunk, type, cid, timeout](SharedClient c) {
                                  if (type == SET) {
                                      return c->addElementUnordered(*key_copy, res_hint.get(), &chunk, cid, timeout);
                                  }
                                  return c->addElementToTail(*key_copy, res_hint.get(), &chunk, cid, timeout);
                              }).get();
        }
    };
    return createWithRemainder(std::move(create_future), std::move(remainder));
}

// =========================================================================
// LIFECYCLE
// =========================================================================

void FastCacheSmartClient::shutdown() {
    bool expected = false;
    if (!shutdown_requested_.compare_exchange_strong(expected, true)) {
        return;
    }
    if (refresh_thread_.joinable()) {
        refresh_thread_.request_stop();
        refresh_thread_.join();
    }
    {
        std::lock_guard<std::mutex> lock(ready_mutex_);
        is_ready_.store(false);
    }
    ready_cv_.notify_all();
    coordinator_client_->shutdown();
    routing_table_->shutdown();
}

FastCacheSmartClient::~FastCacheSmartClient() {
    shutdown();
}

// =========================================================================
// TTL MANAGEMENT
// =========================================================================

std::future<bool> FastCacheSmartClient::setTtl(const Key &key, const KeyHint *hint, uint64_t ttl,
                                               int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<bool>(hint_copy, [key_copy, hint_copy, ttl, cid, timeout](SharedClient c) {
        return c->setTtl(*key_copy, hint_copy.get(), ttl, cid, timeout);
    });
}

std::future<int64_t> FastCacheSmartClient::getTtl(const Key &key, const KeyHint *hint, int32_t clientId,
                                                  std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<int64_t>(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->getTtl(*key_copy, hint_copy.get(), cid, timeout);
    });
}

// =========================================================================
// KEY-VALUE OPERATIONS
// =========================================================================

std::future<ValuePtr> FastCacheSmartClient::getAndDeleteValue(const Key &key, const KeyHint *hint,
                                                              int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->getAndDeleteValue(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<KeyHint> FastCacheSmartClient::createKeyValue(const Key &key, const KeyHint *hint, const Value &value,
                                                          std::chrono::milliseconds ttl, int32_t clientId,
                                                          std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto value_copy = std::shared_ptr<Value>(copyValue(&value));
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    return execute<KeyHint>(hint_copy, [key_copy, hint_copy, value_copy, eff_ttl, cid, timeout](SharedClient c) {
        return c->createKeyValue(*key_copy, hint_copy.get(), *value_copy, eff_ttl, cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getValue(const Key &key, const KeyHint *hint, int32_t clientId,
                                                     std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->getValue(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::updateKeyValue(const Key &key, const KeyHint *hint, const Value &value,
                                                           std::chrono::milliseconds ttl, int32_t clientId,
                                                           std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto value_copy = std::shared_ptr<Value>(copyValue(&value));
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, value_copy, eff_ttl, cid, timeout](SharedClient c) {
        return c->updateKeyValue(*key_copy, hint_copy.get(), *value_copy, eff_ttl, cid, timeout);
    });
}

std::future<bool> FastCacheSmartClient::existKey(const Key &key, const KeyHint *hint, int32_t clientId,
                                                 std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<bool>(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->existKey(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<bool> FastCacheSmartClient::remove(const Key &key, const KeyHint *hint, int32_t clientId,
                                               std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<bool>(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->remove(*key_copy, hint_copy.get(), cid, timeout);
    });
}

// =========================================================================
// CONTAINER CREATION
// =========================================================================

std::future<KeyHint> FastCacheSmartClient::createQueue(const Key &key, const KeyHint *keyHint,
                                                       const std::vector<ValuePtr> *initialValue,
                                                       std::chrono::milliseconds ttl, int32_t clientId,
                                                       std::chrono::milliseconds timeout) {
    return createUnorderedContainerSmart(key, keyHint, initialValue, ttl, clientId, timeout, QUEUE);
}

std::future<KeyHint> FastCacheSmartClient::createList(const Key &key, const KeyHint *keyHint,
                                                      const std::vector<ValuePtr> *initialValue,
                                                      std::chrono::milliseconds ttl, int32_t clientId,
                                                      std::chrono::milliseconds timeout) {
    return createUnorderedContainerSmart(key, keyHint, initialValue, ttl, clientId, timeout, LIST);
}

std::future<KeyHint> FastCacheSmartClient::createVector(const Key &key, const KeyHint *keyHint,
                                                        const std::vector<ValuePtr> *initialValue,
                                                        std::chrono::milliseconds ttl, int32_t clientId,
                                                        std::chrono::milliseconds timeout) {
    return createUnorderedContainerSmart(key, keyHint, initialValue, ttl, clientId, timeout, VECTOR);
}

std::future<KeyHint> FastCacheSmartClient::createSet(const Key &key, const KeyHint *keyHint,
                                                     const std::vector<ValuePtr> *initialValue,
                                                     std::chrono::milliseconds ttl, int32_t clientId,
                                                     std::chrono::milliseconds timeout) {
    return createUnorderedContainerSmart(key, keyHint, initialValue, ttl, clientId, timeout, SET);
}

std::future<KeyHint> FastCacheSmartClient::createOrderedSet(const Key &key, const KeyHint *keyHint,
                                                            const std::vector<OrderedValuePtr> *initialValue,
                                                            std::chrono::milliseconds ttl, int32_t clientId,
                                                            std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, keyHint);
    auto owned = copyOrderedValues(initialValue);
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    const int64_t limit = maxRpcSize();

    const size_t split = firstChunkSplit<OrderedValuePtr>(
        owned->ptrs, 128, limit,
        [&](OrderedValuePtr v) { return estimateOrderedValueSize(v, cid, compression_threshold_); });

    std::vector<OrderedValuePtr> first(owned->ptrs.begin(), owned->ptrs.begin() + split);
    std::vector<OrderedValuePtr> rest(owned->ptrs.begin() + split, owned->ptrs.end());

    auto create_future = execute<KeyHint>(
        hint_copy,
        [key_copy, hint_copy, owned, first, eff_ttl, cid, timeout](SharedClient c) {
            const std::vector<OrderedValuePtr> *first_ptr = first.empty() ? nullptr : &first;
            return c->createOrderedSet(*key_copy, hint_copy.get(), first_ptr, eff_ttl, cid, timeout);
        });

    if (rest.empty()) {
        return create_future;
    }

    auto rest_chunks = partitionOrderedValueChunks(rest, 0, cid, compression_threshold_, limit);
    auto remainder = [this, key_copy, owned, rest_chunks = std::move(rest_chunks), cid, timeout](
                         std::shared_ptr<KeyHint> res_hint) {
        for (const auto &chunk : rest_chunks) {
            execute<uint32_t>(res_hint,
                              [key_copy, res_hint, chunk, cid, timeout](SharedClient c) {
                                  return c->addElementWithWeight(*key_copy, res_hint.get(), &chunk, cid, timeout);
                              }).get();
        }
    };
    return createWithRemainder(std::move(create_future), std::move(remainder));
}

std::future<KeyHint> FastCacheSmartClient::createMap(const Key &key, const KeyHint *keyHint,
                                                     const std::map<KeyPtr, ValuePtr> *initialValue,
                                                     std::chrono::milliseconds ttl, int32_t clientId,
                                                     std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, keyHint);
    auto owned = copyMap(initialValue);
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    const int64_t limit = maxRpcSize();

    const size_t split = firstChunkSplit<std::pair<KeyPtr, ValuePtr> >(
        owned->entries, 128, limit,
        [&](const std::pair<KeyPtr, ValuePtr> &e) {
            return estimateKeySize(e.first, cid, compression_threshold_) +
                   estimateValueSize(e.second, cid, compression_threshold_);
        });

    auto first_map = std::make_shared<std::map<KeyPtr, ValuePtr> >();
    for (size_t i = 0; i < split; ++i) {
        first_map->insert(owned->entries[i]);
    }
    std::vector<std::pair<KeyPtr, ValuePtr> > rest(owned->entries.begin() + split, owned->entries.end());

    auto create_future = execute<KeyHint>(
        hint_copy,
        [key_copy, hint_copy, owned, first_map, eff_ttl, cid, timeout](SharedClient c) {
            const std::map<KeyPtr, ValuePtr> *first_ptr = first_map->empty() ? nullptr : first_map.get();
            return c->createMap(*key_copy, hint_copy.get(), first_ptr, eff_ttl, cid, timeout);
        });

    if (rest.empty()) {
        return create_future;
    }

    auto rest_chunks = partitionMapChunks(rest, 0, cid, compression_threshold_, limit);
    auto remainder = [this, key_copy, owned, rest_chunks = std::move(rest_chunks), cid, timeout](
                         std::shared_ptr<KeyHint> res_hint) {
        for (const auto &chunk : rest_chunks) {
            execute<uint32_t>(res_hint,
                              [key_copy, res_hint, chunk, cid, timeout](SharedClient c) {
                                  return c->addElementHashMap(*key_copy, res_hint.get(),
                                                              &chunk.keys, &chunk.values, cid, timeout);
                              }).get();
        }
    };
    return createWithRemainder(std::move(create_future), std::move(remainder));
}

std::future<KeyHint> FastCacheSmartClient::createOrderedMap(const Key &key, const KeyHint *keyHint,
                                                            const std::map<OrderedKeyPtr, OrderedValuePtr> *initialValue,
                                                            std::chrono::milliseconds ttl, int32_t clientId,
                                                            std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, keyHint);
    auto owned = copyOrderedMap(initialValue);
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    const int64_t limit = maxRpcSize();

    const size_t split = firstChunkSplit<std::pair<OrderedKeyPtr, OrderedValuePtr> >(
        owned->entries, 128, limit,
        [&](const std::pair<OrderedKeyPtr, OrderedValuePtr> &e) {
            return estimateOrderedKeySize(e.first, cid) +
                   estimateOrderedValueSize(e.second, cid, compression_threshold_);
        });

    auto first_map = std::make_shared<std::map<OrderedKeyPtr, OrderedValuePtr> >();
    for (size_t i = 0; i < split; ++i) {
        first_map->insert(owned->entries[i]);
    }

    // Remainder goes through addElementOrderedMap which takes ordered keys as OrderedValuePtr
    std::vector<std::pair<OrderedValuePtr, ValuePtr> > rest;
    rest.reserve(owned->entries.size() - split);
    for (size_t i = split; i < owned->entries.size(); ++i) {
        rest.emplace_back(owned->key_as_value_storage[i].get(),
                          static_cast<ValuePtr>(owned->entries[i].second));
    }

    auto create_future = execute<KeyHint>(
        hint_copy,
        [key_copy, hint_copy, owned, first_map, eff_ttl, cid, timeout](SharedClient c) {
            const std::map<OrderedKeyPtr, OrderedValuePtr> *first_ptr =
                    first_map->empty() ? nullptr : first_map.get();
            return c->createOrderedMap(*key_copy, hint_copy.get(), first_ptr, eff_ttl, cid, timeout);
        });

    if (rest.empty()) {
        return create_future;
    }

    auto rest_chunks = partitionOrderedMapChunks(rest, 0, cid, compression_threshold_, limit);
    auto remainder = [this, key_copy, owned, rest_chunks = std::move(rest_chunks), cid, timeout](
                         std::shared_ptr<KeyHint> res_hint) {
        for (const auto &chunk : rest_chunks) {
            execute<uint32_t>(res_hint,
                              [key_copy, res_hint, chunk, cid, timeout](SharedClient c) {
                                  return c->addElementOrderedMap(*key_copy, res_hint.get(),
                                                                 &chunk.keys, &chunk.values, cid, timeout);
                              }).get();
        }
    };
    return createWithRemainder(std::move(create_future), std::move(remainder));
}

// =========================================================================
// CONTAINER INFO & POSITIONAL READS
// =========================================================================

std::future<ValuePtr> FastCacheSmartClient::getElementWithWeight(const Key &key, const KeyHint *hint, uint64_t pos,
                                                                 int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, pos, cid, timeout](SharedClient c) {
        return c->getElementWithWeight(*key_copy, hint_copy.get(), pos, cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveElementWithWeight(const Key &key, const KeyHint *hint,
                                                                          uint64_t pos, int32_t clientId,
                                                                          std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, pos, cid, timeout](SharedClient c) {
        return c->getAndRemoveElementWithWeight(*key_copy, hint_copy.get(), pos, cid, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::getSize(const Key &key, const KeyHint *hint, int32_t clientId,
                                                    std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<uint32_t>(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->getSize(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveFront(const Key &key, const KeyHint *hint, int32_t clientId,
                                                              std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->getAndRemoveFront(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getHead(const Key &key, const KeyHint *hint, int32_t clientId,
                                                    std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->getHead(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getTail(const Key &key, const KeyHint *hint, int32_t clientId,
                                                    std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->getTail(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getElementAtPosition(const Key &key, const KeyHint *hint, uint64_t pos,
                                                                 int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, pos, cid, timeout](SharedClient c) {
        return c->getElementAtPosition(*key_copy, hint_copy.get(), pos, cid, timeout);
    });
}

// =========================================================================
// STREAMING OPERATIONS
// =========================================================================

std::future<std::vector<ValuePtr> > FastCacheSmartClient::streamList(const Key &key, const KeyHint *hint,
                                                                     int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<std::vector<ValuePtr> >(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->streamList(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<std::vector<ValuePtr> > FastCacheSmartClient::streamVector(const Key &key, const KeyHint *hint,
                                                                       int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<std::vector<ValuePtr> >(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->streamVector(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<std::vector<ValuePtr> > FastCacheSmartClient::streamSet(const Key &key, const KeyHint *hint,
                                                                    int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<std::vector<ValuePtr> >(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->streamSet(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<std::map<KeyPtr, ValuePtr> > FastCacheSmartClient::streamMap(const Key &key, const KeyHint *hint,
                                                                         int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<std::map<KeyPtr, ValuePtr> >(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->streamMap(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<std::vector<OrderedValuePtr> > FastCacheSmartClient::streamOrderedSet(
    const Key &key, const KeyHint *hint, int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<std::vector<OrderedValuePtr> >(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->streamOrderedSet(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<std::vector<std::pair<OrderedKeyPtr, ValuePtr> > > FastCacheSmartClient::streamOrderedMap(
    const Key &key, const KeyHint *hint, int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<std::vector<std::pair<OrderedKeyPtr, ValuePtr> > >(hint_copy,
        [key_copy, hint_copy, cid, timeout](SharedClient c) {
            return c->streamOrderedMap(*key_copy, hint_copy.get(), cid, timeout);
        });
}

std::future<std::vector<ValuePtr> > FastCacheSmartClient::streamElementInRangeUnordered(
    const Key &key, const KeyHint *hint, ContainerType containerType,
    uint64_t start, uint64_t end, int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<std::vector<ValuePtr> >(hint_copy,
        [key_copy, hint_copy, containerType, start, end, cid, timeout](SharedClient c) {
            return c->streamElementInRangeUnordered(*key_copy, hint_copy.get(), containerType, start, end, cid, timeout);
        });
}

std::future<std::vector<OrderedValuePtr> > FastCacheSmartClient::streamElementInRangeOrderedSet(
    const Key &key, const KeyHint *hint, uint64_t startWeight, uint64_t endWeight,
    bool reverse, int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<std::vector<OrderedValuePtr> >(hint_copy,
        [key_copy, hint_copy, startWeight, endWeight, reverse, cid, timeout](SharedClient c) {
            return c->streamElementInRangeOrderedSet(*key_copy, hint_copy.get(), startWeight, endWeight,
                                                     reverse, cid, timeout);
        });
}

std::future<std::vector<std::pair<OrderedKeyPtr, ValuePtr> > > FastCacheSmartClient::streamElementInRangeOrderedMap(
    const Key &key, const KeyHint *hint, uint64_t startWeight, uint64_t endWeight,
    bool reverse, int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<std::vector<std::pair<OrderedKeyPtr, ValuePtr> > >(hint_copy,
        [key_copy, hint_copy, startWeight, endWeight, reverse, cid, timeout](SharedClient c) {
            return c->streamElementInRangeOrderedMap(*key_copy, hint_copy.get(), startWeight, endWeight,
                                                     reverse, cid, timeout);
        });
}

// =========================================================================
// INSERTION OPERATIONS (chunked, as in the Java smart client)
// =========================================================================

std::future<uint32_t> FastCacheSmartClient::addElementUnordered(const Key &key, const KeyHint *hint,
                                                                const std::vector<ValuePtr> *data, int32_t clientId,
                                                                std::chrono::milliseconds timeout) {
    if (data == nullptr || data->empty()) {
        std::promise<uint32_t> p;
        p.set_value(0);
        return p.get_future();
    }
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto owned = copyValues(data);
    const int32_t cid = effectiveClientId(clientId);

    const size_t base = estimateKeySize(key_copy.get(), cid, compression_threshold_) + 32;
    auto chunks = partitionValueChunks(owned->ptrs, base, cid, compression_threshold_, maxRpcSize());

    std::vector<std::function<std::future<uint32_t>()> > calls;
    calls.reserve(chunks.size());
    for (const auto &chunk : chunks) {
        calls.push_back([this, key_copy, hint_copy, owned, chunk, cid, timeout]() {
            return execute<uint32_t>(hint_copy, [key_copy, hint_copy, chunk, cid, timeout](SharedClient c) {
                return c->addElementUnordered(*key_copy, hint_copy.get(), &chunk, cid, timeout);
            });
        });
    }
    return runAddPipeline(std::move(calls));
}

std::future<uint32_t> FastCacheSmartClient::addElementWithWeight(const Key &key, const KeyHint *hint,
                                                                 const std::vector<OrderedValuePtr> *data,
                                                                 int32_t clientId, std::chrono::milliseconds timeout) {
    if (data == nullptr || data->empty()) {
        std::promise<uint32_t> p;
        p.set_value(0);
        return p.get_future();
    }
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto owned = copyOrderedValues(data);
    const int32_t cid = effectiveClientId(clientId);

    const size_t base = estimateKeySize(key_copy.get(), cid, compression_threshold_) + 32;
    auto chunks = partitionOrderedValueChunks(owned->ptrs, base, cid, compression_threshold_, maxRpcSize());

    std::vector<std::function<std::future<uint32_t>()> > calls;
    calls.reserve(chunks.size());
    for (const auto &chunk : chunks) {
        calls.push_back([this, key_copy, hint_copy, owned, chunk, cid, timeout]() {
            return execute<uint32_t>(hint_copy, [key_copy, hint_copy, chunk, cid, timeout](SharedClient c) {
                return c->addElementWithWeight(*key_copy, hint_copy.get(), &chunk, cid, timeout);
            });
        });
    }
    return runAddPipeline(std::move(calls));
}

std::future<uint32_t> FastCacheSmartClient::addElementToTail(const Key &key, const KeyHint *hint,
                                                             const std::vector<ValuePtr> *data, int32_t clientId,
                                                             std::chrono::milliseconds timeout) {
    if (data == nullptr || data->empty()) {
        std::promise<uint32_t> p;
        p.set_value(0);
        return p.get_future();
    }
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto owned = copyValues(data);
    const int32_t cid = effectiveClientId(clientId);

    const size_t base = estimateKeySize(key_copy.get(), cid, compression_threshold_) + 32;
    auto chunks = partitionValueChunks(owned->ptrs, base, cid, compression_threshold_, maxRpcSize());

    std::vector<std::function<std::future<uint32_t>()> > calls;
    calls.reserve(chunks.size());
    for (const auto &chunk : chunks) {
        calls.push_back([this, key_copy, hint_copy, owned, chunk, cid, timeout]() {
            return execute<uint32_t>(hint_copy, [key_copy, hint_copy, chunk, cid, timeout](SharedClient c) {
                return c->addElementToTail(*key_copy, hint_copy.get(), &chunk, cid, timeout);
            });
        });
    }
    return runAddPipeline(std::move(calls));
}

std::future<uint32_t> FastCacheSmartClient::addElementToHead(const Key &key, const KeyHint *hint,
                                                             const std::vector<ValuePtr> *data, int32_t clientId,
                                                             std::chrono::milliseconds timeout) {
    if (data == nullptr || data->empty()) {
        std::promise<uint32_t> p;
        p.set_value(0);
        return p.get_future();
    }
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto owned = copyValues(data);
    const int32_t cid = effectiveClientId(clientId);

    const size_t base = estimateKeySize(key_copy.get(), cid, compression_threshold_) + 32;
    auto chunks = partitionValueChunks(owned->ptrs, base, cid, compression_threshold_, maxRpcSize());

    // Java pushHead: chunks are pushed in reverse order to preserve element order
    std::vector<std::function<std::future<uint32_t>()> > calls;
    calls.reserve(chunks.size());
    for (auto it = chunks.rbegin(); it != chunks.rend(); ++it) {
        const auto chunk = *it;
        calls.push_back([this, key_copy, hint_copy, owned, chunk, cid, timeout]() {
            return execute<uint32_t>(hint_copy, [key_copy, hint_copy, chunk, cid, timeout](SharedClient c) {
                return c->addElementToHead(*key_copy, hint_copy.get(), &chunk, cid, timeout);
            });
        });
    }
    return runAddPipeline(std::move(calls));
}

std::future<uint32_t> FastCacheSmartClient::addElementToPosition(const Key &key, const KeyHint *hint,
                                                                 const std::vector<ValuePtr> *data, uint32_t pos,
                                                                 int32_t clientId, std::chrono::milliseconds timeout) {
    if (data == nullptr || data->empty()) {
        std::promise<uint32_t> p;
        p.set_value(0);
        return p.get_future();
    }
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto owned = copyValues(data);
    const int32_t cid = effectiveClientId(clientId);

    const size_t base = estimateKeySize(key_copy.get(), cid, compression_threshold_) + 32;
    auto chunks = partitionValueChunks(owned->ptrs, base, cid, compression_threshold_, maxRpcSize());

    // Java insertAt: each chunk is inserted at pos advanced by previous chunk sizes
    std::vector<std::function<std::future<uint32_t>()> > calls;
    calls.reserve(chunks.size());
    uint32_t current_pos = pos;
    for (const auto &chunk : chunks) {
        const uint32_t target_pos = current_pos;
        const uint32_t chunk_size = static_cast<uint32_t>(chunk.size());
        calls.push_back([this, key_copy, hint_copy, owned, chunk, target_pos, cid, timeout]() {
            return execute<uint32_t>(hint_copy, [key_copy, hint_copy, chunk, target_pos, cid, timeout](SharedClient c) {
                return c->addElementToPosition(*key_copy, hint_copy.get(), &chunk, target_pos, cid, timeout);
            });
        });
        current_pos += chunk_size;
    }
    return runAddPipeline(std::move(calls));
}

std::future<uint32_t> FastCacheSmartClient::addElementToPositionBefore(const Key &key, const KeyHint *hint,
                                                                       const std::vector<ValuePtr> *data,
                                                                       ValuePtr pivot, int32_t clientId,
                                                                       std::chrono::milliseconds timeout) {
    if (data == nullptr || data->empty()) {
        std::promise<uint32_t> p;
        p.set_value(0);
        return p.get_future();
    }
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto owned = copyValues(data);
    auto pivot_copy = std::shared_ptr<Value>(copyValue(pivot));
    const int32_t cid = effectiveClientId(clientId);

    const size_t base = estimateKeySize(key_copy.get(), cid, compression_threshold_) + 32;
    auto chunks = partitionValueChunks(owned->ptrs, base, cid, compression_threshold_, maxRpcSize());

    // Java insertBefore: first chunk goes before pivot, the rest chain after
    // the last element of the previous chunk
    std::vector<std::function<std::future<uint32_t>()> > calls;
    calls.reserve(chunks.size());
    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto chunk = chunks[i];
        if (i == 0) {
            calls.push_back([this, key_copy, hint_copy, owned, chunk, pivot_copy, cid, timeout]() {
                return execute<uint32_t>(hint_copy,
                                         [key_copy, hint_copy, chunk, pivot_copy, cid, timeout](SharedClient c) {
                                             return c->addElementToPositionBefore(*key_copy, hint_copy.get(), &chunk,
                                                                                  pivot_copy.get(), cid, timeout);
                                         });
            });
        } else {
            ValuePtr anchor = chunks[i - 1].back();
            calls.push_back([this, key_copy, hint_copy, owned, chunk, anchor, cid, timeout]() {
                return execute<uint32_t>(hint_copy,
                                         [key_copy, hint_copy, chunk, anchor, cid, timeout](SharedClient c) {
                                             return c->addElementToPositionAfter(*key_copy, hint_copy.get(), &chunk,
                                                                                 anchor, cid, timeout);
                                         });
            });
        }
    }
    return runAddPipeline(std::move(calls));
}

std::future<uint32_t> FastCacheSmartClient::addElementToPositionAfter(const Key &key, const KeyHint *hint,
                                                                      const std::vector<ValuePtr> *data,
                                                                      ValuePtr pivot, int32_t clientId,
                                                                      std::chrono::milliseconds timeout) {
    if (data == nullptr || data->empty()) {
        std::promise<uint32_t> p;
        p.set_value(0);
        return p.get_future();
    }
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto owned = copyValues(data);
    auto pivot_copy = std::shared_ptr<Value>(copyValue(pivot));
    const int32_t cid = effectiveClientId(clientId);

    const size_t base = estimateKeySize(key_copy.get(), cid, compression_threshold_) + 32;
    auto chunks = partitionValueChunks(owned->ptrs, base, cid, compression_threshold_, maxRpcSize());

    // Java insertAfter: each chunk goes after the last element of the previous chunk
    std::vector<std::function<std::future<uint32_t>()> > calls;
    calls.reserve(chunks.size());
    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto chunk = chunks[i];
        if (i == 0) {
            calls.push_back([this, key_copy, hint_copy, owned, chunk, pivot_copy, cid, timeout]() {
                return execute<uint32_t>(hint_copy,
                                         [key_copy, hint_copy, chunk, pivot_copy, cid, timeout](SharedClient c) {
                                             return c->addElementToPositionAfter(*key_copy, hint_copy.get(), &chunk,
                                                                                 pivot_copy.get(), cid, timeout);
                                         });
            });
        } else {
            ValuePtr anchor = chunks[i - 1].back();
            calls.push_back([this, key_copy, hint_copy, owned, chunk, anchor, cid, timeout]() {
                return execute<uint32_t>(hint_copy,
                                         [key_copy, hint_copy, chunk, anchor, cid, timeout](SharedClient c) {
                                             return c->addElementToPositionAfter(*key_copy, hint_copy.get(), &chunk,
                                                                                 anchor, cid, timeout);
                                         });
            });
        }
    }
    return runAddPipeline(std::move(calls));
}

// =========================================================================
// POP & DELETION OPERATIONS
// =========================================================================

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveTail(const Key &key, const KeyHint *hint,
                                                             int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->getAndRemoveTail(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveElementAtPosition(const Key &key, const KeyHint *hint,
                                                                          uint64_t pos, int32_t clientId,
                                                                          std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, pos, cid, timeout](SharedClient c) {
        return c->getAndRemoveElementAtPosition(*key_copy, hint_copy.get(), pos, cid, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::removeTail(const Key &key, const KeyHint *hint,
                                                       int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<uint32_t>(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->removeTail(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::removeHead(const Key &key, const KeyHint *hint,
                                                       int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<uint32_t>(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->removeHead(*key_copy, hint_copy.get(), cid, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::removeElementAtPosition(const Key &key, const KeyHint *hint,
                                                                    uint64_t pos, uint64_t endPos, int32_t clientId,
                                                                    std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<uint32_t>(hint_copy, [key_copy, hint_copy, pos, endPos, cid, timeout](SharedClient c) {
        return c->removeElementAtPosition(*key_copy, hint_copy.get(), pos, endPos, cid, timeout);
    });
}

// =========================================================================
// LOCKING OPERATIONS
// =========================================================================

std::future<LockStatus> FastCacheSmartClient::lockObject(const Key &key, const KeyHint *hint, LockType type,
                                                         int32_t clientId, std::chrono::milliseconds duration,
                                                         std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<LockStatus>(hint_copy, [key_copy, hint_copy, type, cid, duration, timeout](SharedClient c) {
        return c->lockObject(*key_copy, hint_copy.get(), type, cid, duration, timeout);
    });
}

std::future<LockStatus> FastCacheSmartClient::unlockObject(const Key &key, const KeyHint *hint, int32_t clientId,
                                                           std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<LockStatus>(hint_copy, [key_copy, hint_copy, cid, timeout](SharedClient c) {
        return c->unlockObject(*key_copy, hint_copy.get(), cid, timeout);
    });
}

// =========================================================================
// ATOMIC OPERATIONS
// =========================================================================

std::future<ValuePtr> FastCacheSmartClient::atomicExchange(const Key &key, const KeyHint *hint, int64_t value,
                                                           std::chrono::milliseconds ttl, int32_t clientId,
                                                           std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, value, eff_ttl, cid, timeout](SharedClient c) {
        return c->atomicExchange(*key_copy, hint_copy.get(), value, eff_ttl, cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::atomicAdd(const Key &key, const KeyHint *hint, int64_t delta,
                                                      std::chrono::milliseconds ttl, int32_t clientId,
                                                      std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, delta, eff_ttl, cid, timeout](SharedClient c) {
        return c->atomicAdd(*key_copy, hint_copy.get(), delta, eff_ttl, cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::atomicSub(const Key &key, const KeyHint *hint, int64_t delta,
                                                      std::chrono::milliseconds ttl, int32_t clientId,
                                                      std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, delta, eff_ttl, cid, timeout](SharedClient c) {
        return c->atomicSub(*key_copy, hint_copy.get(), delta, eff_ttl, cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::atomicAdd(const Key &key, const KeyHint *hint, double delta,
                                                      std::chrono::milliseconds ttl, int32_t clientId,
                                                      std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, delta, eff_ttl, cid, timeout](SharedClient c) {
        return c->atomicAdd(*key_copy, hint_copy.get(), delta, eff_ttl, cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::atomicSub(const Key &key, const KeyHint *hint, double delta,
                                                      std::chrono::milliseconds ttl, int32_t clientId,
                                                      std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, delta, eff_ttl, cid, timeout](SharedClient c) {
        return c->atomicSub(*key_copy, hint_copy.get(), delta, eff_ttl, cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::atomicAnd(const Key &key, const KeyHint *hint, int64_t mask,
                                                      std::chrono::milliseconds ttl, int32_t clientId,
                                                      std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, mask, eff_ttl, cid, timeout](SharedClient c) {
        return c->atomicAnd(*key_copy, hint_copy.get(), mask, eff_ttl, cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::atomicOr(const Key &key, const KeyHint *hint, int64_t mask,
                                                     std::chrono::milliseconds ttl, int32_t clientId,
                                                     std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, mask, eff_ttl, cid, timeout](SharedClient c) {
        return c->atomicOr(*key_copy, hint_copy.get(), mask, eff_ttl, cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::atomicXor(const Key &key, const KeyHint *hint, int64_t mask,
                                                      std::chrono::milliseconds ttl, int32_t clientId,
                                                      std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, mask, eff_ttl, cid, timeout](SharedClient c) {
        return c->atomicXor(*key_copy, hint_copy.get(), mask, eff_ttl, cid, timeout);
    });
}

std::future<AtomicCasRes> FastCacheSmartClient::atomicCompareAndSet(const Key &key, const KeyHint *hint,
                                                                    int64_t expectedValue, int64_t newValue,
                                                                    std::chrono::milliseconds ttl, int32_t clientId,
                                                                    std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    const auto eff_ttl = effectiveTtl(ttl);
    return execute<AtomicCasRes>(hint_copy,
                                 [key_copy, hint_copy, expectedValue, newValue, eff_ttl, cid, timeout](SharedClient c) {
                                     return c->atomicCompareAndSet(*key_copy, hint_copy.get(), expectedValue,
                                                                   newValue, eff_ttl, cid, timeout);
                                 });
}

// =========================================================================
// CONTAINER VALUE OPERATIONS
// =========================================================================

std::future<ValuePtr> FastCacheSmartClient::getContainerValue(const Key &key, const KeyHint *hint,
                                                              const Key &elementKey, int32_t clientId,
                                                              std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto element_copy = copyKey(elementKey);
    const int32_t cid = effectiveClientId(clientId);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, element_copy, cid, timeout](SharedClient c) {
        return c->getContainerValue(*key_copy, hint_copy.get(), *element_copy, cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::getAndRemoveContainerValue(const Key &key, const KeyHint *hint,
                                                                       const Key &elementKey, int32_t clientId,
                                                                       std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto element_copy = copyKey(elementKey);
    const int32_t cid = effectiveClientId(clientId);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, element_copy, cid, timeout](SharedClient c) {
        return c->getAndRemoveContainerValue(*key_copy, hint_copy.get(), *element_copy, cid, timeout);
    });
}

std::future<bool> FastCacheSmartClient::containsContainerKey(const Key &key, const KeyHint *hint,
                                                             const Key &elementKey, int32_t clientId,
                                                             std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto element_copy = copyKey(elementKey);
    const int32_t cid = effectiveClientId(clientId);
    return execute<bool>(hint_copy, [key_copy, hint_copy, element_copy, cid, timeout](SharedClient c) {
        return c->containsContainerKey(*key_copy, hint_copy.get(), *element_copy, cid, timeout);
    });
}

std::future<ValuePtr> FastCacheSmartClient::updateContainerValue(const Key &key, const KeyHint *hint,
                                                                 const Key &elementKey, const Value &value,
                                                                 int32_t clientId,
                                                                 std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto element_copy = copyKey(elementKey);
    auto value_copy = std::shared_ptr<Value>(copyValue(&value));
    const int32_t cid = effectiveClientId(clientId);
    return execute<ValuePtr>(hint_copy, [key_copy, hint_copy, element_copy, value_copy, cid, timeout](SharedClient c) {
        return c->updateContainerValue(*key_copy, hint_copy.get(), *element_copy, *value_copy, cid, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::removeFromContainer(const Key &key, const KeyHint *hint,
                                                                const Key &elementKey, int32_t clientId,
                                                                std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    auto element_copy = copyKey(elementKey);
    const int32_t cid = effectiveClientId(clientId);
    return execute<uint32_t>(hint_copy, [key_copy, hint_copy, element_copy, cid, timeout](SharedClient c) {
        return c->removeFromContainer(*key_copy, hint_copy.get(), *element_copy, cid, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::removeFromContainer(const Key &key, const KeyHint *hint,
                                                                const std::vector<KeyPtr> *keys,
                                                                const std::vector<ValuePtr> *values,
                                                                int32_t clientId, std::chrono::milliseconds timeout) {
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);
    return execute<uint32_t>(hint_copy, [key_copy, hint_copy, keys, values, cid, timeout](SharedClient c) {
        return c->removeFromContainer(*key_copy, hint_copy.get(), keys, values, cid, timeout);
    });
}

std::future<uint32_t> FastCacheSmartClient::addElementHashMap(const Key &key, const KeyHint *hint,
                                                              const std::vector<KeyPtr> *container_keys,
                                                              const std::vector<ValuePtr> *container_values,
                                                              int32_t clientId,
                                                              std::chrono::milliseconds timeout) {
    if (container_keys == nullptr || container_keys->empty()) {
        std::promise<uint32_t> p;
        p.set_value(0);
        return p.get_future();
    }
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);

    // Deep-copy keys/values into owned entries
    auto owned = std::make_shared<OwnedMapData>();
    owned->key_storage.reserve(container_keys->size());
    owned->value_storage.reserve(container_keys->size());
    owned->entries.reserve(container_keys->size());
    for (size_t i = 0; i < container_keys->size(); ++i) {
        const Key *k = container_keys->at(i);
        owned->key_storage.push_back(std::make_unique<Key>(k->size, k->data));
        const Value *v = (container_values != nullptr && i < container_values->size())
                             ? container_values->at(i)
                             : nullptr;
        owned->value_storage.push_back(copyValue(v));
        owned->entries.emplace_back(owned->key_storage.back().get(),
                                    owned->value_storage.back().get());
    }

    const size_t base = estimateKeySize(key_copy.get(), cid, compression_threshold_) + 32;
    auto chunks = partitionMapChunks(owned->entries, base, cid, compression_threshold_, maxRpcSize());

    std::vector<std::function<std::future<uint32_t>()> > calls;
    calls.reserve(chunks.size());
    for (const auto &chunk : chunks) {
        calls.push_back([this, key_copy, hint_copy, owned, chunk, cid, timeout]() {
            return execute<uint32_t>(hint_copy, [key_copy, hint_copy, chunk, cid, timeout](SharedClient c) {
                return c->addElementHashMap(*key_copy, hint_copy.get(), &chunk.keys, &chunk.values, cid, timeout);
            });
        });
    }
    return runAddPipeline(std::move(calls));
}

std::future<uint32_t> FastCacheSmartClient::addElementOrderedMap(const Key &key, const KeyHint *hint,
                                                                 const std::vector<OrderedValuePtr> *container_keys,
                                                                 const std::vector<ValuePtr> *container_values,
                                                                 int32_t clientId,
                                                                 std::chrono::milliseconds timeout) {
    if (container_keys == nullptr || container_keys->empty()) {
        std::promise<uint32_t> p;
        p.set_value(0);
        return p.get_future();
    }
    auto key_copy = copyKey(key);
    auto hint_copy = resolveHint(key, hint);
    const int32_t cid = effectiveClientId(clientId);

    // Deep-copy ordered keys and values
    auto owned_keys = copyOrderedValues(container_keys);
    auto owned_values = copyValues(container_values);
    std::vector<std::pair<OrderedValuePtr, ValuePtr> > entries;
    entries.reserve(container_keys->size());
    for (size_t i = 0; i < owned_keys->ptrs.size(); ++i) {
        ValuePtr v = i < owned_values->ptrs.size() ? owned_values->ptrs[i] : nullptr;
        entries.emplace_back(owned_keys->ptrs[i], v);
    }

    const size_t base = estimateKeySize(key_copy.get(), cid, compression_threshold_) + 32;
    auto chunks = partitionOrderedMapChunks(entries, base, cid, compression_threshold_, maxRpcSize());

    std::vector<std::function<std::future<uint32_t>()> > calls;
    calls.reserve(chunks.size());
    for (const auto &chunk : chunks) {
        calls.push_back([this, key_copy, hint_copy, owned_keys, owned_values, chunk, cid, timeout]() {
            return execute<uint32_t>(hint_copy, [key_copy, hint_copy, chunk, cid, timeout](SharedClient c) {
                return c->addElementOrderedMap(*key_copy, hint_copy.get(), &chunk.keys, &chunk.values, cid, timeout);
            });
        });
    }
    return runAddPipeline(std::move(calls));
}
