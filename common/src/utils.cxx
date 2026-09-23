//
// Created by alex on 22.09.2026.
//
#include <utils.hxx>
#include "compression.hxx"

// =========================================================================
// Proto builders (used by standalone client)
// =========================================================================

::hurricache::Key buildKeyProto(const Key &key, const KeyHint *hint, int32_t clientId) {
    ::hurricache::Key proto_key;
    auto *payload = proto_key.mutable_payload();
    payload->set_size(key.size);
    payload->mutable_payload()->assign(key.data, key.size);

    if (hint != nullptr) {
        auto *key_hint = proto_key.mutable_keyhint();
        key_hint->set_week_hash(hint->weak_hash);
        key_hint->set_strong_hash(hint->strong_hash);
    }
    proto_key.set_clientid(clientId);
    return proto_key;
}

::hurricache::GetRequest buildGetRequestProto(const Key &key, const KeyHint *hint, int32_t clientId) {
    ::hurricache::GetRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    return request;
}

::hurricache::Value buildValueProto(const Value &value, std::chrono::milliseconds ttl, int32_t clientId) {
    ::hurricache::Value proto_value;
    auto *payload = proto_value.mutable_value();
    payload->set_size(static_cast<uint32_t>(value.size));
    payload->set_payload(absl::string_view(value.data, static_cast<size_t>(value.size)));

    if (ttl.count() > 0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        proto_value.set_ttl(static_cast<uint64_t>(now_ms) + static_cast<uint64_t>(ttl.count()));
    }

    auto *lock_info = proto_value.mutable_lock_info();
    lock_info->set_type(static_cast<hurricache::LockType>(NO_LOCK));
    lock_info->set_lockedby(clientId);

    return proto_value;
}

::hurricache::AtomicCreate buildAtomicCreateProto(const Key &key, const KeyHint *hint, int32_t clientId,
                                                  int64_t value, std::chrono::milliseconds ttl) {
    hurricache::AtomicCreate request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    auto *av = request.mutable_val();
    av->set_val(value);
    if (ttl.count() > 0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request.set_ttl(static_cast<uint64_t>(now_ms) + static_cast<uint64_t>(ttl.count()));
    }
    return request;
}

::hurricache::ContainerGetRequest buildContainerGetRequestProto(const Key &key, const KeyHint *hint, int32_t clientId,
                                                                const Key &elementKey) {
    hurricache::ContainerGetRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    auto *ek = request.mutable_element_key();
    ek->mutable_payload()->set_size(elementKey.size);
    ek->mutable_payload()->mutable_payload()->assign(elementKey.data, elementKey.size);
    return request;
}

::hurricache::KeyPositionRequest buildPositionRequestProto(const Key &key, const KeyHint *hint, int32_t clientId,
                                                           int32_t pos) {
    hurricache::KeyPositionRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_pos(static_cast<uint64_t>(pos));
    return request;
}


Key *keyRequestToKey(const ::hurricache::Key &request, KeyHint *hint) {
    if (!request.has_payload()) [[unlikely]] {
        return nullptr;
    }
    const auto &key_binary_payload = request.payload();
    const auto &data = key_binary_payload.payload();

    if (key_binary_payload.size() != data.size()) [[unlikely]] {
        return nullptr;
    }

    uint32_t size = 0;
    uint32_t _strong_hash = UINT32_MAX;
    uint32_t _weak_hash = UINT32_MAX;
    char *payload = nullptr;
    if (request.has_keyhint()) {
        auto has_strong_hash = request.keyhint().has_strong_hash();
        auto has_week_hash = request.keyhint().has_week_hash();
        if (has_strong_hash) _strong_hash = request.keyhint().strong_hash();
        if (has_week_hash) _weak_hash = request.keyhint().week_hash();
    }

    if (request.has_compressioninfo())[[unlikely]] {
        const auto &cInfo = request.compressioninfo();
        size = cInfo.rawsize();
        payload = decompress(const_cast<char *>(data.data()), key_binary_payload.size(),
                             size);
    } else {
        size = key_binary_payload.size();
        payload = const_cast<char *>(data.data());
    }
    if (payload != nullptr)[[likely]] {
        if (hint != nullptr) {
            hint->strong_hash = _strong_hash;
            hint->weak_hash = _weak_hash;
        }
        auto key = new Key{size, payload};
        return key;
    }
    return nullptr;
}


OrderedKey *keyRequestToKey(const ::hurricache::OrderedKey &request, KeyHint *hint) {
    uint32_t size = 0;
    uint32_t _strong_hash = UINT32_MAX;
    uint32_t _weak_hash = UINT32_MAX;
    char *payload = nullptr;
    if (request.has_keyhint()) {
        auto has_strong_hash = request.keyhint().has_strong_hash();
        auto has_week_hash = request.keyhint().has_week_hash();
        if (has_strong_hash) _strong_hash = request.keyhint().strong_hash();
        if (has_week_hash) _weak_hash = request.keyhint().week_hash();
    }

    if (request.has_compressioninfo())[[unlikely]] {
        const auto &cInfo = request.compressioninfo();
        size = cInfo.rawsize();
        payload = decompress(const_cast<char *>(request.payload().payload().data()), request.payload().size(),
                             size);
    } else {
        payload = const_cast<char *>(request.payload().payload().data());
        size = request.payload().size();
    }
    if (payload != nullptr)[[likely]] {
        auto key = new OrderedKey{request.order(), size, payload};
        if (hint != nullptr) {
            hint->strong_hash = _strong_hash;
            hint->weak_hash = _weak_hash;
        }
        return key;
    } else {
        return nullptr;
    }
}


OrderedValuePtr valueRequestToOrderedValue(const ::hurricache::OrderedValue &request) {
    uint32_t _rawSize = 0;
    bool compressed = false;
    if (request.has_compressioninfo()) {
        compressed = request.compressioninfo().enabled();
        _rawSize = request.compressioninfo().rawsize();
    }

    const auto &payload = request.value().payload();
    auto _size = payload.size();

    uint64_t order = 0;
    if (request.has_order()) {
        order = request.order();
    }

    char *buff = nullptr;
    bool success = false;

    // Проверяем, является ли Cord плоским (flat / состоит ровно из одного чанка нужного размера)
    auto chunks = payload.Chunks();
    auto it = chunks.begin();
    bool is_flat = false;
    const char *flat_data = nullptr;

    if (it != chunks.end()) {
        const auto &first_chunk = *it;
        auto next_it = it;
        ++next_it;
        if (next_it == chunks.end() && first_chunk.size() == _size) {
            is_flat = true;
            flat_data = static_cast<const char *>(first_chunk.data());
        }
    }

    if (compressed && is_flat) {
        char *raw = decompress(flat_data, _size, _rawSize);
        if (raw != nullptr) {
            _size = _rawSize;
            buff = raw;
            success = true;
        } else {
            return nullptr;
        }
    } else {
        buff = new char[_size];
        char *write_ptr = buff;
        size_t real_size = 0;

        if (is_flat && !compressed) {
            memcpy(buff, flat_data, _size);
            real_size = _size;
        } else {
            for (const auto &chunk: chunks) {
                size_t chunk_size = chunk.size();
                memcpy(write_ptr, chunk.data(), chunk_size);
                write_ptr += chunk_size;
                real_size += chunk_size;
            }
        }

        if (real_size == _size) [[likely]] {
            if (compressed) {
                char *raw = decompress(buff, _size, _rawSize);
                if (raw != nullptr) {
                    delete []buff;
                    _size = _rawSize;
                    buff = raw;
                } else {
                    delete []buff;
                    return nullptr;
                }
            }
            success = (buff != nullptr);
        } else {
            delete []buff;
            return nullptr;
        }
    }

    if (success && buff != nullptr) [[likely]] {
        auto val = new OrderedValue();
        val->weight = order;
        val->size = _size;
        val->data = buff;
        return val;
    }

    return nullptr;
}


ValuePtr valueRequestToValue(const ::hurricache::Value &request) {
    uint32_t _rawSize = 0;
    bool compressed = false;
    if (request.has_compressioninfo()) {
        compressed = request.compressioninfo().enabled();
        _rawSize = request.compressioninfo().rawsize();
        compressed = true;
    }

    const auto &payload = request.value().payload();
    auto _size = payload.size();


    char *buff = nullptr;
    bool success = false;

    auto chunks = payload.Chunks();
    auto it = chunks.begin();
    bool is_flat = false;
    const char *flat_data = nullptr;

    if (it != chunks.end()) {
        const auto &first_chunk = *it;
        auto next_it = it;
        ++next_it;
        if (next_it == chunks.end() && first_chunk.size() == _size) {
            is_flat = true;
            flat_data = static_cast<const char *>(first_chunk.data());
        }
    }

    if (compressed && is_flat) {
        char *raw = decompress(flat_data, _size, _rawSize);
        if (raw != nullptr) {
            _size = _rawSize;
            buff = raw;
            success = true;
        } else {
            return nullptr;
        }
    } else {
        buff = new char [_size];
        char *write_ptr = buff;
        size_t real_size = 0;

        if (is_flat && !compressed) {
            memcpy(buff, flat_data, _size);
            real_size = _size;
        } else {
            for (const auto &chunk: chunks) {
                std::size_t chunk_size = chunk.size();
                memcpy(write_ptr, chunk.data(), chunk_size);
                real_size += chunk_size;
                write_ptr += chunk_size;
            }
        }

        if (real_size == _size) [[likely]] {
            if (compressed) {
                char *raw = decompress(buff, _size, _rawSize);
                if (raw != nullptr) {
                    delete []buff;
                    _size = _rawSize;
                    buff = raw;
                } else {
                    delete []buff;
                    return nullptr;
                }
            }
            success = (buff != nullptr);
        } else {
            delete []buff;
            return nullptr;
        }
    }

    if (success && buff != nullptr) [[likely]] {
        auto val = new Value();
        val->data = buff;
        val->size = _size;
        return val;
    }

    return nullptr;
}

hurricache::CreateContainerRequest buildContainerRequest(
    const Key &key, const KeyHint *hint, int32_t clientId,
    hurricache::ContainerType type, std::chrono::milliseconds ttl,
    const std::vector<ValuePtr> *values) {
    hurricache::CreateContainerRequest request;
    *request.mutable_key() = buildKeyProto(key, hint, clientId);
    request.set_type(type);

    if (ttl.count() > 0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request.set_ttl(static_cast<uint64_t>(now_ms) + static_cast<uint64_t>(ttl.count()));
    }
    if (values != nullptr) {
        for (const auto &v: *values) {
            *request.add_value_unordered() = buildValueProtoNoTtl(v, clientId);
        }
    }
    return request;
}


hurricache::CreateContainerRequest buildContainerRequestOrdered(
    const Key &key, const KeyHint *hint, int32_t clientId,
    hurricache::ContainerType type, std::chrono::milliseconds ttl,
    const std::vector<OrderedValuePtr> *values) {
    hurricache::CreateContainerRequest request;
    auto effective_hint = hint == nullptr ? calculateKeyHint(key) : hint;
    *request.mutable_key() = buildKeyProto(key, effective_hint, clientId);
    request.set_type(type);

    if (ttl.count() > 0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request.set_ttl(static_cast<uint64_t>(now_ms) + static_cast<uint64_t>(ttl.count()));
    }

    for (const auto &val: *values) {
        auto *ordered_val = request.add_value_ordered();
        ordered_val->set_order(val->weight);
        auto *ov_val = ordered_val->mutable_value();
        ov_val->set_size(static_cast<uint32_t>(val->size));
        ov_val->set_payload(absl::string_view(val->data, static_cast<size_t>(val->size)));
    }

    return request;
}
