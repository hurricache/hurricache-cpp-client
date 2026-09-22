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

std::ostream &operator<<(std::ostream &os, const Key &obj) {
    print_data_safely(os, obj.data, obj.size);
    return os;
}

std::ostream &operator<<(std::ostream &os, const Value &obj) {
    print_data_safely(os, obj.data, obj.size);
    return os;
}