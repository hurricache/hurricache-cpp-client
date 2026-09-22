#ifndef COMPRESSION_HXX
#define COMPRESSION_HXX
#include <absl/strings/cord.h>

char *decompress(const absl::Cord& source_cord, uint32_t expectedLen) ;
char *decompress(const char *source, uint32_t sourceLen, uint32_t expectedLen);

char *compress(char *source, uint32_t sourceLen, uint32_t *destLen);
#endif
