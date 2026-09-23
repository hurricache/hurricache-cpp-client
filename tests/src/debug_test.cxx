//
// Minimal test to debug client creation
//
#include <iostream>
#include <chrono>
#include "standalone_client.hxx"

int main() {
    std::cout << "[1] Starting...\n";
    
    std::string host = "127.0.0.1";
    int32_t port = 50000;
    
    std::cout << "[2] Creating client...\n";
    FastCacheStandaloneClient client(host, port);
    std::cout << "[3] Client created: " << client.toString() << "\n";
    
    std::cout << "[4] Running simple test...\n";
    
    Key key{};
    key.size = 4;
    key.data = const_cast<char*>("test");
    
    KeyHint hint{};
    hint.weak_hash = 0;
    hint.strong_hash = 0;
    
    std::cout << "[5] Calling getTtl...\n";
    try {
        auto future = client.getTtl(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        std::cout << "[6] getTtl returned: " << result << "\n";
    } catch (const std::exception& e) {
        std::cout << "[6] Exception: " << e.what() << "\n";
    }
    
    std::cout << "[7] Shutting down...\n";
    client.shutdown();
    std::cout << "[8] Done!\n";
    
    return 0;
}
