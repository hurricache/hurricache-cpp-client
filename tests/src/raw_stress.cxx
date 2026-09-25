#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <algorithm>
#include <deque>
#include <future>
#include <numeric>

#include "standalone_client.hxx"
#include "grpcpp/create_channel.h"

struct CacheItem {
    Key key;
    KeyHint hint;
};

std::string generate_random_bytes(size_t size, std::mt19937& rng) {
    std::string data;
    data.resize(size);
    std::uniform_int_distribution<int> dist(32, 126);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<char>(dist(rng));
    }
    return data;
}

// Функция для вывода статистики по задержкам
void print_latency_stats(const std::string& phase_name, std::vector<double>& all_latencies) {
    if (all_latencies.empty()) return;

    std::sort(all_latencies.begin(), all_latencies.end());

    double min_lat = all_latencies.front();
    double max_lat = all_latencies.back();
    double sum = std::accumulate(all_latencies.begin(), all_latencies.end(), 0.0);
    double avg_lat = sum / all_latencies.size();

    size_t p50_idx = static_cast<size_t>(all_latencies.size() * 0.50);
    size_t p95_idx = static_cast<size_t>(all_latencies.size() * 0.95);
    size_t p99_idx = static_cast<size_t>(all_latencies.size() * 0.99);

    std::cout << "--- " << phase_name << " Latency Stats (microseconds) ---\n"
              << "  Min : " << min_lat << " us\n"
              << "  Avg : " << avg_lat << " us\n"
              << "  p50 : " << all_latencies[p50_idx] << " us\n"
              << "  p95 : " << all_latencies[p95_idx] << " us\n"
              << "  p99 : " << all_latencies[p99_idx] << " us\n"
              << "  Max : " << max_lat << " us\n";
}

int main() {
    std::string host = "localhost";
    int32_t port = 50000;
    int num_threads = 32;
    size_t keys_per_thread = 100000;        // Итого 1.6 миллиона ключей
    size_t target_payload_size = 5*1024;
    const size_t WINDOW_SIZE = 16;

    std::string target = host + ":" + std::to_string(port);
    std::cout << "Preparing extreme stress test with " << num_threads << " threads and window size " << WINDOW_SIZE <<" payload size: " << target_payload_size <<"...\n";

    std::vector<std::vector<std::string>> thread_raw_keys(num_threads);
    std::vector<std::vector<CacheItem>> thread_items(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        thread_raw_keys[i].reserve(keys_per_thread);
        thread_items[i].reserve(keys_per_thread);
    }

    // Хранилища латентностей по потокам
    std::vector<std::vector<double>> thread_create_latencies(num_threads);
    std::vector<std::vector<double>> thread_get_latencies(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        thread_create_latencies[i].reserve(keys_per_thread);
        thread_get_latencies[i].reserve(keys_per_thread);
    }

    // ==========================================
    // ФАЗА 1: CREATE
    // ==========================================
    std::cout << "\n--- EXTREME PHASE 1: CREATE (" << (num_threads * keys_per_thread) << " keys) ---\n";

    auto start_time = std::chrono::steady_clock::now();
    std::vector<std::thread> workers;
    workers.reserve(num_threads);

    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back([i, keys_per_thread, target_payload_size, WINDOW_SIZE, &target,
                              &thread_raw_keys, &thread_items, &thread_create_latencies]() {
            grpc::ChannelArguments args;
            args.SetInt(GRPC_ARG_MAX_CONCURRENT_STREAMS, -1);
            auto channel = grpc::CreateCustomChannel(target, grpc::InsecureChannelCredentials(), args);
            FastCacheStandaloneClient client(channel, target, std::chrono::milliseconds(3000));

            std::mt19937 rng(i + 1337);

            struct WindowItem {
                size_t idx;
                std::chrono::steady_clock::time_point start_tp;
                std::future<KeyHint> fut;
            };
            std::deque<WindowItem> window;

            size_t seq = 0;
            while (seq < keys_per_thread || !window.empty()) {
                if ((window.size() >= WINDOW_SIZE || seq >= keys_per_thread) && !window.empty()) {
                    auto& req = window.front();
                    try {
                        thread_items[i][req.idx].hint = req.fut.get();
                        auto duration = std::chrono::duration<double, std::micro>(
                            std::chrono::steady_clock::now() - req.start_tp
                        ).count();
                        thread_create_latencies[i].push_back(duration);
                    } catch (...) {}
                    window.pop_front();
                }

                if (seq < keys_per_thread && window.size() < WINDOW_SIZE) {
                    std::string raw_key_str = "t" + std::to_string(i) + "_k_" + std::to_string(seq);
                    if (raw_key_str.size() < target_payload_size) {
                        raw_key_str.append(target_payload_size - raw_key_str.size(), 'x');
                    }

                    thread_raw_keys[i].push_back(std::move(raw_key_str));
                    const std::string& stored_key = thread_raw_keys[i].back();

                    Key k(static_cast<uint32_t>(stored_key.size()), stored_key.data());

                    std::string raw_val = generate_random_bytes(target_payload_size, rng);
                    Value v(raw_val.data(), raw_val.size());

                    auto req_start = std::chrono::steady_clock::now();
                    auto fut = client.createKeyValue(k, nullptr, v,std::chrono::minutes(1));
                    thread_items[i].emplace_back(std::move(k), KeyHint{});
                    size_t idx = thread_items[i].size() - 1;

                    window.push_back({idx, req_start, std::move(fut)});
                    seq++;
                }
            }
        });
    }

    for (auto& w : workers) w.join();
    auto end_time = std::chrono::steady_clock::now();
    double create_duration = std::chrono::duration<double>(end_time - start_time).count();
    uint64_t total_created = num_threads * keys_per_thread;
    std::cout << "Create Throughput: " << (total_created / create_duration) << " req/sec (" << create_duration << "s)\n";

    // Сборка и вывод латентностей Create
    std::vector<double> all_create_lat;
    all_create_lat.reserve(total_created);
    for (auto& vec : thread_create_latencies) {
        all_create_lat.insert(all_create_lat.end(), vec.begin(), vec.end());
    }
    print_latency_stats("CREATE", all_create_lat);


    // ==========================================
    // ФАЗА 2: GET
    // ==========================================
    std::cout << "\n--- EXTREME PHASE 2: GET ---\n";
    start_time = std::chrono::steady_clock::now();
    workers.clear();

    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back([i, WINDOW_SIZE, &target, &thread_items, &thread_get_latencies]() {
            grpc::ChannelArguments args;
            args.SetInt(GRPC_ARG_MAX_CONCURRENT_STREAMS, 8000);
            auto channel = grpc::CreateCustomChannel(target, grpc::InsecureChannelCredentials(), args);
            FastCacheStandaloneClient client(channel, target, std::chrono::milliseconds(3000));

            struct WindowItem {
                std::chrono::steady_clock::time_point start_tp;
                std::future<ValuePtr> fut;
            };
            std::deque<WindowItem> window;

            size_t seq = 0;
            size_t total = thread_items[i].size();

            while (seq < total || !window.empty()) {
                if ((window.size() >= WINDOW_SIZE || seq >= total) && !window.empty()) {
                    auto& req = window.front();
                    try {
                        ValuePtr val_ptr = req.fut.get();
                        auto duration = std::chrono::duration<double, std::micro>(
                            std::chrono::steady_clock::now() - req.start_tp
                        ).count();
                        thread_get_latencies[i].push_back(duration);
                    } catch (...) {}
                    window.pop_front();
                }

                if (seq < total && window.size() < WINDOW_SIZE) {
                    auto& item = thread_items[i][seq];
                    auto req_start = std::chrono::steady_clock::now();
                    auto fut = client.getValue(item.key, &item.hint);
                    window.push_back({req_start, std::move(fut)});
                    seq++;
                }
            }
        });
    }

    for (auto& w : workers) w.join();
    end_time = std::chrono::steady_clock::now();
    double get_duration = std::chrono::duration<double>(end_time - start_time).count();
    std::cout << "Get Throughput    : " << (total_created / get_duration) << " req/sec (" << get_duration << "s)\n";

    // Сборка и вывод латентностей Get
    std::vector<double> all_get_lat;
    all_get_lat.reserve(total_created);
    for (auto& vec : thread_get_latencies) {
        all_get_lat.insert(all_get_lat.end(), vec.begin(), vec.end());
    }
    print_latency_stats("GET", all_get_lat);

    std::cout << "\n========================================\njis extreme test completed!\n";
    return 0;
}