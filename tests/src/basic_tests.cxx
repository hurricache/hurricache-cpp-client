//
// Main test runner - includes all test modules
//
#include <iostream>
#include <chrono>
#include <future>

#include "standalone_client.hxx"

// Test module declarations
void testTtlManagement(FastCacheStandaloneClient &client);
void testKeyValueOperations(FastCacheStandaloneClient &client);
void testContainerCreation(FastCacheStandaloneClient &client);
void testContainerInfoBoundaryPositionalPop(FastCacheStandaloneClient &client);
void testStreamingOperations(FastCacheStandaloneClient &client);
void testInsertionDeletion(FastCacheStandaloneClient &client);
void testLockingAtomicContainerValue(FastCacheStandaloneClient &client);

int main(int argc, char* argv[]) {
    // Use explicit host and port
    std::string host = "127.0.0.1";
    int32_t port = 50000;
    
    if (argc > 1) {
        // Parse "host:port" from first argument
        std::string target = argv[1];
        auto pos = target.find(':');
        if (pos != std::string::npos) {
            host = target.substr(0, pos);
            port = std::stoi(target.substr(pos + 1));
        }
    }
    
    std::cout << "HurriCache C++ Client Tests\n";
    std::cout << "===========================\n";
    std::cout << "Host: " << host << "\n";
    std::cout << "Port: " << port << "\n\n";
    
    try {
        // Create client with explicit host and port
        FastCacheStandaloneClient client(host, port);
        std::cout << "Client created: " << client.toString() << "\n\n";
        
        // Run test modules
        // testTtlManagement(client);
        testKeyValueOperations(client);
        // testContainerCreation(client);
        // testContainerInfoBoundaryPositionalPop(client);
        // testStreamingOperations(client);
        // testInsertionDeletion(client);
        // testLockingAtomicContainerValue(client);
        
        client.shutdown();
        
        std::cout << "===========================\n";
        std::cout << "All test modules completed!\n";
        std::cout << "===========================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
