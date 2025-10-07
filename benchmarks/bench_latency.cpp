#include <benchmark/benchmark.h>
#include "../include/market_data.h"
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>

using namespace hft_straddle::data;

// Benchmark timestamp creation latency
static void BM_TimestampLatency(benchmark::State& state) {
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        Timestamp ts = Timestamp::now();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        benchmark::DoNotOptimize(latency);
    }
}
BENCHMARK(BM_TimestampLatency);

// Benchmark end-to-end market data processing latency
static void BM_MarketDataProcessingLatency(benchmark::State& state) {
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Simulate complete market data processing pipeline
        MarketTick tick;
        tick.timestamp = Timestamp::now();
        tick.symbol_id = 1;
        tick.bid = Price(100.0);
        tick.ask = Price(100.05);
        tick.last = Price(100.025);
        tick.volume = 1000;
        
        // Process the tick
        Price midpoint = tick.midpoint();
        double spread = tick.spread_pct();
        
        // Simulate strategy decision
        bool should_trade = (spread < 0.01) && (tick.volume > 500);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        benchmark::DoNotOptimize(latency);
        benchmark::DoNotOptimize(should_trade);
    }
}
BENCHMARK(BM_MarketDataProcessingLatency);

// Benchmark option pricing calculation latency
static void BM_OptionPricingLatency(benchmark::State& state) {
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Simplified Black-Scholes calculation
        double S = 150.0;  // Stock price
        double K = 150.0;  // Strike price
        double T = 0.25;   // Time to expiration (3 months)
        double r = 0.02;   // Risk-free rate
        double sigma = 0.25; // Volatility
        
        // Calculate d1 and d2
        double d1 = (std::log(S/K) + (r + 0.5*sigma*sigma)*T) / (sigma*std::sqrt(T));
        double d2 = d1 - sigma*std::sqrt(T);
        
        // Approximate normal CDF (for benchmarking purposes)
        auto norm_cdf = [](double x) {
            return 0.5 * (1.0 + std::erf(x / std::sqrt(2.0)));
        };
        
        // Calculate call price
        double call_price = S * norm_cdf(d1) - K * std::exp(-r*T) * norm_cdf(d2);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        benchmark::DoNotOptimize(call_price);
        benchmark::DoNotOptimize(latency);
    }
}
BENCHMARK(BM_OptionPricingLatency);

// Benchmark memory allocation latency
static void BM_MemoryAllocationLatency(benchmark::State& state) {
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Allocate and deallocate market data structures
        auto tick = std::make_unique<MarketTick>();
        tick->timestamp = Timestamp::now();
        tick->symbol_id = 1;
        tick->bid = Price(100.0);
        tick->ask = Price(100.05);
        tick->last = Price(100.025);
        tick->volume = 1000;
        
        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        benchmark::DoNotOptimize(latency);
    }
}
BENCHMARK(BM_MemoryAllocationLatency);

// Benchmark cache miss latency
static void BM_CacheMissLatency(benchmark::State& state) {
    const size_t size = state.range(0);
    std::vector<uint64_t> data(size);
    
    // Initialize data
    for (size_t i = 0; i < size; ++i) {
        data[i] = i;
    }
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        uint64_t sum = 0;
        // Access data with large strides to cause cache misses
        for (size_t i = 0; i < size; i += 64) {  // 64-element stride
            sum += data[i];
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        benchmark::DoNotOptimize(sum);
        benchmark::DoNotOptimize(latency);
    }
}
BENCHMARK(BM_CacheMissLatency)->Range(1024, 1024*1024);

// Benchmark atomic operations latency
static void BM_AtomicOperationsLatency(benchmark::State& state) {
    std::atomic<uint64_t> counter{0};
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        counter.fetch_add(1, std::memory_order_relaxed);
        uint64_t value = counter.load(std::memory_order_relaxed);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        benchmark::DoNotOptimize(value);
        benchmark::DoNotOptimize(latency);
    }
}
BENCHMARK(BM_AtomicOperationsLatency);

// Benchmark thread context switching latency
static void BM_ThreadContextSwitchLatency(benchmark::State& state) {
    std::atomic<bool> flag1{false};
    std::atomic<bool> flag2{false};
    
    std::thread worker([&]() {
        while (true) {
            while (!flag1.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            flag1.store(false, std::memory_order_release);
            flag2.store(true, std::memory_order_release);
        }
    });
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        flag1.store(true, std::memory_order_release);
        while (!flag2.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        flag2.store(false, std::memory_order_release);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        benchmark::DoNotOptimize(latency);
    }
    
    worker.detach();  // Let it terminate when main exits
}
BENCHMARK(BM_ThreadContextSwitchLatency);

// Benchmark function call overhead
static void BM_FunctionCallOverhead(benchmark::State& state) {
    auto simple_function = [](double a, double b) -> double {
        return a + b;
    };
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        double result = simple_function(100.0, 0.05);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(latency);
    }
}
BENCHMARK(BM_FunctionCallOverhead);

// Benchmark SIMD operations latency
static void BM_SIMDOperationsLatency(benchmark::State& state) {
    alignas(32) double data1[4] = {100.0, 100.1, 100.2, 100.3};
    alignas(32) double data2[4] = {0.05, 0.05, 0.05, 0.05};
    alignas(32) double result[4];
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Simple vectorized addition (compiler should optimize to SIMD)
        for (int i = 0; i < 4; ++i) {
            result[i] = data1[i] + data2[i];
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(latency);
    }
}
BENCHMARK(BM_SIMDOperationsLatency);

// Benchmark system call latency
static void BM_SystemCallLatency(benchmark::State& state) {
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Use a lightweight system call
        auto current_time = std::chrono::high_resolution_clock::now();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        benchmark::DoNotOptimize(current_time);
        benchmark::DoNotOptimize(latency);
    }
}
BENCHMARK(BM_SystemCallLatency);

// Benchmark end-to-end trading decision latency
static void BM_TradingDecisionLatency(benchmark::State& state) {
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Complete trading decision pipeline
        MarketTick tick;
        tick.timestamp = Timestamp::now();
        tick.symbol_id = 1;
        tick.bid = Price(149.95);
        tick.ask = Price(150.05);
        tick.last = Price(150.00);
        tick.volume = 5000;
        
        // Market analysis
        double spread = tick.spread_pct();
        double midpoint = tick.midpoint().value();
        bool sufficient_volume = tick.volume > 1000;
        bool tight_spread = spread < 0.01;
        
        // Risk analysis
        double position_size = 10000.0;  // $10k position
        double portfolio_value = 1000000.0;  // $1M portfolio
        double position_concentration = position_size / portfolio_value;
        bool risk_acceptable = position_concentration < 0.05;  // Max 5% per position
        
        // Trading decision
        bool should_trade = sufficient_volume && tight_spread && risk_acceptable;
        
        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        benchmark::DoNotOptimize(should_trade);
        benchmark::DoNotOptimize(latency);
    }
}
BENCHMARK(BM_TradingDecisionLatency);

BENCHMARK_MAIN();