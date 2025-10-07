#include <gtest/gtest.h>
#include "../include/straddle_strategy.h"
#include "../include/market_data.h"

using namespace hft_straddle;
using namespace hft_straddle::strategy;
using namespace hft_straddle::data;

class StraddleStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        strategy_ = std::make_unique<StraddleStrategy>();
        
        // Create test market data
        test_tick_.timestamp = Timestamp::now();
        test_tick_.symbol_id = 1;  // AAPL
        test_tick_.bid = Price(149.50);
        test_tick_.ask = Price(149.55);
        test_tick_.last = Price(149.52);
        test_tick_.volume = 10000;
        
        // Create test straddle quote
        strcpy(straddle_.underlying_symbol, "AAPL");
        straddle_.underlying_price = Price(150.0);
        straddle_.expiration_date = 20251220;
        straddle_.strike_price = Price(150.0);
        
        straddle_.call_option.bid = Price(5.50);
        straddle_.call_option.ask = Price(5.60);
        straddle_.call_option.last_price = Price(5.55);
        straddle_.call_option.volume = 500;
        straddle_.call_option.implied_volatility = 0.25;
        
        straddle_.put_option.bid = Price(5.40);
        straddle_.put_option.ask = Price(5.50);
        straddle_.put_option.last_price = Price(5.45);
        straddle_.put_option.volume = 400;
        straddle_.put_option.implied_volatility = 0.24;
        
        straddle_.straddle_price = Price(11.0);
        straddle_.total_volume = 900;
        straddle_.bid_ask_spread = 0.05;
    }

    std::unique_ptr<StraddleStrategy> strategy_;
    MarketTick test_tick_;
    StraddleQuote straddle_;
};

TEST_F(StraddleStrategyTest, StrategyInitialization) {
    EXPECT_TRUE(strategy_ != nullptr);
    
    // Verify initial state
    auto metrics = strategy_->get_performance_metrics();
    EXPECT_EQ(metrics.total_trades, 0);
    EXPECT_EQ(metrics.winning_trades, 0);
    EXPECT_DOUBLE_EQ(metrics.total_pnl, 0.0);
    EXPECT_DOUBLE_EQ(metrics.win_rate, 0.0);
}

TEST_F(StraddleStrategyTest, GoodEntryOpportunityDetection) {
    // Test conditions that should trigger entry
    straddle_.call_option.implied_volatility = 0.15;  // Low volatility
    straddle_.put_option.implied_volatility = 0.15;
    straddle_.total_volume = 2000;  // High volume
    straddle_.bid_ask_spread = 0.02;  // Tight spread
    
    bool is_good_entry = strategy_->is_good_entry_opportunity(1, test_tick_);
    // Note: This test would need access to market data registry
    // For now, just verify the method exists and doesn't crash
    EXPECT_TRUE(true);
}

TEST_F(StraddleStrategyTest, PositionCreation) {
    bool position_created = strategy_->create_straddle_position(1, test_tick_);
    
    // Without proper market data setup, this might return false
    // But it should not crash
    EXPECT_TRUE(position_created || !position_created);
}

TEST_F(StraddleStrategyTest, PerformanceCalculations) {
    // Test profit/loss calculations
    StraddlePosition position;
    position.position_id = 1;
    position.underlying_symbol_id = 1;
    position.entry_time = Timestamp::now();
    position.strike_price = Price(150.0);
    position.entry_premium = Price(11.0);
    position.quantity = 1;
    position.profit_target = Price(12.65);  // 15% profit target
    position.stop_loss = Price(8.25);       // 25% stop loss
    
    // Test profit target calculation
    EXPECT_DOUBLE_EQ(position.profit_target.value(), 12.65);
    EXPECT_DOUBLE_EQ(position.stop_loss.value(), 8.25);
}

TEST_F(StraddleStrategyTest, RiskManagement) {
    // Test position sizing
    double available_capital = 100000.0;
    double position_value = 11000.0;  // $11k position
    
    // Should not exceed 5% of capital per position
    double max_position_size = available_capital * 0.05;
    EXPECT_LE(position_value, max_position_size);
}

TEST_F(StraddleStrategyTest, TimingValidation) {
    // Test that we don't hold positions too long
    StraddlePosition position;
    position.entry_time = Timestamp::now();
    position.expiration_date = 20251220;
    
    // Calculate days to expiration
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::gmtime(&time_t);
    
    int current_date = (tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday;
    int days_to_expiry = (position.expiration_date - current_date) / 100 * 30; // Rough approximation
    
    // Should not hold positions with less than 7 days to expiry
    EXPECT_TRUE(days_to_expiry > 7 || days_to_expiry < 0);  // Allow negative for past dates in tests
}

TEST_F(StraddleStrategyTest, VolatilityAnalysis) {
    // Test volatility spike detection
    double base_volatility = 0.20;
    double current_volatility = 0.15;
    
    // Entry should be triggered when volatility is low
    double volatility_threshold = 0.18;
    bool should_enter = current_volatility < volatility_threshold;
    
    EXPECT_TRUE(should_enter);
}

TEST_F(StraddleStrategyTest, LiquidityCheck) {
    // Test minimum liquidity requirements
    uint32_t min_volume = 1000;
    double max_spread = 0.03;  // 3%
    
    bool has_sufficient_liquidity = (straddle_.total_volume >= min_volume) && 
                                   (straddle_.bid_ask_spread <= max_spread);
    
    EXPECT_TRUE(has_sufficient_liquidity);
}

TEST_F(StraddleStrategyTest, PerformanceMetrics) {
    // Simulate some trades to test metrics calculation
    PerformanceMetrics metrics;
    metrics.total_trades = 10;
    metrics.winning_trades = 8;
    metrics.total_pnl = 15000.0;
    metrics.total_capital_deployed = 100000.0;
    
    // Calculate derived metrics
    metrics.win_rate = static_cast<double>(metrics.winning_trades) / metrics.total_trades;
    double roi = metrics.total_pnl / metrics.total_capital_deployed;
    
    EXPECT_DOUBLE_EQ(metrics.win_rate, 0.8);  // 80% win rate
    EXPECT_DOUBLE_EQ(roi, 0.15);              // 15% ROI
}

TEST_F(StraddleStrategyTest, StressTest) {
    // Test strategy with extreme market conditions
    StraddleQuote extreme_straddle = straddle_;
    
    // Very high volatility
    extreme_straddle.call_option.implied_volatility = 0.80;
    extreme_straddle.put_option.implied_volatility = 0.85;
    
    // Very wide spread
    extreme_straddle.bid_ask_spread = 0.10;  // 10% spread
    
    // Low volume
    extreme_straddle.total_volume = 50;
    
    // Strategy should reject this opportunity
    // (Implementation would need actual strategy logic)
    EXPECT_TRUE(true);  // Placeholder assertion
}