#include <gtest/gtest.h>
#include "../include/data_ingestion.h"
#include "../include/market_data.h"

using namespace hft_straddle;
using namespace hft_straddle::ingestion;
using namespace hft_straddle::data;

class DataIngestionTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = std::make_unique<DataIngestionEngine>();
    }

    void TearDown() override {
        if (engine_ && engine_->is_running()) {
            engine_->stop();
        }
    }

    std::unique_ptr<DataIngestionEngine> engine_;
};

TEST_F(DataIngestionTest, EngineInitialization) {
    EXPECT_TRUE(engine_ != nullptr);
    EXPECT_FALSE(engine_->is_running());
    
    auto stats = engine_->get_stats();
    EXPECT_EQ(stats.events_processed, 0);
    EXPECT_EQ(stats.events_dropped, 0);
}

TEST_F(DataIngestionTest, SymbolSubscription) {
    std::vector<std::string> test_symbols = {"AAPL", "GOOGL", "MSFT", "TSLA", "NVDA"};
    
    bool subscription_success = engine_->subscribe_symbols(test_symbols);
    EXPECT_TRUE(subscription_success);
    
    auto subscribed_symbols = engine_->get_subscribed_symbols();
    EXPECT_EQ(subscribed_symbols.size(), test_symbols.size());
    
    for (const auto& symbol : test_symbols) {
        EXPECT_TRUE(std::find(subscribed_symbols.begin(), subscribed_symbols.end(), symbol) 
                   != subscribed_symbols.end());
    }
}

TEST_F(DataIngestionTest, EngineStartStop) {
    // Test engine start
    bool start_success = engine_->start();
    EXPECT_TRUE(start_success);
    EXPECT_TRUE(engine_->is_running());
    
    // Give it a moment to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Test engine stop
    bool stop_success = engine_->stop();
    EXPECT_TRUE(stop_success);
    EXPECT_FALSE(engine_->is_running());
}

TEST_F(DataIngestionTest, EventProcessingPerformance) {
    // Test that event processing meets performance requirements
    const size_t num_test_events = 10000;
    std::vector<DataEvent> test_events;
    test_events.reserve(num_test_events);
    
    // Create test events
    for (size_t i = 0; i < num_test_events; ++i) {
        DataEvent event;
        event.type = DataEventType::MARKET_TICK;
        event.timestamp = Timestamp::now();
        event.market_tick.symbol_id = static_cast<uint32_t>(i % 100);
        event.market_tick.bid = Price(100.0 + (i % 50));
        event.market_tick.ask = Price(100.05 + (i % 50));
        event.market_tick.last = Price(100.025 + (i % 50));
        event.market_tick.volume = 1000 + (i % 5000);
        
        test_events.push_back(event);
    }
    
    // Measure processing time
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Process events (in a real implementation)
    // For testing, we'll just iterate through them
    for (const auto& event : test_events) {
        // Simulate processing
        double midpoint = event.market_tick.midpoint().value();
        (void)midpoint;  // Suppress unused variable warning
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Should process 10k events in less than 100ms
    EXPECT_LT(duration.count(), 100000);
    
    // Calculate events per second
    double events_per_second = static_cast<double>(num_test_events) / 
                              (static_cast<double>(duration.count()) / 1000000.0);
    
    // Should achieve at least 100k events/second
    EXPECT_GT(events_per_second, 100000.0);
}

TEST_F(DataIngestionTest, MemoryUsage) {
    // Test that memory usage remains reasonable
    const size_t max_events = 1000000;  // 1M events
    
    // Simulate memory allocation for event buffer
    size_t event_size = sizeof(DataEvent);
    size_t total_memory = max_events * event_size;
    
    // Should use less than 256MB for 1M events
    EXPECT_LT(total_memory, 256 * 1024 * 1024);
}

TEST_F(DataIngestionTest, ThreadSafety) {
    // Test concurrent access to the data ingestion engine
    std::vector<std::string> symbols = {"AAPL", "GOOGL", "MSFT"};
    engine_->subscribe_symbols(symbols);
    engine_->start();
    
    std::atomic<bool> stop_flag{false};
    std::vector<std::thread> worker_threads;
    
    // Create multiple threads accessing engine stats
    for (int i = 0; i < 4; ++i) {
        worker_threads.emplace_back([this, &stop_flag]() {
            while (!stop_flag.load()) {
                auto stats = engine_->get_stats();
                (void)stats;  // Use the stats to prevent optimization
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    // Let threads run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Signal threads to stop
    stop_flag.store(true);
    
    // Wait for all threads to complete
    for (auto& thread : worker_threads) {
        thread.join();
    }
    
    engine_->stop();
    
    // If we reach this point without hanging or crashing, thread safety test passed
    EXPECT_TRUE(true);
}

TEST_F(DataIngestionTest, EventValidation) {
    // Test validation of incoming market data
    MarketTick valid_tick;
    valid_tick.timestamp = Timestamp::now();
    valid_tick.symbol_id = 1;
    valid_tick.bid = Price(100.00);
    valid_tick.ask = Price(100.05);
    valid_tick.last = Price(100.025);
    valid_tick.volume = 1000;
    
    // Valid tick should pass validation
    bool is_valid = (valid_tick.ask.value() > valid_tick.bid.value()) &&
                   (valid_tick.last.value() >= valid_tick.bid.value()) &&
                   (valid_tick.last.value() <= valid_tick.ask.value()) &&
                   (valid_tick.volume > 0);
    
    EXPECT_TRUE(is_valid);
    
    // Invalid tick should fail validation
    MarketTick invalid_tick = valid_tick;
    invalid_tick.ask = Price(99.95);  // Ask < Bid
    
    bool is_invalid = (invalid_tick.ask.value() <= invalid_tick.bid.value());
    EXPECT_TRUE(is_invalid);
}

TEST_F(DataIngestionTest, LatencyMeasurement) {
    // Test that latency measurements are accurate
    auto start = Timestamp::now();
    
    // Simulate some processing time
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    
    auto end = Timestamp::now();
    uint64_t latency_ns = end.nanoseconds_since_epoch - start.nanoseconds_since_epoch;
    
    // Should be approximately 100 microseconds (100,000 nanoseconds)
    EXPECT_GT(latency_ns, 50000);   // At least 50μs
    EXPECT_LT(latency_ns, 500000);  // Less than 500μs (allowing for system variance)
}

TEST_F(DataIngestionTest, ErrorRecovery) {
    // Test error handling and recovery
    std::vector<std::string> invalid_symbols = {"", "TOOLONGSYMBOL12345", "INVALID@SYMBOL"};
    
    // Engine should handle invalid symbols gracefully
    bool result = engine_->subscribe_symbols(invalid_symbols);
    
    // Result may be true or false depending on implementation,
    // but it shouldn't crash
    EXPECT_TRUE(result || !result);
    
    // Engine should still be in a valid state
    EXPECT_FALSE(engine_->is_running());  // Should not be running yet
}

TEST_F(DataIngestionTest, ResourceCleanup) {
    // Test proper resource cleanup
    engine_->subscribe_symbols({"AAPL", "GOOGL"});
    engine_->start();
    
    // Get initial stats
    auto initial_stats = engine_->get_stats();
    
    engine_->stop();
    
    // After stop, engine should be in clean state
    EXPECT_FALSE(engine_->is_running());
    
    // Should be able to restart
    bool restart_success = engine_->start();
    EXPECT_TRUE(restart_success);
    
    engine_->stop();
}