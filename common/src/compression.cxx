#include <compression.hxx>

#ifdef GZIP
#include <zlib.h>
#else
#include <libdeflate.h>

#endif


static thread_local libdeflate_compressor *shared_compressor = nullptr;
static thread_local libdeflate_decompressor *dec = nullptr;
#ifdef GZIP
char *gzip_decompress(char *source, uint32_t sourceLen, uint32_t expectedLen) {
    z_stream stream;

    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = (uInt) sourceLen;
    stream.next_in = (Bytef *) source;

    // Allocate the destination buffer based on expected size
    char *dest = new char[expectedLen];
    if (!dest)
        return NULL;

    stream.avail_out = (uInt) expectedLen;
    stream.next_out = (Bytef *) dest;

    // Window bits: 15 + 16 (or 31) to decode GZip format specifically
    int windowBits = 15 + 16;
#ifdef __PREFETCH
__builtin_prefetch (stream.next_in);
__builtin_prefetch (stream.next_out);
#endif

if (inflateInit2(&stream, windowBits) != Z_OK)
    {
        delete[] dest;
        return NULL;
    }

int ret = inflate(&stream, Z_FINISH);
inflateEnd (&stream);

    if (ret!= Z_STREAM_END)
    {
        delete[] dest;
        return NULL;
    }

    return dest;
}
#endif
#ifdef GZIP
char *gzip_compress(char *source, uint32_t sourceLen, uint32_t *destLen) {
    z_stream stream;

    // Initialize stream struct
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = (uInt) sourceLen;
    stream.next_in = (Bytef *) source;

    // Window bits: 15 is default.
    // Adding 16 to the window bits tells zlib to write a GZip header/trailer.
    int windowBits = 15 + 16;
    int memLevel = 8; // Default memory usage
#ifdef __PREFETCH
__builtin_prefetch (stream.next_in);
__builtin_prefetch (stream.next_out);
#endif

if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
windowBits, memLevel, Z_DEFAULT_STRATEGY) != Z_OK)
    {
        return NULL;
    }

// GZip has a header and trailer; bound gives us the worst-case size
size_t maxDestLen = deflateBound(&stream, sourceLen);
char *dest = new char[maxDestLen];

    if (!dest)
    {
        delete[] dest;
        return NULL;
    }

stream.avail_out= (uInt)maxDestLen;
stream.next_out= (Bytef *)dest;

// Perform compression
int ret = deflate(&stream, Z_FINISH);

    if (ret!= Z_STREAM_END)
    {
        delete[] dest;
        deflateEnd(&stream);
        return NULL;
    }

// Final compressed size
    *destLen= stream.total_out;

deflateEnd (&stream);
    return dest;
}
#endif
char *gzip_decompress_libdeflate(const char *source, uint32_t sourceLen, uint32_t expectedLen) {
    char *dest = new char[expectedLen];
    if (!dest)
        return nullptr;

    if (!dec) {
        dec = libdeflate_alloc_decompressor();
    }

    libdeflate_result result = libdeflate_gzip_decompress(
        dec,
        source,
        sourceLen,
        dest,
        expectedLen,
        nullptr
    );

    if ( result != LIBDEFLATE_SUCCESS)[[unlikely]] {
        delete []dest;
        return nullptr;
    }

    return dest;
}

char *gzip_decompress_libdeflate(const absl::Cord& source_cord, uint32_t expectedLen) {
    uint32_t sourceLen = source_cord.size();
    if (sourceLen == 0) return nullptr;

    char *dest = new char[expectedLen];
    if (!dest) return nullptr;

    if (!dec) {
        dec = libdeflate_alloc_decompressor();
    }

    absl::optional<absl::string_view> flat = source_cord.TryFlat();
    const char* source_ptr = nullptr;
    char* temp_buffer = nullptr;

    if (flat.has_value()) {
        source_ptr = flat->data();
    } else {
        temp_buffer = new char[sourceLen];
        if (!temp_buffer) {
            delete []dest;
            return nullptr;
        }
        source_ptr = temp_buffer;
        char* write_ptr = temp_buffer;
        for (absl::string_view chunk : source_cord.Chunks()) {
            memcpy(write_ptr, chunk.data(), chunk.size());
            write_ptr += chunk.size();
        }
    }

    libdeflate_result result = libdeflate_gzip_decompress(
        dec,
        source_ptr,
        sourceLen,
        dest,
        expectedLen,
        nullptr
    );

    if (temp_buffer != nullptr)[[unlikely]] {
        delete []temp_buffer;
    }

    if (result != LIBDEFLATE_SUCCESS)[[unlikely]] {
        delete []dest;
        return nullptr;
    }

    return dest;
}

char *gzip_compress_libdeflate(const char *source, uint32_t sourceLen, uint32_t *destLen) {
    if (!shared_compressor) {
        shared_compressor = libdeflate_alloc_compressor(1);
    }
    size_t maxDestLen = libdeflate_gzip_compress_bound(shared_compressor, sourceLen);

    char *dest = new char[maxDestLen];

    if (!dest)
        return nullptr;

    size_t actualSize = libdeflate_gzip_compress(
        shared_compressor,
        source,
        sourceLen,
        dest,
        maxDestLen);

    if (actualSize == 0)[[unlikely]] {

        delete []dest;
        return nullptr;
    }

    *destLen = static_cast<uint32_t>(actualSize);
    return dest;
}

#ifndef GZIP
char *decompress(const absl::Cord& source_cord, uint32_t expectedLen) {
    return gzip_decompress_libdeflate(source_cord, expectedLen);
}
#endif

char *decompress(const char *source, uint32_t sourceLen, uint32_t expectedLen) {
#ifdef GZIP
    return gzip_decompress(source, sourceLen, expectedLen);
#else
    return gzip_decompress_libdeflate(source, sourceLen, expectedLen);
#endif
}

char *compress(char *source, uint32_t sourceLen, uint32_t *destLen) {
#ifdef GZIP
    return gzip_compress(source, sourceLen, destLen);
#else
    return gzip_compress_libdeflate(source, sourceLen, destLen);
#endif
}
