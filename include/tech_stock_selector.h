/*
 * ===================================================================
 *                      TECH STOCK SELECTOR MODULE
 * ===================================================================
 * 
 * Intelligent stock selection system for optimal straddle candidates
 * Focuses on high-liquidity technology stocks with active options markets
 * 
 * SELECTION CRITERIA:
 * - Market cap > $50B (large cap tech stocks)
 * - Average daily volume > 10M shares
 * - Options volume > 10K contracts/day
 * - Bid-ask spread < 2% for ATM options
 * - Implied volatility rank in optimal range
 * - Earnings announcements timing
 * - News sentiment analysis
 * 
 * TARGET UNIVERSE:
 * Primary: AAPL, GOOGL, MSFT, AMZN, TSLA, NVDA, META
 * Secondary: NFLX, CRM, ADBE, ORCL, NOW, SNOW, ZM
 * 
 * ===================================================================
 */

#pragma once

#include "hft_straddle_system.h"
#include "market_data.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <atomic>

namespace hft::selection {

// Stock category enumeration
enum class StockCategory : uint8_t {
    MEGA_CAP = 0,      // > $1T (AAPL, MSFT, GOOGL, AMZN)
    LARGE_CAP = 1,     // $200B - $1T (TSLA, NVDA, META)
    MID_CAP = 2,       // $50B - $200B (NFLX, CRM, ADBE)
    GROWTH = 3,        // High growth stocks
    VALUE = 4,         // Value stocks
    VOLATILE = 5       // High volatility stocks
};

// Stock fundamental data
struct StockFundamentals {
    std::string symbol;
    std::string company_name;
    std::string sector;
    StockCategory category;
    
    // Market data
    double market_cap;              // Market capitalization
    double avg_daily_volume_3m;     // 3-month average daily volume
    double avg_daily_dollar_volume; // Dollar volume
    double shares_outstanding;       // Shares outstanding
    double free_float;              // Free float
    
    // Options market data
    double avg_options_volume;      // Average daily options volume
    double avg_options_open_int;    // Average open interest
    double avg_bid_ask_spread_pct;  // Average bid-ask spread %
    double avg_implied_volatility;  // Average implied volatility
    
    // Volatility metrics
    double historical_vol_30d;      // 30-day historical volatility
    double historical_vol_90d;      // 90-day historical volatility
    double vol_of_vol;              // Volatility of volatility
    double beta;                    // Beta vs SPY
    
    // Earnings and events
    data::Timestamp next_earnings;  // Next earnings date
    int days_to_earnings;           // Days until earnings
    bool has_upcoming_events;       // Other corporate events
    
    StockFundamentals() = default;
};

// Stock selection score
struct SelectionScore {
    std::string symbol;
    double total_score;
    
    // Component scores (0-100 each)
    double liquidity_score;
    double volatility_score;
    double options_activity_score;
    double fundamental_score;
    double technical_score;
    double timing_score;
    
    // Ranking
    int rank;
    bool is_tradeable;
    std::string selection_reason;
    
    SelectionScore() : total_score(0), rank(0), is_tradeable(false) {}
};

// Technology stock universe
class TechStockUniverse {
private:
    // Predefined tech stock lists
    std::vector<std::string> mega_cap_stocks_ = {
        "AAPL", "MSFT", "GOOGL", "GOOG", "AMZN"
    };
    
    std::vector<std::string> large_cap_stocks_ = {
        "TSLA", "NVDA", "META", "NFLX", "CRM", "ADBE"
    };
    
    std::vector<std::string> mid_cap_stocks_ = {
        "ORCL", "NOW", "SNOW", "ZM", "SHOP", "SQ", "PYPL",
        "ROKU", "TWLO", "OKTA", "DDOG", "CRWD"
    };
    
    std::vector<std::string> ai_ml_stocks_ = {
        "NVDA", "AMD", "INTC", "QCOM", "AVGO", "MU"
    };
    
    std::vector<std::string> cloud_stocks_ = {
        "AMZN", "MSFT", "GOOGL", "CRM", "NOW", "SNOW"
    };
    
    std::unordered_map<std::string, StockFundamentals> fundamentals_;
    std::vector<std::string> active_universe_;
    
public:
    TechStockUniverse();
    
    // Universe management
    void initialize_universe();
    void update_fundamentals(const std::string& symbol, const StockFundamentals& data);
    void refresh_universe();
    
    // Stock access
    std::vector<std::string> get_mega_cap_stocks() const { return mega_cap_stocks_; }
    std::vector<std::string> get_large_cap_stocks() const { return large_cap_stocks_; }
    std::vector<std::string> get_mid_cap_stocks() const { return mid_cap_stocks_; }
    std::vector<std::string> get_all_stocks() const;
    std::vector<std::string> get_stocks_by_category(StockCategory category) const;
    
    // Fundamental data access
    bool get_fundamentals(const std::string& symbol, StockFundamentals& data) const;
    std::vector<std::string> get_active_universe() const { return active_universe_; }
    
    // Universe statistics
    size_t get_universe_size() const { return active_universe_.size(); }
    double get_average_market_cap() const;
    double get_average_volume() const;
};

// Stock selection engine
class TechStockSelector {
private:
    // Selection configuration
    struct Config {
        // Liquidity filters
        double min_market_cap;
        double min_avg_volume;
        double min_dollar_volume;
        
        // Options market filters
        double min_options_volume;
        double max_bid_ask_spread;
        double min_implied_vol;
        double max_implied_vol;
        
        // Volatility filters
        double min_historical_vol;
        double max_historical_vol;
        double min_vol_of_vol;
        
        // Timing filters
        int min_days_to_earnings;
        int max_days_to_earnings;
        bool avoid_earnings_week;
        
        // Scoring weights
        double liquidity_weight;
        double volatility_weight;
        double options_weight;
        double fundamental_weight;
        double technical_weight;
        double timing_weight;
        
        // Selection limits
        size_t max_selections;
        double min_total_score;
        
        Config() : min_market_cap(50e9),
                   min_avg_volume(10e6),
                   min_dollar_volume(500e6),
                   min_options_volume(10000),
                   max_bid_ask_spread(0.02),
                   min_implied_vol(0.15),
                   max_implied_vol(0.80),
                   min_historical_vol(0.20),
                   max_historical_vol(1.00),
                   min_vol_of_vol(0.10),
                   min_days_to_earnings(7),
                   max_days_to_earnings(45),
                   avoid_earnings_week(false),
                   liquidity_weight(0.25),
                   volatility_weight(0.25),
                   options_weight(0.20),
                   fundamental_weight(0.15),
                   technical_weight(0.10),
                   timing_weight(0.05),
                   max_selections(5),
                   min_total_score(70.0) {}
    };
    
    Config config_;
    std::unique_ptr<TechStockUniverse> universe_;
    
    // Market data access
    std::function<bool(const std::string&, data::MarketTick&)> market_data_callback_;
    std::function<bool(const std::string&, std::vector<data::OptionTick>&)> options_data_callback_;
    
    // Selection cache
    std::vector<SelectionScore> last_selection_;
    data::Timestamp last_selection_time_;
    std::atomic<bool> selection_valid_{false};
    
public:
    explicit TechStockSelector(const Config& config = Config{});
    ~TechStockSelector();
    
    // Initialization
    bool initialize();
    
    // Data callbacks
    void set_market_data_callback(std::function<bool(const std::string&, data::MarketTick&)> callback);
    void set_options_data_callback(std::function<bool(const std::string&, std::vector<data::OptionTick>&)> callback);
    
    // Main selection process
    std::vector<SelectionScore> select_best_candidates();
    std::vector<std::string> get_top_symbols(size_t count = 5);
    
    // Individual scoring functions
    double calculate_liquidity_score(const StockFundamentals& stock, const data::MarketTick& market_data);
    double calculate_volatility_score(const StockFundamentals& stock);
    double calculate_options_activity_score(const StockFundamentals& stock, 
                                           const std::vector<data::OptionTick>& options_chain);
    double calculate_fundamental_score(const StockFundamentals& stock);
    double calculate_technical_score(const std::string& symbol, const data::MarketTick& market_data);
    double calculate_timing_score(const StockFundamentals& stock);
    
    // Filtering functions
    bool passes_liquidity_filter(const StockFundamentals& stock, const data::MarketTick& market_data);
    bool passes_options_filter(const std::vector<data::OptionTick>& options_chain);
    bool passes_volatility_filter(const StockFundamentals& stock);
    bool passes_timing_filter(const StockFundamentals& stock);
    
    // Selection results
    std::vector<SelectionScore> get_last_selection() const { return last_selection_; }
    bool is_selection_valid() const { return selection_valid_.load(); }
    data::Timestamp get_last_selection_time() const { return last_selection_time_; }
    
    // Configuration
    void update_config(const Config& new_config) { config_ = new_config; }
    Config get_config() const { return config_; }
    
private:
    // Helper functions
    void refresh_fundamentals();
    bool is_earnings_week(const StockFundamentals& stock) const;
    double normalize_score(double value, double min_val, double max_val) const;
    SelectionScore create_selection_score(const std::string& symbol);
};

// Volatility ranking system
class VolatilityRanker {
private:
    struct VolatilityMetrics {
        std::string symbol;
        double current_iv;              // Current implied volatility
        double iv_percentile;           // IV percentile (0-100)
        double hv_current;              // Current historical volatility
        double hv_percentile;           // HV percentile (0-100)
        double iv_hv_ratio;             // IV/HV ratio
        double iv_rank;                 // IV rank (0-100)
        double volatility_trend;        // Volatility trend (rising/falling)
        bool is_low_vol_regime;         // Is in low volatility regime
        double vol_expansion_potential; // Potential for vol expansion
    };
    
    std::unordered_map<std::string, VolatilityMetrics> vol_metrics_;
    std::unordered_map<std::string, std::vector<double>> iv_history_;
    std::unordered_map<std::string, std::vector<double>> hv_history_;
    
public:
    // Update volatility data
    void update_volatility_data(const std::string& symbol, 
                               double implied_vol, 
                               double historical_vol);
    
    // Get volatility metrics
    bool get_volatility_metrics(const std::string& symbol, VolatilityMetrics& metrics);
    
    // Ranking functions
    std::vector<std::string> rank_by_low_volatility(const std::vector<std::string>& symbols);
    std::vector<std::string> rank_by_vol_expansion_potential(const std::vector<std::string>& symbols);
    std::vector<std::string> get_optimal_straddle_candidates(const std::vector<std::string>& symbols);
    
    // Analysis functions
    bool is_good_straddle_entry(const std::string& symbol);
    double calculate_vol_expansion_probability(const std::string& symbol);
    
private:
    double calculate_percentile(const std::vector<double>& data, double value);
    double calculate_trend(const std::vector<double>& data, size_t window = 10);
};

// Market timing analyzer
class MarketTimingAnalyzer {
private:
    struct MarketConditions {
        double vix_level;               // VIX level
        double vix_percentile;          // VIX percentile
        double market_trend;            // Market trend (SPY)
        double sector_rotation;         // Sector rotation factor
        bool is_risk_on;               // Risk-on environment
        bool is_earnings_season;       // In earnings season
        double correlation_regime;      // Inter-stock correlation
    };
    
    MarketConditions current_conditions_;
    
public:
    // Update market conditions
    void update_market_conditions();
    
    // Timing analysis
    bool is_good_market_timing_for_straddles();
    double get_market_timing_score();
    
    // Market regime detection
    bool is_low_volatility_regime();
    bool is_trending_market();
    bool is_range_bound_market();
    
    // Get current conditions
    MarketConditions get_current_conditions() const { return current_conditions_; }
};

} // namespace hft::selection