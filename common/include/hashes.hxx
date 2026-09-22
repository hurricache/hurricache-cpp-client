//
// Created by alex on 19.04.2026.
//

#ifndef FASTCACHE_HASHES_HXX
#define FASTCACHE_HASHES_HXX
#include <cstdint>

uint32_t week_hash(const char *data, uint32_t size, uint64_t seed = 0);

uint32_t strong_hash(const char *data, uint32_t size);

uint64_t crc32_accumulate(const char* data, size_t size, uint64_t seed = 0);
uint32_t crc32_finalize(uint64_t crc, size_t total_size);

#endif //FASTCACHE_HASHES_HXX
