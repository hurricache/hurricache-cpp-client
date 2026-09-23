//
// Universal test runner for all HurriCache operations
// Runs all test suites sequentially
//
#include <iostream>
#include <string>
#include <chrono>
#include <functional>
#include <vector>
#include "standalone_client.hxx"
#include "test_base.hxx"

// Forward declarations - basic tests module
void testTtlManagement(FastCacheStandaloneClient &client);
void testKeyValueOperations(FastCacheStandaloneClient &client);
void testContainerCreation(FastCacheStandaloneClient &client);
void testContainerInfoBoundaryPositionalPop(FastCacheStandaloneClient &client);
void testStreamingOperations(FastCacheStandaloneClient &client);
void testInsertionDeletion(FastCacheStandaloneClient &client);
void testLockingAtomicContainerValue(FastCacheStandaloneClient &client);

// Forward declarations - list/queue/vector/set tests module
void testListOperations(FastCacheStandaloneClient &client);
void testQueueOperations(FastCacheStandaloneClient &client);
void testVectorOperations(FastCacheStandaloneClient &client);
void testSetOperations(FastCacheStandaloneClient &client);



struct TestResult {
    std::string name;
    bool passed;
    std::string message;
};

static TestResult runTestSuite(const std::string& name, std::function<void(FastCacheStandaloneClient&)> test_func, FastCacheStandaloneClient& client) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Running: " << name << "\n";
    std::cout << std::string(60, '=') << "\n";
    
    try {
        test_func(client);
        return {name, true, "PASSED"};
    } catch (const std::exception& e) {
        return {name, false, "FAILED - " + std::string(e.what())};
    } catch (...) {
        return {name, false, "FAILED - unknown error"};
    }
}

static void printSummary(const std::vector<TestResult>& results) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "TEST SUMMARY\n";
    std::cout << std::string(60, '=') << "\n\n";
    
    int total = static_cast<int>(results.size());
    int passed = 0;
    
    for (const auto& result : results) {
        std::string status = result.passed ? "[PASS]" : "[FAIL]";
        std::cout << status << " | " << result.name;
        if (!result.message.empty() && result.message != "PASSED") {
            std::cout << " - " << result.message;
        }
        std::cout << "\n";
        if (result.passed) passed++;
    }
    
    std::cout << "\n" << std::string(60, '-') << "\n";
    std::cout << "Total: " << total << " | Passed: " << passed << " | Failed: " << (total - passed) << "\n";
    std::cout << std::string(60, '=') << "\n";
}

int main(int argc, char* argv[]) {
    auto start_time = std::chrono::steady_clock::now();
    
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
    
    std::cout << "HurriCache C++ Client - Universal Test Runner\n";
    std::cout << "Target: " << host << ":" << port << "\n";
    
    std::vector<TestResult> all_results;
    
    try {
        // Create client
        std::cout << "\nCreating client... ";
        FastCacheStandaloneClient client(host, port);
        std::cout << "OK\n";
        std::cout << "Client: " << client.toString() << "\n";
        
        // Test basic connectivity
        std::cout << "\nTesting connectivity... ";
        Key test_key = test_base::make_key("universal_runner_test");
        auto exist_future = client.existKey(test_key);
        try {
            bool exists = exist_future.get();
            std::cout << "OK (key exists=" << exists << ")\n";
        } catch (...) {
            std::cout << "WARNING: Server unavailable, tests will be skipped\n";
        }
        
        // Define test suites
        std::vector<std::pair<std::string, std::function<void(FastCacheStandaloneClient&)>>> test_suites = {
            // Basic tests module
            {"TTL Management", testTtlManagement},
            {"Key-Value Operations", testKeyValueOperations},
            {"Container Creation", testContainerCreation},
            {"Container Info & Boundary/Positional Pop", testContainerInfoBoundaryPositionalPop},
            {"Streaming Operations", testStreamingOperations},
            {"Insertion/Deletion", testInsertionDeletion},
            {"Locking/Atomic Container Value", testLockingAtomicContainerValue},
            
            // List/Queue/Vector/Set tests
            {"List Operations", testListOperations},
            {"Queue Operations", testQueueOperations},
            {"Vector Operations", testVectorOperations},
            {"Set Operations", testSetOperations},
        };
        
        // Run all test suites
        for (const auto& [suite_name, test_func] : test_suites) {
            auto result = runTestSuite(suite_name, test_func, client);
            all_results.push_back(result);
        }
        
        client.shutdown();
        
    } catch (const std::exception& e) {
        std::cerr << "\nFatal error: " << e.what() << "\n";
        printSummary(all_results);
        return 1;
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    printSummary(all_results);
    
    std::cout << "\nTotal execution time: " << duration.count() << " ms\n";
    
    // Return non-zero if any tests failed
    bool all_passed = std::all_of(all_results.begin(), all_results.end(), 
                                  [](const TestResult& r) { return r.passed; });
    
    return all_passed ? 0 : 1;
}
