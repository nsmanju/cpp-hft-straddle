#include <gtest/gtest.h>
#include "../include/market_data.h"

using namespace hft_straddle;
using namespace hft_straddle::data;

class MarketDataTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test data
        test_tick_.timestamp = Timestamp::now();
        test_tick_.symbol_id = 1;
        test_tick_.bid = Price(149.50);
        test_tick_.ask = Price(149.55);
        test_tick_.last = Price(149.52);
        test_tick_.volume = 1000;
    }

    MarketTick test_tick_;
};

TEST_F(MarketDataTest, MarketTickConstruction) {
    EXPECT_GT(test_tick_.timestamp.nanoseconds_since_epoch, 0);
    EXPECT_EQ(test_tick_.symbol_id, 1);
    EXPECT_DOUBLE_EQ(test_tick_.bid.value(), 149.50);
    EXPECT_DOUBLE_EQ(test_tick_.ask.value(), 149.55);
    EXPECT_DOUBLE_EQ(test_tick_.last.value(), 149.52);
    EXPECT_EQ(test_tick_.volume, 1000);
}

TEST_F(MarketDataTest, MidpointCalculation) {
    double midpoint = test_tick_.midpoint().value();
    EXPECT_DOUBLE_EQ(midpoint, 149.525);
}

TEST_F(MarketDataTest, SpreadCalculation) {
    double spread_pct = test_tick_.spread_pct();
    EXPECT_GT(spread_pct, 0.0);
    EXPECT_LT(spread_pct, 1.0);  // Should be less than 1%
}

TEST_F(MarketDataTest, TimestampPrecision) {
    Timestamp t1 = Timestamp::now();
    Timestamp t2 = Timestamp::now();
    
    // Timestamps should be different (nanosecond precision)
    EXPECT_GE(t2.nanoseconds_since_epoch, t1.nanoseconds_since_epoch);
}

TEST_F(MarketDataTest, OptionTickConstruction) {
    OptionTick option_tick;
    strcpy(option_tick.symbol, "AAPL");
    option_tick.expiration_date = 20251220;  // Dec 20, 2025
    option_tick.strike_price = Price(150.0);
    option_tick.option_type = OptionType::CALL;
    option_tick.bid = Price(5.50);
    option_tick.ask = Price(5.60);
    option_tick.last_price = Price(5.55);
    option_tick.volume = 500;
    option_tick.open_interest = 1500;
    option_tick.implied_volatility = 0.25;
    option_tick.delta = 0.6;
    option_tick.gamma = 0.05;
    option_tick.theta = -0.02;
    option_tick.vega = 0.15;
    
    EXPECT_STREQ(option_tick.symbol, "AAPL");
    EXPECT_EQ(option_tick.option_type, OptionType::CALL);
    EXPECT_DOUBLE_EQ(option_tick.strike_price.value(), 150.0);
    EXPECT_GT(option_tick.implied_volatility, 0.0);
    EXPECT_LT(option_tick.implied_volatility, 1.0);
}

TEST_F(MarketDataTest, StraddleQuoteConstruction) {
    StraddleQuote straddle;
    strcpy(straddle.underlying_symbol, "AAPL");
    straddle.underlying_price = Price(150.0);
    straddle.expiration_date = 20251220;
    straddle.strike_price = Price(150.0);
    
    // Call option data
    straddle.call_option.bid = Price(5.50);
    straddle.call_option.ask = Price(5.60);
    straddle.call_option.last_price = Price(5.55);
    straddle.call_option.volume = 500;
    
    // Put option data
    straddle.put_option.bid = Price(5.40);
    straddle.put_option.ask = Price(5.50);
    straddle.put_option.last_price = Price(5.45);
    straddle.put_option.volume = 400;
    
    // Straddle price should be sum of call and put
    straddle.straddle_price = Price(straddle.call_option.last_price.value() + 
                                   straddle.put_option.last_price.value());
    
    EXPECT_STREQ(straddle.underlying_symbol, "AAPL");
    EXPECT_DOUBLE_EQ(straddle.underlying_price.value(), 150.0);
    EXPECT_DOUBLE_EQ(straddle.straddle_price.value(), 11.0);  // 5.55 + 5.45
}

TEST_F(MarketDataTest, DataEventAlignment) {
    // Verify cache alignment for performance
    DataEvent event;
    EXPECT_EQ(reinterpret_cast<uintptr_t>(&event) % 64, 0);
}

TEST_F(MarketDataTest, PerformanceBenchmark) {
    // Simple performance check - should be very fast
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; ++i) {
        MarketTick tick = test_tick_;
        double midpoint = tick.midpoint().value();
        double spread = tick.spread_pct();
        (void)midpoint;  // Suppress unused variable warning
        (void)spread;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should complete 10k iterations in less than 1ms
    EXPECT_LT(duration.count(), 1000);
}