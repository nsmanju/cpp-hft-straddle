#include <benchmark/benchmark.h>
#include "../include/straddle_strategy.h"
#include "../include/market_data.h"
#include <vector>
#include <random>

using namespace hft_straddle::strategy;
using namespace hft_straddle::data;

// Helper function to create test straddle quote
StraddleQuote create_test_straddle(double underlying_price, double call_price, double put_price) {
    StraddleQuote straddle;
    strcpy(straddle.underlying_symbol, "AAPL");
    straddle.underlying_price = Price(underlying_price);
    straddle.expiration_date = 20251220;
    straddle.strike_price = Price(150.0);
    
    straddle.call_option.bid = Price(call_price - 0.05);
    straddle.call_option.ask = Price(call_price + 0.05);
    straddle.call_option.last_price = Price(call_price);
    straddle.call_option.volume = 500;
    straddle.call_option.implied_volatility = 0.25;
    
    straddle.put_option.bid = Price(put_price - 0.05);
    straddle.put_option.ask = Price(put_price + 0.05);
    straddle.put_option.last_price = Price(put_price);
    straddle.put_option.volume = 400;
    straddle.put_option.implied_volatility = 0.24;
    
    straddle.straddle_price = Price(call_price + put_price);
    straddle.total_volume = 900;
    straddle.bid_ask_spread = 0.05;
    
    return straddle;
}

// Benchmark strategy initialization
static void BM_StrategyInitialization(benchmark::State& state) {
    for (auto _ : state) {
        StraddleStrategy strategy;
        benchmark::DoNotOptimize(strategy);
    }
}
BENCHMARK(BM_StrategyInitialization);

// Benchmark profit/loss calculation
static void BM_ProfitLossCalculation(benchmark::State& state) {
    StraddlePosition position;
    position.position_id = 1;
    position.underlying_symbol_id = 1;
    position.entry_time = Timestamp::now();
    position.strike_price = Price(150.0);
    position.entry_premium = Price(11.0);
    position.quantity = 1;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dist(140.0, 160.0);
    
    for (auto _ : state) {
        double current_underlying = price_dist(gen);
        
        // Calculate current straddle value
        double call_intrinsic = std::max(0.0, current_underlying - position.strike_price.value());
        double put_intrinsic = std::max(0.0, position.strike_price.value() - current_underlying);
        double current_value = call_intrinsic + put_intrinsic;
        
        // Calculate P&L
        double pnl = (current_value - position.entry_premium.value()) * position.quantity;
        
        benchmark::DoNotOptimize(pnl);
    }
}
BENCHMARK(BM_ProfitLossCalculation);

// Benchmark volatility analysis
static void BM_VolatilityAnalysis(benchmark::State& state) {
    std::vector<double> price_history;
    price_history.reserve(100);
    
    // Generate sample price history
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> price_dist(150.0, 5.0);
    
    for (int i = 0; i < 100; ++i) {
        price_history.push_back(price_dist(gen));
    }
    
    for (auto _ : state) {
        // Calculate historical volatility
        double sum = 0.0;
        for (size_t i = 1; i < price_history.size(); ++i) {
            double return_rate = std::log(price_history[i] / price_history[i-1]);
            sum += return_rate * return_rate;
        }
        
        double variance = sum / (price_history.size() - 1);
        double volatility = std::sqrt(variance * 252);  // Annualized
        
        benchmark::DoNotOptimize(volatility);
    }
}
BENCHMARK(BM_VolatilityAnalysis);

// Benchmark risk calculations
static void BM_RiskCalculations(benchmark::State& state) {
    StraddlePosition position;
    position.position_id = 1;
    position.strike_price = Price(150.0);
    position.entry_premium = Price(11.0);
    position.quantity = 10;  // 10 contracts
    
    double portfolio_value = 1000000.0;  // $1M portfolio
    
    for (auto _ : state) {
        // Calculate position value
        double position_value = position.entry_premium.value() * position.quantity * 100;  // Options are per 100 shares
        
        // Calculate position concentration
        double position_concentration = position_value / portfolio_value;
        
        // Calculate Greeks (simplified)
        double delta = 0.0;  // ATM straddle should be delta neutral
        double gamma = 0.10;  // High gamma for ATM options
        double theta = -0.05; // Time decay
        double vega = 0.20;   // Volatility sensitivity
        
        // Calculate portfolio Greeks
        double portfolio_delta = delta * position.quantity;
        double portfolio_gamma = gamma * position.quantity;
        double portfolio_theta = theta * position.quantity;
        double portfolio_vega = vega * position.quantity;
        
        benchmark::DoNotOptimize(position_concentration);
        benchmark::DoNotOptimize(portfolio_delta);
        benchmark::DoNotOptimize(portfolio_gamma);
        benchmark::DoNotOptimize(portfolio_theta);
        benchmark::DoNotOptimize(portfolio_vega);
    }
}
BENCHMARK(BM_RiskCalculations);

// Benchmark entry signal generation
static void BM_EntrySignalGeneration(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dist(145.0, 155.0);
    std::uniform_real_distribution<> vol_dist(0.15, 0.35);
    std::uniform_int_distribution<> volume_dist(500, 5000);
    
    for (auto _ : state) {
        double underlying_price = price_dist(gen);
        double call_price = vol_dist(gen) * 10.0;  // Simplified pricing
        double put_price = vol_dist(gen) * 10.0;
        
        StraddleQuote straddle = create_test_straddle(underlying_price, call_price, put_price);
        
        // Entry criteria evaluation
        bool low_volatility = (straddle.call_option.implied_volatility < 0.20) && 
                             (straddle.put_option.implied_volatility < 0.20);
        bool sufficient_volume = straddle.total_volume > 1000;
        bool tight_spread = straddle.bid_ask_spread < 0.03;
        bool otm_criteria = std::abs(straddle.underlying_price.value() - straddle.strike_price.value()) / 
                           straddle.strike_price.value() < 0.02;  // Within 2% of ATM
        
        bool enter_trade = low_volatility && sufficient_volume && tight_spread && otm_criteria;
        
        benchmark::DoNotOptimize(enter_trade);
    }
}
BENCHMARK(BM_EntrySignalGeneration);

// Benchmark exit signal generation
static void BM_ExitSignalGeneration(benchmark::State& state) {
    StraddlePosition position;
    position.position_id = 1;
    position.entry_time = Timestamp::now();
    position.strike_price = Price(150.0);
    position.entry_premium = Price(11.0);
    position.quantity = 1;
    position.profit_target = Price(12.65);  // 15% profit
    position.stop_loss = Price(8.25);       // 25% loss
    position.expiration_date = 20251220;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dist(8.0, 15.0);
    
    for (auto _ : state) {
        double current_straddle_price = price_dist(gen);
        
        // Exit criteria evaluation
        bool profit_target_hit = current_straddle_price >= position.profit_target.value();
        bool stop_loss_hit = current_straddle_price <= position.stop_loss.value();
        
        // Time-based exit (simplified - assuming we're close to expiration)
        auto current_time = Timestamp::now();
        uint64_t time_held_ns = current_time.nanoseconds_since_epoch - position.entry_time.nanoseconds_since_epoch;
        uint64_t max_hold_time_ns = 30ULL * 24 * 60 * 60 * 1000000000;  // 30 days in nanoseconds
        bool max_time_reached = time_held_ns > max_hold_time_ns;
        
        bool exit_trade = profit_target_hit || stop_loss_hit || max_time_reached;
        
        benchmark::DoNotOptimize(exit_trade);
    }
}
BENCHMARK(BM_ExitSignalGeneration);

// Benchmark position management
static void BM_PositionManagement(benchmark::State& state) {
    const size_t num_positions = state.range(0);
    std::vector<StraddlePosition> positions;
    positions.reserve(num_positions);
    
    // Create test positions
    for (size_t i = 0; i < num_positions; ++i) {
        StraddlePosition pos;
        pos.position_id = static_cast<uint32_t>(i);
        pos.underlying_symbol_id = static_cast<uint32_t>(i % 10);
        pos.entry_time = Timestamp::now();
        pos.strike_price = Price(150.0 + (i % 20));
        pos.entry_premium = Price(10.0 + (i % 5));
        pos.quantity = 1 + (i % 3);
        pos.profit_target = Price(pos.entry_premium.value() * 1.15);
        pos.stop_loss = Price(pos.entry_premium.value() * 0.75);
        pos.expiration_date = 20251220;
        positions.push_back(pos);
    }
    
    for (auto _ : state) {
        double total_exposure = 0.0;
        double total_pnl = 0.0;
        
        for (const auto& pos : positions) {
            double position_value = pos.entry_premium.value() * pos.quantity * 100;
            total_exposure += position_value;
            
            // Simplified P&L calculation
            double current_value = pos.entry_premium.value() * 1.05;  // 5% gain
            double pnl = (current_value - pos.entry_premium.value()) * pos.quantity * 100;
            total_pnl += pnl;
        }
        
        benchmark::DoNotOptimize(total_exposure);
        benchmark::DoNotOptimize(total_pnl);
    }
}
BENCHMARK(BM_PositionManagement)->Range(10, 1000);

// Benchmark performance metrics calculation
static void BM_PerformanceMetrics(benchmark::State& state) {
    std::vector<double> trade_returns;
    trade_returns.reserve(100);
    
    // Generate sample trade returns
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> return_dist(0.05, 0.15);  // 5% mean, 15% std dev
    
    for (int i = 0; i < 100; ++i) {
        trade_returns.push_back(return_dist(gen));
    }
    
    for (auto _ : state) {
        // Calculate performance metrics
        double total_return = 0.0;
        double sum_squared_returns = 0.0;
        int winning_trades = 0;
        
        for (double ret : trade_returns) {
            total_return += ret;
            sum_squared_returns += ret * ret;
            if (ret > 0) winning_trades++;
        }
        
        double avg_return = total_return / trade_returns.size();
        double variance = (sum_squared_returns / trade_returns.size()) - (avg_return * avg_return);
        double std_dev = std::sqrt(variance);
        double sharpe_ratio = avg_return / std_dev;
        double win_rate = static_cast<double>(winning_trades) / trade_returns.size();
        
        benchmark::DoNotOptimize(avg_return);
        benchmark::DoNotOptimize(sharpe_ratio);
        benchmark::DoNotOptimize(win_rate);
    }
}
BENCHMARK(BM_PerformanceMetrics);

BENCHMARK_MAIN();