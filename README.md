# HurriCache C++ Client

A high-performance C++ client for HurriCache distributed in-memory cache with ValuePtr/KeyPtr smart pointer semantics, automatic hash calculation, and comprehensive container operations.

## Table of Contents
- [Features](#features)
- [Prerequisites](#prerequisites)
- [Project Structure](#project-structure)
- [Build Instructions](#build-instructions)
- [Running Tests](#running-tests)
- [Usage Guide](#usage-guide)
- [Memory Management](#memory-management)
- [API Overview](#api-overview)
- [Container Types](#container-types)
- [Thread Safety](#thread-safety)
- [Troubleshooting](#troubleshooting)

## Features

- **Memory Ownership**: All objects are passed as raw pointers — the client never frees function arguments; the caller must free return values
- **ValuePtr/KeyPtr Semantics**: Memory allocated by caller, not automatically managed
- **Automatic Hash Calculation**: `KeyHint*` can be `nullptr` — hashes calculated automatically via `calculateKeyHint(key)`
- **Default Parameters**: Minimal boilerplate — `clientId = 0`, `timeout = milliseconds(0)` use defaults
- **Async gRPC**: Non-blocking operations using `std::future`
- **Container Support**: List, Queue, Vector, Set, Map, OrderedSet, OrderedMap
- **Streaming Operations**: Efficient batch retrieval for large containers
- **Atomic Operations**: Compare-and-swap, add, subtract, bitwise operations
- **Locking**: Read/Write/Global lock semantics with owner verification

## Prerequisites

- **CMake**: 3.20 or higher
- **C++ Compiler**: C++23 support (GCC 13+, Clang 16+, or recent MSVC)
- **Dependencies** (included in `third_party/`):
  - gRPC 1.82.1 (via git submodule)
  - Protocol Buffers 23+
  - Abseil CPP
  - OpenSSL

## Project Structure

```
hurricache-cpp-client/
├── CMakeLists.txt              # Root build configuration
├── README.md                   # This file
├── common/                     # Shared utilities
│   ├── include/
│   │   └── utils.hxx          # Hash functions, key/value builders
│   └── src/
│       └── utils.cxx
├── standalone/                 # Standalone client
│   ├── include/
│   │   └── standalone_client.hxx  # Client API declarations
│   └── src/
│       └── standalone_client.cpp  # Client implementation
├── smart/                      # Smart client (multi-node, coordinator-based)
│   ├── include/
│   └── src/
├── tests/                      # Test suites
│   ├── include/
│   │   └── test_base.hxx      # Test helpers: make_key, make_value, make_hint
│   └── src/
│       ├── comprehensive_test_runner.cxx  # All tests combined
│       ├── list_ops_test.cxx               # List operations
│       ├── test_queue_operations.cxx       # Queue operations
│       ├── test_vector_operations.cxx      # Vector operations
│       ├── test_set_operations.cxx         # Set operations
│       └── ...
├── proto/                      # Protocol buffer definitions
│   ├── cache.proto            # Main HurriCache service definition
│   └── coordinator.proto      # Coordinator service (smart client)
└── third_party/               # Vendored dependencies
    ├── grpc/
    ├── protobuf/
    └── ...
```

## Build Instructions

### Quick Build (Default Debug)

```bash
cd hurricache-cpp-client
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Build with CLion (GUI)

1. Open project root in CLion
2. CLion will automatically configure CMake
3. Build target: `hurricache_standalone_client` for library, or `list_ops_test` for tests

### Build from Command Line (Release)

```bash
cd hurricache-cpp-client
rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Build with Custom Paths

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl \
  -DgRPC_DIR=/path/to/grpc/lib/cmake/grpc
```

### Available Build Targets

- `hurricache_common` — Shared utilities library
- `hurricache_standalone_client` — Standalone client library
- `hurricache_smart_client` — Smart client for multi-node deployments
- `list_ops_test` — List operation tests
- `queue_ops_test` — Queue operation tests
- `vector_ops_test` — Vector + Set operation tests
- `comprehensive_test` — All container tests combined
- `client_smoke_test` — Legacy smoke tests

## Running Tests

### Prerequisites

Ensure HurriCache server is running:

```bash
# Pull and start server using Docker
docker pull alexaborisov/fastcache-standalone-noavx512:latest
docker run -d -p 50000:50000 alexaborisov/fastcache-standalone-noavx512:latest

# Verify server is running
docker ps | grep fastcache
```

### Run Individual Test Suites

```bash
# Navigate to build directory
cd build

# Run list operations tests
./tests/list_ops_test 127.0.0.1:50000

# Run queue operations tests
./tests/queue_ops_test 127.0.0.1:50000

# Run vector operations tests
./tests/vector_ops_test 127.0.0.1:50000

# Run comprehensive tests (all suites)
./tests/comprehensive_test 127.0.0.1:50000
```

### Command-Line Arguments

```
comprehensive_test [host:port]

Defaults:
  host = 127.0.0.1
  port = 50000
```

Examples:
```bash
# Custom server
./tests/comprehensive_test 192.168.1.100:50000

# Local server (default)
./tests/comprehensive_test
```

## Usage Guide

### Basic Setup

```cpp
#include "standalone_client.hxx"

// Create client with default parameters
FastCacheStandaloneClient client("127.0.0.1", 50000);

// Or with custom timeout and client ID
FastCacheStandaloneClient client(
    "127.0.0.1",
    50000,
    clientId: 42,
    timeout: std::chrono::milliseconds(5000)
);
```

### Key-Value Operations

```cpp
// Create a key
Key key(5,const_cast<char*>("mykey"));

// Create a value
ValuePtr value = new Value{4, const_cast<char*>("hello")};

// Set TTL
auto ttl_future = client.setTtl(key, nullptr, 60000);  // 60s TTL
bool ttl_set = ttl_future.get();

// Create key-value pair (auto-calculates hash)
auto hint_future = client.createKeyValue(key, nullptr, *value, std::chrono::milliseconds(60000));
KeyHint hint = hint_future.get();

// Get value
auto get_future = client.getValue(key, nullptr);  // nullptr hint = auto-calculate
ValuePtr result = get_future.get();

if (result && result->size > 0) {
    std::string data(result->data, result->size);
    std::cout << "Value: " << data << "\n";
}

delete result;  // Value destructor automatically frees result->data

// Update value
auto update_future = client.updateKeyValue(key, nullptr, *value, std::chrono::milliseconds(60000));
ValuePtr updated = update_future.get();

// Check existence
auto exist_future = client.existKey(key, nullptr);
bool exists = exist_future.get();

// Delete
auto remove_future = client.remove(key, nullptr);
bool removed = remove_future.get();
```

### Container Operations

#### List

```cpp
// Create list with initial data
Key key = /* ... */;
std::vector<ValuePtr> initial = {
    new Value{2, const_cast<char*>("v1")},
    new Value{2, const_cast<char*>("v2")},
    new Value{2, const_cast<char*>("v3")}
};

auto hint = client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000)).get();

// Stream all elements
auto items = client.streamList(key, nullptr).get();
for (const auto& item : items) {
    std::cout << "Item: " << std::string(item->data, item->size) << "\n";
}

// Pop operations
auto front = client.getAndRemoveFront(key, nullptr).get();
auto head = client.getHead(key, nullptr).get();
auto tail = client.getTail(key, nullptr).get();

// Add operations
std::vector<ValuePtr> data = {new Value{3, const_cast<char*>("new")}};
int added = client.addElementToTail(key, nullptr, &data).get();
int added_head = client.addElementToHead(key, nullptr, &data).get();

// Positional operations
auto elem = client.getElementAtPosition(key, nullptr, 0).get();
int size = client.getSize(key, nullptr).get();

// Remove operations
client.removeHead(key, nullptr).get();
client.removeTail(key, nullptr).get();
```

#### Queue

```cpp
// Create queue
auto hint = client.createQueue(key, nullptr, &initial).get();

// FIFO operations
auto dequeued = client.getAndRemoveFront(key, nullptr).get();
client.addElementToTail(key, nullptr, &data).get();  // Enqueue
auto front = client.getHead(key, nullptr).get();       // Peek front
```

#### Vector

```cpp
// Create vector with random access
auto hint = client.createVector(key, nullptr, &initial).get();

// Random access by index
auto elem_at_0 = client.getElementAtPosition(key, nullptr, 0).get();
auto elem_at_1 = client.getElementAtPosition(key, nullptr, 1).get();

// Insert at position
std::vector<ValuePtr> data = {new Value{5, const_cast<char*>("middle")}};
client.addElementToPosition(key, nullptr, &data, 1).get();  // Insert at index 1
```

#### Set

```cpp
// Create set
auto hint = client.createSet(key, nullptr, &initial).get();

// Add elements
std::vector<ValuePtr> data = {new Value{3, const_cast<char*>("item")}};
int added = client.addElementUnordered(key, nullptr, &data).get();

// Stream all elements
auto items = client.streamSet(key, nullptr).get();
```

### Locking Operations

```cpp
// Acquire write lock
auto lock_status = client.lockObject(
    key,
    nullptr,                    // nullptr = auto-calculate hash
    LockType::WRITE_LOCK,
    clientId: 1,
    duration: std::chrono::milliseconds(60000)  // 1 minute
).get();

if (lock_status == OK) {
    // Perform operations...
    
    // Release lock
    auto unlock_status = client.unlockObject(key, nullptr, 1).get();
}
```

### Atomic Operations

```cpp
// Atomic create
auto hint = client.atomicCreate(key, nullptr, 42, std::chrono::milliseconds(60000)).get();

// Atomic load
int64_t value = client.atomicLoad(key, nullptr).get();

// Atomic add
auto result = client.atomicAdd(key, nullptr, 10).get();  // Add 10

// Atomic compare-and-swap
AtomicCasRes cas_result = client.atomicCompareAndSet(
    key, nullptr,
    expectedValue: 42,
    newValue: 100,
    ttl: std::chrono::milliseconds(60000)
).get();

if (cas_result.success) {
    std::cout << "CAS succeeded!\n";
} else {
    std::cout << "CAS failed, expected value was: " << cas_result.expected_value << "\n";
}
```

## Memory Management

### Ownership Rules

**Critical**: This client uses raw pointers and manual memory management. Understanding ownership is essential to avoid memory leaks and double-free errors.

#### Function Arguments (Input Parameters)

**Client does NOT take ownership of function arguments.**

- All `Key*`, `Value*`, `KeyHint*`, and container pointers passed to client methods remain owned by the caller
- Client only reads/copies the data — it does NOT free the memory
- Caller is responsible for allocating and freeing all argument memory

```cpp
// ✅ CORRECT: Caller allocates and frees
Key key(5,const_cast<char*>("mykey"));
ValuePtr value = new Value{4, const_cast<char*>("hello")};

// Client copies data internally, but does NOT free 'value'
auto hint = client.createKeyValue(key, nullptr, *value, ttl).get();

// Caller must free the value
delete value;

// ❌ WRONG: Don't pass stack objects that go out of scope
void bad_example() {
    Value local_value{4, const_cast<char*>("temp")};
    
    // Dangerous! local_value might be destroyed before client finishes using it
    client.getValue(key, nullptr, &local_value);  // POTENTIAL USE-AFTER-FREE
}
```

#### Return Values (Output Parameters)

**Client DOES take ownership of return values. Caller MUST free them.**

- All methods returning `ValuePtr`, `KeyPtr`, `KeyHint`, or containers allocate memory that must be freed by the caller
- Client transfers ownership to the caller — client will NOT free these pointers
- Failure to free return values results in memory leaks

```cpp
// ✅ CORRECT: Free return value when done
ValuePtr result = client.getValue(key, nullptr).get();

if (result && result->size > 0 && result->data) {
    // Use the value
    std::string data(result->data, result->size);
    std::cout << "Value: " << data << "\n";
}

// CRITICAL: Free the memory
delete result;  // Value destructor automatically frees result->data

// ❌ WRONG: Memory leak!
ValuePtr leaked = client.getValue(key, nullptr).get();
// Forgot to delete - memory leak!
```

#### Container Elements

**Streaming operations return vectors of pointers — caller owns all memory.**

```cpp
// ✅ CORRECT: Use free_content for containers
auto items = client.streamList(key, nullptr).get();

for (const auto& item : items) {
    if (item) {
        std::cout << "Item: " << std::string(item->data, item->size) << "\n";
    }
}
free_content(items);  // Frees all ValuePtr elements
```

### Complete Memory Management Example

```cpp
#include "standalone_client.hxx"
#include <iostream>

void proper_memory_management_example() {
    FastCacheStandaloneClient client("127.0.0.1", 50000);
    
    // Step 1: Allocate input data (caller owns)
    Key key(5,const_cast<char*>("mykey"));
    
    
    ValuePtr create_value = new Value{5, const_cast<char*>("hello")};
    
    // Step 2: Create in cache (client copies data, caller still owns)
    auto hint = client.createKeyValue(key, nullptr, *create_value, 
                                      std::chrono::milliseconds(60000)).get();
    
    // Step 3: Free input value (client has copied data internally)
    delete create_value;  // Value destructor frees data
    create_value = nullptr;
    
    // Step 4: Retrieve value (client allocates, caller owns result)
    ValuePtr retrieved = client.getValue(key, nullptr).get();
    
    if (retrieved && retrieved->size > 0 && retrieved->data) {
        std::cout << "Retrieved: " << std::string(retrieved->data, retrieved->size) << "\n";
    }
    
    // Step 5: CRITICAL - Free retrieved value
    delete retrieved;  // Value destructor frees data
    retrieved = nullptr;
    
    // Step 6: Stream container (caller owns all elements)
    std::vector<ValuePtr> items = client.streamList(key, nullptr).get();
    
    for (const auto& item : items) {
        if (item) {
            std::cout << "Item: " << std::string(item->data, item->size) << "\n";
        }
    }
    free_content(items);  // Free all elements
    
    // Step 7: Cleanup key data (if dynamically allocated)
    // Note: const_cast<char*> points to string literal, don't free it!
}
```

### Memory Deallocation

**All memory allocated by the client must be freed by the caller.** Use the `free_content()` helper from `utils.hxx` for convenient cleanup.

#### Freeing Single Values

```cpp
// ✅ CORRECT: Free individual values
ValuePtr value = client.getValue(key, nullptr).get();
if (value) {
    // Use value...
    delete value;  // Value destructor automatically frees value->data
}
```

#### Freeing Container Elements (Vectors)

```cpp
// ✅ CORRECT: Use free_content for vectors
auto items = client.streamList(key, nullptr).get();

for (const auto& item : items) {
    std::cout << "Item: " << std::string(item->data, item->size) << "\n";
}

free_content(items);  // Frees all ValuePtr elements
```

#### Freeing Map Entries

```cpp
// ✅ CORRECT: Use free_content for maps
auto map_data = client.streamMap(key, nullptr).get();

for (const auto& [key, value] : map_data) {
    std::cout << "Key: " << std::string(key->data, key->size) << "\n";
    std::cout << "Value: " << std::string(value->data, value->size) << "\n";
}

free_content(map_data);  // Frees both keys and values
```

#### Freeing Ordered Containers

```cpp
// ✅ CORRECT: Use free_content for ordered containers
auto ordered_items = client.streamOrderedSet(key, nullptr).get();

for (const auto& item : ordered_items) {
    std::cout << "Weight: " << item->weight << "\n";
}

free_content(ordered_items);  // Frees all OrderedValuePtr elements
```

### Memory Management Quick Reference

| Parameter Type | Direction | Owner | Free By |
|----------------|-----------|-------|---------|
| `const Key &key` | Input | Caller | Caller |
| `const KeyHint *hint` | Input (optional) | Caller | Caller (if allocated) |
| `const Value &value` | Input | Caller | Caller |
| `ValuePtr` (return) | Output | Caller | **Caller** (`delete result;`) |
| `KeyPtr` (return) | Output | Caller | **Caller** (`delete result;`) |
| `std::vector<ValuePtr>` | Output | Caller | **Caller** (`free_content(container);`) |
| `std::map<KeyPtr, ValuePtr>` | Output | Caller | **Caller** (`free_content(map);`) |
| `std::vector<Key>` | Input | Caller | Caller |

### Common Pitfalls

#### 1. Memory Leak — Forgetting to Free Return Values

```cpp
// ❌ WRONG: Memory leak
void leak_example() {
    ValuePtr val = client.getValue(key).get();
    // Forgot to delete val!
}

// ✅ CORRECT: Always free
void safe_example() {
    ValuePtr val = client.getValue(key).get();
    if (val) {
        // Use val...
        delete val;
    }
}
```

#### 2. Use-After-Free — Modifying Freed Memory

```cpp
// ❌ WRONG: Use-after-free
void uaf_example() {
    ValuePtr val = client.getValue(key).get();
    delete val;
    
    // BUG: Using freed memory!
    std::cout << val->size << "\n";  // UNDEFINED BEHAVIOR
}

// ✅ CORRECT: Use before freeing
void safe_example() {
    ValuePtr val = client.getValue(key).get();
    if (val) {
        std::cout << val->size << "\n";  // Safe
        delete val;
    }
}
```

#### 3. Double-Free — Freeing Same Memory Twice

```cpp
// ❌ WRONG: Double-free
void double_free_example() {
    ValuePtr val = client.getValue(key).get();
    delete val;
    
    // BUG: Double-free!
    delete val;  // UNDEFINED BEHAVIOR
}

// ✅ CORRECT: Nullify after free
void safe_example() {
    ValuePtr val = client.getValue(key).get();
    if (val) {
        // Use val...
        delete val;
        val = nullptr;  // Prevent double-free
    }
}
```

#### 4. Passing Temporary Pointers

```cpp
// ❌ WRONG: Temporary value destroyed
void temporary_example() {
    Value temp{5, const_cast<char*>("temp")};
    
    // Dangerous: temp destroyed at end of scope
    client.createKeyValue(key, nullptr, temp, ttl).get();
    // Client may still reference 'temp' data!
}

// ✅ CORRECT: Allocate on heap
void safe_example() {
    ValuePtr temp = new Value{5, const_cast<char*>("temp")};
    
    client.createKeyValue(key, nullptr, *temp, ttl).get();
    
    delete temp;  // Safe: client has copied data
}
```

### Best Practices

1. **Always check for nullptr** before using return values
2. **Free immediately after use** — don't store pointers beyond their lifetime
3. **Use RAII wrappers** in your code (e.g., `std::unique_ptr` with custom deleter)
4. **Document ownership** in your code comments
5. **Use valgrind** or AddressSanitizer to detect leaks:
   ```bash
   valgrind --leak-check=full ./tests/comprehensive_test
   # or
   g++ -fsanitize=address -g your_program.cpp
   ```

## API Overview

### Key Types

```cpp
// Key: represents cache key with binary data
struct Key {
    uint32_t size;
    char* data;
};

// KeyHint: contains hash values for efficient lookup
struct KeyHint {
    uint32_t weak_hash;   // Fast hash for initial lookup
    uint32_t strong_hash; // Cryptographic hash for verification
};

// Value: represents cached value with binary data
struct Value {
    uint64_t size;
    char* data;
};

// Smart pointer aliases
using ValuePtr = Value*;
using KeyPtr = Key*;
using OrderedValuePtr = OrderedValue*;
using OrderedKeyPtr = OrderedKey*;
```

### Memory Management

⚠️ **See [Memory Management](#memory-management) section above for detailed ownership rules.**

**Quick reference:**
- Function arguments: Caller owns memory, client does NOT free
- Return values (`ValuePtr`, `KeyPtr`, containers): Caller owns memory, MUST free
- Container elements from streaming: Caller owns all elements, MUST free each

```cpp
// Allocate on heap
ValuePtr value = new Value{size, data};

// Use value
auto result = client.getValue(key).get();

// Free when done
delete result;  // Value destructor automatically frees data
```

### Default Parameters

All methods support minimal calling style:
```cpp
// Full signature
client.createList(key, hint, &initial, ttl, clientId, timeout);

// With defaults (recommended)
client.createList(key);                              // Create empty
client.createList(key, nullptr, &initial);           // With data, auto-hash
client.createList(key, nullptr, &initial, ttl);      // Custom TTL

// Timeout defaults to client's defaultTimeout_
// ClientId defaults to 0
// KeyHint nullptr triggers automatic hash calculation
```

## Container Types

| Container | Ordered | Random Access | Duplicates | Key Methods |
|-----------|---------|---------------|------------|-------------|
| **List** | FIFO | Yes (by index) | Yes        | `getHead`, `getTail`, `getAndRemoveFront` |
| **Queue** | FIFO | No | Yes        | `getAndRemoveFront`, `addElementToTail` |
| **Vector** | Indexed | Yes | Yes        | `getElementAtPosition`, `addElementToPosition` |
| **Set** | Unordered | No | No         | `addElementUnordered`, `streamSet` |
| **Map** | Unordered | By key | No         | `streamMap`, `getContainerValue` |
| **OrderedSet** | By weight | Yes | Yes        | `addElementWithWeight`, `streamOrderedSet` |
| **OrderedMap** | By weight | By key | Yes        | `createOrderedMap`, `streamOrderedMap` |

**Note:** For ordered containers, the key is a combination of weight and data key. Duplicates are allowed when keys have different weights. 

## Thread Safety

- **Client Instance**: Not thread-safe. Create one instance per thread or use synchronization.
- **gRPC Channel**: Thread-safe for concurrent reads/writes.
- **Async Operations**: `std::future` results can be retrieved from any thread.

```cpp
// Thread-safe usage pattern
void worker_thread(int client_id) {
    FastCacheStandaloneClient client("127.0.0.1", 50000, client_id);
    
    // Use client in this thread
    auto result = client.getValue(key).get();
    
    // Shutdown when done
    client.shutdown();
}

std::thread t1(worker_thread, 1);
std::thread t2(worker_thread, 2);
t1.join();
t2.join();
```

## Troubleshooting

### Build Errors

**Problem**: `c++23 not supported`
```bash
# Use GCC 13+ or Clang 16+
g++ --version
# Or specify compiler explicitly
cmake .. -DCMAKE_CXX_COMPILER=g++-13
```

**Problem**: `gRPC not found`
```bash
# Ensure third_party dependencies are initialized
git submodule update --init --recursive
```

### Runtime Errors

**Problem**: `Connection refused`
```bash
# Check if server is running
netstat -an | grep 50000
# Or
telnet 127.0.0.1 50000
```

**Problem**: `Key already exist or incorrect type`
```cpp
// Use unique keys for each test
Key key = test_base::make_key("unique_key_" + std::to_string(uuid()));

// Or ensure previous key is removed
client.remove(key, nullptr).get();
```

**Problem**: `Segmentation fault`
```cpp
// Always check for nullptr returns
ValuePtr result = client.getValue(key).get();
if (result && result->size > 0 && result->data) {
    // Safe to use
}
```

### Test Failures

**Problem**: Tests fail with "gRPC call failed"
- Ensure HurriCache server is running on correct port
- Check network connectivity
- Verify no key collisions (use UUID-based keys in tests)

**Problem**: `NOT_FOUND` errors
- Key doesn't exist or was deleted
- Container type mismatch (e.g., calling List operations on Queue)

### Debug Logging

Client outputs debug info to stderr:
```
[DEBUG] Creating channel to 127.0.0.1:50000
[DEBUG] Channel created, creating stub
[DEBUG] Stub created, starting completion queue thread
```

## License

Proprietary - HurriCache Project

## Support

For issues and questions:
- Create an issue in the project repository
- Check existing documentation in `/proto/` for gRPC service definitions
- Review test suites in `/tests/src/` for usage examples
