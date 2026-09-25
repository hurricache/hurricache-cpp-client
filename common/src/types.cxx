//
// Created by alex on 22.09.2026.
//
#include <types.hxx>

std::ostream & operator<<(std::ostream &os, const KeyHint &obj) {
    return os << "KeyHint(" << obj.weak_hash << " , " << obj.strong_hash << ")";
}


inline void print_data_safely(std::ostream &os, const char *data, uint32_t size) {
    os << "{size=" << size << ", data=\"";
    if (data != nullptr && size > 0) {
        for (uint32_t i = 0; i < size; ++i) {
            auto c = static_cast<unsigned char>(data[i]);
            if (std::isprint(c)) {
                os << static_cast<char>(c);
            } else {
                os << '.';
            }
        }
    }
    os << "\"}";
}

Key::Key() : size(0), data(nullptr) {}

Key::Key(uint32_t src_size,const char* src_data) : size(src_size) {
    if (size > 0 && src_data != nullptr) {
        data = new char[size];
        std::memcpy(data, src_data, size);
    } else {
        data = nullptr;
        size = 0;
    }
}


Key::Key(const char* src_data, uint32_t src_size) : size(src_size) {
    if (size > 0 && src_data != nullptr) {
        data = new char[size];
        std::memcpy(data, src_data, size);
    } else {
        data = nullptr;
        size = 0;
    }
}
Key::~Key() {
    if (data != nullptr) {
        delete [] data;
        data = nullptr;
    }
}

Key::Key(Key &&other) noexcept : size(other.size), data(other.data) {
    other.size = 0;
    other.data = nullptr;
}
Value::Value() : size(0), data(nullptr) {}

Value::Value(const char* src_data, uint64_t src_size) : size(src_size) {
    if (size > 0 && src_data != nullptr) {
        data = new char[size];
        std::memcpy(data, src_data, size);
    } else {
        data = nullptr;
        size = 0;
    }
}
Value::Value(Value &&other) noexcept : size(other.size), data(other.data) {
    other.size = 0;
    other.data = nullptr;
}
Value::~Value() {
    if (data != nullptr) {
        delete [] data;
        data = nullptr;
    }
}

std::ostream &operator<<(std::ostream &os, const Key &obj) {
    print_data_safely(os, obj.data, obj.size);
    return os;
}

std::ostream &operator<<(std::ostream &os, const Value &obj) {
    print_data_safely(os, obj.data, obj.size);
    return os;
}