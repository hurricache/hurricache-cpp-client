//
// Comprehensive test runner for all HurriCache operations
// Based on Java comprehensive tests
//
#include <iostream>
#include "standalone_client.hxx"
#include "test_base.hxx"

// Forward declarations
void testListOperations(FastCacheStandaloneClient &client);
void testQueueOperations(FastCacheStandaloneClient &client);
void testVectorOperations(FastCacheStandaloneClient &client);
void testSetOperations(FastCacheStandaloneClient &client);

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int32_t port = 50000;
    
    if (argc > 1) {
        std::string target = argv[1];
        auto pos = target.find(':');
        if (pos != std::string::npos) {
            host = target.substr(0, pos);
            port = std::stoi(target.substr(pos + 1));
        }
    }
    
    std::cout << "HurriCache Comprehensive Tests\n";
    std::cout << "Target: " << host << ":" << port << "\n\n";
    std::cout.flush();
    
    try {
        std::cout << "Creating client... " << std::flush;
        FastCacheStandaloneClient client(host, port);
        std::cout << "OK\n";
        std::cout << "Client: " << client.toString() << "\n\n";
        
        // Test basic connectivity first
        std::cout << "Testing basic connectivity... " << std::flush;
        Key test_key = test_base::make_key("connectivity_test");
        
        auto exist_future = client.existKey(test_key);
        try {
            bool exists = exist_future.get();
            std::cout << "OK (key exists=" << exists << ")\n\n";
        } catch (...) {
            std::cout << "Connection test failed, but continuing...\n\n";
        }
        
        // Run all test suites
        testListOperations(client);
        testQueueOperations(client);
        testVectorOperations(client);
        testSetOperations(client);
        
        client.shutdown();
        std::cout << "\n=== ALL COMPREHENSIVE TESTS COMPLETED! ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
