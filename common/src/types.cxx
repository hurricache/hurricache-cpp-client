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
Value::Value() : size(0), data(nullptr),type(BYTES) {}

Value::Value(const char* src_data, uint64_t src_size) : size(src_size),type(BYTES) {
    if (size > 0 && src_data != nullptr) {
        data = new char[size];
        memcpy(data, src_data, size);
    } else {
        data = nullptr;
        size = 0;
    }
}

Value::Value(const int64_t src_int):type(INT),int_value(src_int),size(sizeof(uint64_t)) {
}

Value::Value(const double src_double):type(DOUBLE),double_value(src_double),size(sizeof(double)) {
}

Value::Value(Value &&other) noexcept : size(other.size), type(other.type) {
    if (type == BYTES) {
        data = other.data;
    } else {
        int_value = other.int_value; // покрывает и int_value, и double_value через union
    }
    other.size = 0;
    other.type = BYTES;
    other.data = nullptr;
}

Value::~Value() {
    if (type == BYTES && data != nullptr) {
        delete [] data;
        data = nullptr;
    }
}

std::ostream &operator<<(std::ostream &os, const Key &obj) {
    print_data_safely(os, obj.data, obj.size);
    return os;
}

std::ostream &operator<<(std::ostream &os, const Value &obj) {
    switch (obj.type) {
        case INT:
            os << "{type=INT, val=" << obj.int_value << "}";
            break;
        case DOUBLE:
            os << "{type=DOUBLE, val=" << obj.double_value << "}";
            break;
        case BYTES:
            os << "{type=BYTES, ";
            print_data_safely(os, obj.data, obj.size);
            os << "}";
            break;
    }
    return os;
}