#include <benchmark/benchmark.h>
#include "../include/market_data.h"
#include <vector>
#include <random>

using namespace hft_straddle::data;

// Benchmark market tick creation
static void BM_MarketTickCreation(benchmark::State& state) {
    for (auto _ : state) {
        MarketTick tick;
        tick.timestamp = Timestamp::now();
        tick.symbol_id = 1;
        tick.bid = Price(100.0);
        tick.ask = Price(100.05);
        tick.last = Price(100.025);
        tick.volume = 1000;
        
        benchmark::DoNotOptimize(tick);
    }
}
BENCHMARK(BM_MarketTickCreation);

// Benchmark midpoint calculation
static void BM_MidpointCalculation(benchmark::State& state) {
    MarketTick tick;
    tick.bid = Price(100.0);
    tick.ask = Price(100.05);
    
    for (auto _ : state) {
        Price midpoint = tick.midpoint();
        benchmark::DoNotOptimize(midpoint);
    }
}
BENCHMARK(BM_MidpointCalculation);

// Benchmark spread calculation
static void BM_SpreadCalculation(benchmark::State& state) {
    MarketTick tick;
    tick.bid = Price(100.0);
    tick.ask = Price(100.05);
    
    for (auto _ : state) {
        double spread = tick.spread_pct();
        benchmark::DoNotOptimize(spread);
    }
}
BENCHMARK(BM_SpreadCalculation);

// Benchmark timestamp creation
static void BM_TimestampCreation(benchmark::State& state) {
    for (auto _ : state) {
        Timestamp ts = Timestamp::now();
        benchmark::DoNotOptimize(ts);
    }
}
BENCHMARK(BM_TimestampCreation);

// Benchmark option tick processing
static void BM_OptionTickProcessing(benchmark::State& state) {
    OptionTick option_tick;
    strcpy(option_tick.symbol, "AAPL");
    option_tick.expiration_date = 20251220;
    option_tick.strike_price = Price(150.0);
    option_tick.option_type = OptionType::CALL;
    option_tick.bid = Price(5.50);
    option_tick.ask = Price(5.60);
    option_tick.last_price = Price(5.55);
    option_tick.volume = 500;
    option_tick.implied_volatility = 0.25;
    option_tick.delta = 0.6;
    option_tick.gamma = 0.05;
    option_tick.theta = -0.02;
    option_tick.vega = 0.15;
    
    for (auto _ : state) {
        // Simulate processing option tick
        double intrinsic_value = std::max(0.0, option_tick.last_price.value() - option_tick.strike_price.value());
        double time_value = option_tick.last_price.value() - intrinsic_value;
        
        benchmark::DoNotOptimize(intrinsic_value);
        benchmark::DoNotOptimize(time_value);
    }
}
BENCHMARK(BM_OptionTickProcessing);

// Benchmark straddle quote construction
static void BM_StraddleQuoteConstruction(benchmark::State& state) {
    for (auto _ : state) {
        StraddleQuote straddle;
        strcpy(straddle.underlying_symbol, "AAPL");
        straddle.underlying_price = Price(150.0);
        straddle.expiration_date = 20251220;
        straddle.strike_price = Price(150.0);
        
        straddle.call_option.bid = Price(5.50);
        straddle.call_option.ask = Price(5.60);
        straddle.call_option.last_price = Price(5.55);
        
        straddle.put_option.bid = Price(5.40);
        straddle.put_option.ask = Price(5.50);
        straddle.put_option.last_price = Price(5.45);
        
        straddle.straddle_price = Price(straddle.call_option.last_price.value() + 
                                       straddle.put_option.last_price.value());
        
        benchmark::DoNotOptimize(straddle);
    }
}
BENCHMARK(BM_StraddleQuoteConstruction);

// Benchmark bulk market data processing
static void BM_BulkMarketDataProcessing(benchmark::State& state) {
    const size_t num_ticks = state.range(0);
    std::vector<MarketTick> ticks;
    ticks.reserve(num_ticks);
    
    // Generate test data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dist(99.0, 101.0);
    std::uniform_int_distribution<> volume_dist(100, 10000);
    
    for (size_t i = 0; i < num_ticks; ++i) {
        MarketTick tick;
        tick.timestamp = Timestamp::now();
        tick.symbol_id = static_cast<uint32_t>(i % 100);
        tick.bid = Price(price_dist(gen));
        tick.ask = Price(tick.bid.value() + 0.05);
        tick.last = Price((tick.bid.value() + tick.ask.value()) / 2.0);
        tick.volume = volume_dist(gen);
        ticks.push_back(tick);
    }
    
    for (auto _ : state) {
        double total_value = 0.0;
        for (const auto& tick : ticks) {
            total_value += tick.midpoint().value() * tick.volume;
        }
        benchmark::DoNotOptimize(total_value);
    }
}
BENCHMARK(BM_BulkMarketDataProcessing)->Range(1000, 100000);

// Benchmark memory allocation patterns
static void BM_DataEventAllocation(benchmark::State& state) {
    for (auto _ : state) {
        DataEvent event;
        event.type = DataEventType::MARKET_TICK;
        event.timestamp = Timestamp::now();
        event.market_tick.symbol_id = 1;
        event.market_tick.bid = Price(100.0);
        event.market_tick.ask = Price(100.05);
        event.market_tick.last = Price(100.025);
        event.market_tick.volume = 1000;
        
        benchmark::DoNotOptimize(event);
    }
}
BENCHMARK(BM_DataEventAllocation);

// Benchmark cache-aligned memory access
static void BM_CacheAlignedAccess(benchmark::State& state) {
    const size_t num_events = 1000;
    std::vector<DataEvent> events(num_events);
    
    // Initialize events
    for (size_t i = 0; i < num_events; ++i) {
        events[i].type = DataEventType::MARKET_TICK;
        events[i].timestamp = Timestamp::now();
        events[i].market_tick.symbol_id = static_cast<uint32_t>(i);
        events[i].market_tick.bid = Price(100.0 + i * 0.01);
        events[i].market_tick.ask = Price(100.05 + i * 0.01);
        events[i].market_tick.last = Price(100.025 + i * 0.01);
        events[i].market_tick.volume = 1000 + i;
    }
    
    for (auto _ : state) {
        uint64_t total_volume = 0;
        for (const auto& event : events) {
            total_volume += event.market_tick.volume;
        }
        benchmark::DoNotOptimize(total_volume);
    }
}
BENCHMARK(BM_CacheAlignedAccess);

BENCHMARK_MAIN();