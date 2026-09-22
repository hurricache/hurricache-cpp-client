//
// Created by alex on 22.09.2026.
//
#include <utils.hxx>

#include "compression.hxx"


Key *keyRequestToKey(const ::hurricache::Key &request,KeyHint* hint) {
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


OrderedKey *keyRequestToKey(const ::hurricache::OrderedKey &request,KeyHint* hint) {
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


OrderedValue *valueRequestToOrderedValue(const ::hurricache::OrderedValue &request) {
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
        auto val = new OrderedValue{order, _size, buff};
        return val;
    }

    return nullptr;
}


Value *valueRequestToValue(const ::hurricache::Value &request) {
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
        auto val = new Value{_size, buff};
        return val;
    }

    return nullptr;
}