//
// Created by alex on 22.09.2026.
//

#ifndef HURRICACHE_CPP_CLIENT_TYPES_HXX
#define HURRICACHE_CPP_CLIENT_TYPES_HXX
#include <iosfwd>
#include <ostream>
#include <stdint.h>
#include <utility>
#include <cstring>
#include <algorithm>

struct KeyHint {
    uint32_t weak_hash;
    uint32_t strong_hash;

};
std::ostream &operator<<(std::ostream &os, const KeyHint &obj);


struct Key {
    uint32_t size;
    char *data;

    Key();

    Key(uint32_t src_size, const char *src_data);

    Key(const char *src_data, uint32_t src_size);

    virtual ~Key();
    Key(const Key &) = delete;
    Key& operator=(const Key &) = delete;


    bool operator<(const Key& other) const {
        int cmp = memcmp(data, other.data, std::min(size, other.size));
        if (cmp != 0) return cmp < 0;
        return size < other.size;
    }

    friend bool operator==(const Key &lhs, const Key &rhs) {
        return lhs.size == rhs.size
               && memcmp (lhs.data, rhs.data,lhs.size) == 0;
    }

    friend bool operator!=(const Key &lhs, const Key &rhs) {
        return !(lhs == rhs);
    }
};
std::ostream &operator<<(std::ostream &os, const Key &obj);

struct Value {
    uint64_t size;
    char *data;

    Value();

    Value(const char *src_data, uint64_t src_size);

    Value(Value &&other) noexcept;

    virtual ~Value();
    Value(const Value &) = delete;
    Value& operator=(const Value &) = delete;

    friend bool operator==(const Value &lhs, const Value &rhs) {
        return lhs.size == rhs.size
            && memcmp (lhs.data, rhs.data,lhs.size) == 0;
    }

    friend bool operator!=(const Value &lhs, const Value &rhs) {
        return !(lhs == rhs);
    }
};

typedef Value* ValuePtr;
typedef Key* KeyPtr;

std::ostream &operator<<(std::ostream &os, const Value &obj);

template<typename ELEM>
struct Ordered : public ELEM{
    uint64_t weight;

    Ordered() : ELEM(), weight(0) {}

    template <typename... Args>
    Ordered(uint64_t w, Args&&... args)
        : ELEM(std::forward<Args>(args)...), weight(w) {}

    friend std::ostream & operator<<(std::ostream &os, const Ordered &obj) {
        return os << "weight: " << obj.weight << " " << static_cast<const ELEM &>(obj);
    }
    
    bool operator<(const Ordered &other) const {
        if (weight != other.weight) return weight < other.weight;
        return static_cast<const ELEM &>(*this) < static_cast<const ELEM &>(other);
    }
};

using OrderedValue = Ordered<Value>;
using OrderedValuePtr = OrderedValue*;
using OrderedKey = Ordered<Key>;
using OrderedKeyPtr = OrderedKey*;

enum LockType : uint8_t {
    NO_LOCK = 0,
    WRITE_LOCK = 1,
    READ_LOCK = 2,
    GLOBAL = 3
};

enum ContainerType : uint8_t {
    UNDEFINED = 0,
    VECTOR,
    LIST,
    QUEUE,
    SET,
    HASHMAP,
    ORDERED_MAP,
    ORDERED_SET
};

enum LockStatus {
    OK=0,
    CANT_LOCK,
    CANT_UNLOCK,
    GENERIC_ERROR
};
struct AtomicCasRes {
    bool result;
    int64_t expected;
};

#endif //HURRICACHE_CPP_CLIENT_TYPES_HXX
