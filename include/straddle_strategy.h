/*
 * ===================================================================
 *                      STRADDLE STRATEGY ENGINE
 * ===================================================================
 * 
 * High-performance options straddle strategy implementation
 * Based on proven profitable algorithms from Tesla backtesting
 * 
 * STRATEGY OVERVIEW:
 * - Volatility-based entry timing (low vol periods)
 * - OTM strike selection for cheaper premiums
 * - Adaptive profit targets and stop losses
 * - Real-time Greeks monitoring
 * - Multi-stock portfolio management
 * 
 * PERFORMANCE TARGETS:
 * - 70%+ win rate
 * - 15% average profit per trade
 * - Maximum 25% loss per position
 * - Hold time: 1-30 days
 * 
 * ===================================================================
 */

#pragma once

#include "hft_straddle_system.h"
#include "market_data.h"
#include <vector>
#include <memory>
#include <atomic>
#include <unordered_map>

namespace hft::strategy {

// Position status enumeration
enum class PositionStatus : uint8_t {
    NONE = 0,
    ANALYZING = 1,
    ENTRY_PENDING = 2,
    ACTIVE = 3,
    EXIT_PENDING = 4,
    CLOSED = 5,
    ERROR = 6
};

// Trade direction for straddle components
enum class TradeDirection : uint8_t {
    LONG = 0,
    SHORT = 1
};

// Straddle position structure
struct alignas(64) StraddlePosition {
    uint32_t position_id;
    uint32_t symbol_id;
    std::string symbol;
    
    // Entry details
    data::Timestamp entry_time;
    data::Price underlying_entry_price;
    data::Price call_strike;
    data::Price put_strike;
    data::Price call_entry_price;
    data::Price put_entry_price;
    data::Price total_premium_paid;
    
    // Current status
    PositionStatus status;
    data::Timestamp last_update;
    data::Price current_call_price;
    data::Price current_put_price;
    data::Price current_underlying_price;
    
    // P&L tracking
    data::Price unrealized_pnl;
    data::Price realized_pnl;
    data::Price max_profit;
    data::Price max_loss;
    
    // Greeks
    double delta;
    double gamma;
    double theta;
    double vega;
    
    // Risk management
    data::Price profit_target;
    data::Price stop_loss;
    uint16_t days_held;
    uint16_t max_hold_days;
    
    StraddlePosition() = default;
    
    // Calculate current position value
    data::Price calculate_position_value() const {
        return data::Price(current_call_price.to_double() + current_put_price.to_double());
    }
    
    // Calculate current P&L
    data::Price calculate_pnl() const {
        return calculate_position_value() - total_premium_paid;
    }
    
    // Calculate return percentage
    double calculate_return_pct() const {
        if (total_premium_paid.to_double() == 0) return 0.0;
        return (calculate_pnl().to_double() / total_premium_paid.to_double()) * 100.0;
    }
    
    // Check if position should be closed
    bool should_close() const;
};

// Volatility analysis for entry timing
class VolatilityAnalyzer {
private:
    static constexpr size_t VOLATILITY_WINDOW = 20;
    static constexpr double LOW_VOL_PERCENTILE = 0.30;  // Bottom 30% volatility
    
    std::unordered_map<uint32_t, std::vector<double>> volatility_history_;
    std::unordered_map<uint32_t, double> current_volatility_;
    
public:
    // Add price for volatility calculation
    void add_price(uint32_t symbol_id, const data::Price& price, const data::Timestamp& timestamp);
    
    // Calculate current volatility
    double get_current_volatility(uint32_t symbol_id) const;
    
    // Check if current volatility is in low regime (good for straddle entry)
    bool is_low_volatility_regime(uint32_t symbol_id) const;
    
    // Get volatility percentile (0.0 = lowest, 1.0 = highest)
    double get_volatility_percentile(uint32_t symbol_id) const;
    
    // Predict future volatility using GARCH model (simplified)
    double predict_volatility(uint32_t symbol_id, int days_ahead) const;
    
private:
    double calculate_rolling_volatility(const std::vector<double>& prices) const;
};

// Options pricing and Greeks calculator
class OptionsCalculator {
public:
    // Black-Scholes option pricing
    static double black_scholes_call(double S, double K, double T, double r, double sigma);
    static double black_scholes_put(double S, double K, double T, double r, double sigma);
    
    // Greeks calculations
    static double calculate_delta(double S, double K, double T, double r, double sigma, bool is_call);
    static double calculate_gamma(double S, double K, double T, double r, double sigma);
    static double calculate_theta(double S, double K, double T, double r, double sigma, bool is_call);
    static double calculate_vega(double S, double K, double T, double r, double sigma);
    static double calculate_rho(double S, double K, double T, double r, double sigma, bool is_call);
    
    // Implied volatility calculation (Newton-Raphson method)
    static double calculate_implied_volatility(double market_price, double S, double K, 
                                             double T, double r, bool is_call);
    
    // Straddle-specific calculations
    static double straddle_breakeven_upper(double call_strike, double total_premium);
    static double straddle_breakeven_lower(double put_strike, double total_premium);
    static double straddle_max_profit_probability(double S, double K, double T, double sigma);
    
private:
    static double cumulative_normal_distribution(double x);
    static double normal_pdf(double x);
};

// Core straddle strategy engine
class StraddleStrategy {
private:
    // Configuration parameters
    struct Config {
        double otm_offset_pct;
        double max_premium_pct;
        double profit_target_pct;
        double stop_loss_pct;
        uint16_t max_hold_days;
        double min_implied_vol;
        double max_implied_vol;
        double min_time_to_expiry;
        double max_time_to_expiry;
        size_t max_positions;
        double position_size_pct;
        
        Config() : otm_offset_pct(0.02),
                   max_premium_pct(0.05),
                   profit_target_pct(0.15),
                   stop_loss_pct(0.25),
                   max_hold_days(30),
                   min_implied_vol(0.15),
                   max_implied_vol(1.0),
                   min_time_to_expiry(7.0),
                   max_time_to_expiry(60.0),
                   max_positions(10),
                   position_size_pct(0.02) {}
    };
    
    Config config_;
    std::atomic<bool> running_{false};
    
    // Strategy components
    std::unique_ptr<VolatilityAnalyzer> volatility_analyzer_;
    std::unique_ptr<OptionsCalculator> options_calculator_;
    
    // Position management
    std::vector<StraddlePosition> active_positions_;
    std::vector<StraddlePosition> closed_positions_;
    std::atomic<uint32_t> next_position_id_{1};
    mutable std::mutex positions_mutex_;
    
    // Performance tracking
    std::atomic<uint64_t> total_trades_{0};
    std::atomic<uint64_t> winning_trades_{0};
    std::atomic<double> total_pnl_{0.0};
    std::atomic<double> max_drawdown_{0.0};
    
    // Market data access
    std::function<bool(uint32_t, data::MarketTick&)> market_data_callback_;
    std::function<bool(uint32_t, std::vector<data::OptionTick>&)> options_data_callback_;
    
public:
    explicit StraddleStrategy(const Config& config = Config{});
    ~StraddleStrategy();
    
    // Lifecycle
    bool initialize();
    void start();
    void stop();
    
    // Data callbacks
    void set_market_data_callback(std::function<bool(uint32_t, data::MarketTick&)> callback);
    void set_options_data_callback(std::function<bool(uint32_t, std::vector<data::OptionTick>&)> callback);
    
    // Main strategy execution
    void on_market_data(const data::MarketTick& tick);
    void on_options_data(const data::OptionTick& tick);
    
    // Position management
    bool analyze_entry_opportunity(uint32_t symbol_id);
    bool create_straddle_position(uint32_t symbol_id, const data::MarketTick& underlying_tick);
    void update_position(StraddlePosition& position);
    bool should_close_position(const StraddlePosition& position) const;
    void close_position(StraddlePosition& position);
    
    // Strategy logic
    bool is_good_entry_opportunity(uint32_t symbol_id, const data::MarketTick& tick);
    std::pair<data::Price, data::Price> select_optimal_strikes(const data::MarketTick& underlying,
                                                               const std::vector<data::OptionTick>& options_chain);
    double calculate_expected_profit(const StraddlePosition& position) const;
    
    // Performance analytics
    double get_win_rate() const;
    double get_total_pnl() const { return total_pnl_.load(); }
    double get_average_trade_pnl() const;
    double get_max_drawdown() const { return max_drawdown_.load(); }
    double get_sharpe_ratio() const;
    size_t get_active_positions_count() const;
    size_t get_total_trades_count() const { return total_trades_.load(); }
    
    // Position access
    std::vector<StraddlePosition> get_active_positions() const;
    std::vector<StraddlePosition> get_closed_positions() const;
    
private:
    void update_performance_metrics();
    bool validate_position_parameters(const StraddlePosition& position) const;
    void log_trade_execution(const StraddlePosition& position, const std::string& action);
};

// Risk manager for the strategy
class RiskManager {
private:
    struct RiskLimits {
        double max_portfolio_risk;
        double max_position_size;
        double max_correlation;
        double max_sector_exposure;
        size_t max_positions;
        double max_daily_loss;
        double max_monthly_loss;
        
        RiskLimits() : max_portfolio_risk(0.10),
                       max_position_size(0.05),
                       max_correlation(0.70),
                       max_sector_exposure(0.30),
                       max_positions(20),
                       max_daily_loss(0.02),
                       max_monthly_loss(0.05) {}
    };
    
    RiskLimits limits_;
    std::atomic<double> current_portfolio_risk_{0.0};
    std::atomic<double> daily_pnl_{0.0};
    std::atomic<double> monthly_pnl_{0.0};
    
public:
    explicit RiskManager(const RiskLimits& limits = RiskLimits{});
    
    // Risk checks
    bool can_open_position(const StraddlePosition& position, double portfolio_value) const;
    bool should_reduce_exposure() const;
    bool should_stop_trading() const;
    
    // Risk monitoring
    void update_position_risk(const StraddlePosition& position);
    void update_daily_pnl(double pnl);
    
    // Risk metrics
    double get_portfolio_risk() const { return current_portfolio_risk_.load(); }
    double get_daily_pnl() const { return daily_pnl_.load(); }
    double get_monthly_pnl() const { return monthly_pnl_.load(); }
    
    // Risk alerts
    bool is_risk_limit_breached() const;
    std::vector<std::string> get_risk_alerts() const;
};

} // namespace hft::strategy