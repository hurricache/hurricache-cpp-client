//
// Test runner for list operations
//
#include <iostream>
#include "standalone_client.hxx"
#include "test_base.hxx"

// Forward declarations
void testListOperations(FastCacheStandaloneClient &client);

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
    
    std::cout << "HurriCache List Operations Tests" << std::endl;
    std::cout << "Target: " << host << ":" << port << std::endl << std::endl;
    std::cout.flush();
    
    try {
        std::cout << "Creating client... " << std::flush;
        FastCacheStandaloneClient client(host, port);
        std::cout << "OK" << std::endl;
        std::cout << "Client: " << client.toString() << "\n\n";
        
        // Test basic connectivity first
        std::cout << "Testing basic connectivity... ";
        Key test_key = test_base::make_key("connectivity_test");
        KeyHint test_hint = test_base::make_hint();
        
        auto exist_future = client.existKey(test_key, test_hint, 0, std::chrono::milliseconds(2000));
        try {
            bool exists = exist_future.get();
            std::cout << "OK (key exists=" << exists << ")\n\n";
        } catch (...) {
            std::cout << "Connection test failed, but continuing...\n\n";
        }
        
        testListOperations(client);
        
        client.shutdown();
        std::cout << "\nAll tests completed!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
