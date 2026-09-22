//
// Created by alex on 19.04.2026.
//
#include <cstring>
#include <hashes.hxx>
#include <smmintrin.h>


__attribute__((always_inline)) uint32_t strong_hash(const char *data, uint32_t size) {
    if (data == nullptr)[[unlikely]] return 0;
    uint32_t crc = 0;
    const char *sample = data;
    uint32_t tail = size;
    while (tail >= 8) {
        long long val;
        std::memcpy(&val, sample, 8);
        crc = _mm_crc32_u64(crc, val);
        tail -= 8;
        sample += 8;
    }

    if (tail >= 4) {
        int val;
        std::memcpy(&val, sample, 4);
        crc = _mm_crc32_u32(crc, val);
        tail -= 4;
        sample += 4;
    }
    if (tail >= 2) {
        short val;
        std::memcpy(&val, sample, 2);
        crc = _mm_crc32_u16(crc, val);
        tail -= 2;
        sample += 2;
    }
    if (tail >= 1) {
        crc = _mm_crc32_u8(crc, *sample);
    }

    crc ^= size;
    crc ^= crc >> 16;
    crc *= 0x85ebca77;
    crc ^= crc >> 13;
    crc *= 0xc2b2ae3d;
    crc ^= crc >> 16;

    return crc;
}

static constexpr uint64_t Prime_0 = 0xEC997CE353AD73EDull;
static constexpr uint64_t Prime_1 = 0xBE1F145728F2090Dull;

inline uint64_t rot64(uint64_t v, int s) {
    return (v >> s) | (v << (64 - s));
}

inline uint64_t mux64(uint64_t v, uint64_t p, uint64_t &upper) {
#if defined(__SIZEOF_INT128__)
    unsigned __int128 r = (unsigned __int128) v * p;
    upper = (uint64_t) (r >> 64);
    return (uint64_t) r;
#else
    // Fallback for compilers without 128-bit int
    uint64_t l = (v & 0xFFFFFFFF) * (p & 0xFFFFFFFF);
    uint64_t m1 = (v >> 32) * (p & 0xFFFFFFFF);
    uint64_t m2 = (v & 0xFFFFFFFF) * (p >> 32);
    uint64_t h = (v >> 32) * (p >> 32);
    uint64_t mm = m1 + m2 + (l >> 32);
    upper = h + (mm >> 32);
    return (l & 0xFFFFFFFF) | (mm << 32);
#endif
}

__attribute__((always_inline)) uint32_t week_hash(const char *data, uint32_t len, uint64_t seed) {
    if (data == nullptr)[[unlikely]] return 0;
    auto ptr = (const uint8_t *) data;

    uint64_t a = seed ^ Prime_0;
    uint64_t b = len ^ Prime_1;

    while (len >= 32)[[likely]] {
        uint64_t v0, v1, v2, v3;
        std::memcpy(&v0, ptr, 8);
        std::memcpy(&v1, ptr + 8, 8);
        std::memcpy(&v2, ptr + 16, 8);
        std::memcpy(&v3, ptr + 24, 8);

        uint64_t h0, h1, h2, h3;
        b += mux64(v0 ^ a, Prime_0, h0); a ^= h0;
        b += mux64(v1 ^ a, Prime_1, h1); a ^= h1;
        b += mux64(v2 ^ a, Prime_0, h2); a ^= h2;
        b += mux64(v3 ^ a, Prime_1, h3); a ^= h3;

        ptr += 32;
        len -= 32;
    }

    while (len >= 8) {
        uint64_t val;
        std::memcpy(&val, ptr, sizeof(uint64_t));
        uint64_t high;
        uint64_t low = mux64(val ^ a, Prime_0, high);
        a ^= high;
        b += low;
        ptr += 8;
        len -= 8;
    }

    if (len >= 4) {
        uint32_t tail;
        std::memcpy(&tail, ptr, sizeof(uint32_t));
        b ^= tail;
        ptr += 4;
        len -= 4;
    }

    if (len >= 2) {
        uint16_t tail;
        std::memcpy(&tail, ptr, sizeof(uint16_t));
        b ^= tail;
        ptr += 2;
        len -= 2;
    }

    if (len >= 1) {
        uint8_t tail = *ptr;
        b ^= tail;
    }

    uint64_t final_high;
    uint64_t final_low = mux64(a ^ rot64(b, 17), Prime_1, final_high);

    uint64_t res64 = final_low ^ final_high;
    return (uint32_t)(res64 ^ (res64 >> 32));
}


__attribute__((always_inline)) uint64_t crc32_accumulate(const char* data, size_t size, uint64_t seed) {
    uint64_t crc = seed;
    const char* sample = data;
    size_t tail = size;

    // Читаем по 8 байт
    while (tail >= 8) {
        uint64_t val;
        std::memcpy(&val, sample, sizeof(val));
        crc = _mm_crc32_u64(crc, val);
        sample += 8;
        tail -= 8;
    }

    // Хвосты
    if (tail >= 4) {
        uint32_t val;
        std::memcpy(&val, sample, sizeof(val));
        crc = _mm_crc32_u32(static_cast<uint32_t>(crc), val);
        sample += 4;
        tail -= 4;
    }
    if (tail >= 2) {
        uint16_t val;
        std::memcpy(&val, sample, sizeof(val));
        crc = _mm_crc32_u16(static_cast<uint32_t>(crc), val);
        sample += 2;
        tail -= 2;
    }
    if (tail >= 1) {
        crc = _mm_crc32_u8(static_cast<uint32_t>(crc), *sample);
    }

    return crc;
}


__attribute__((always_inline)) uint32_t crc32_finalize(uint64_t crc, size_t total_size) {
    auto result = static_cast<uint32_t>(crc);

    result ^= static_cast<uint32_t>(total_size);
    result ^= result >> 16;
    result *= 0x85ebca77u;
    result ^= result >> 13;
    result *= 0xc2b2ae3du;
    result ^= result >> 16;

    return result;
}