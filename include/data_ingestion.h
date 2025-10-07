/*
 * ===================================================================
 *                      DATA INGESTION MODULE
 * ===================================================================
 * 
 * High-performance data ingestion system for real-time market feeds
 * Supports multiple data sources with ultra-low latency processing
 * 
 * FEATURES:
 * - Multi-threaded data feeds (TCP/UDP/Multicast)
 * - Zero-copy message parsing
 * - Lock-free data distribution
 * - Market data normalization
 * - Feed failover and redundancy
 * - Real-time data validation
 * 
 * SUPPORTED FEEDS:
 * - IEX Cloud API (real-time quotes)
 * - Alpha Vantage (historical data)
 * - Yahoo Finance (backup feed)
 * - Custom CSV/Binary feeds
 * 
 * ===================================================================
 */

#pragma once

#include "hft_straddle_system.h"
#include "market_data.h"
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <queue>
#include <memory>

namespace hft::data {

// Data feed interface for different providers
class IDataFeed {
public:
    virtual ~IDataFeed() = default;
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;
    virtual void subscribe_symbol(const std::string& symbol) = 0;
    virtual void unsubscribe_symbol(const std::string& symbol) = 0;
    virtual void start_feed() = 0;
    virtual void stop_feed() = 0;
};

// Market data event types
enum class DataEventType : uint8_t {
    MARKET_TICK = 0,
    OPTION_TICK = 1,
    TRADE = 2,
    ORDER_BOOK_UPDATE = 3,
    NEWS = 4,
    ERROR = 5
};

// Event structure for data distribution
struct alignas(64) DataEvent {
    DataEventType type;
    Timestamp timestamp;
    uint32_t symbol_id;
    
    // Use variant instead of union for C++17 compatibility
    MarketTick market_tick;
    OptionTick option_tick;
    
    DataEvent() : type(DataEventType::MARKET_TICK), timestamp(Timestamp::now()), symbol_id(0) {}
    
    DataEvent(DataEventType t, const MarketTick& tick) 
        : type(t), timestamp(Timestamp::now()), symbol_id(tick.symbol_id), market_tick(tick) {}
        
    DataEvent(DataEventType t, const OptionTick& tick) 
        : type(t), timestamp(Timestamp::now()), symbol_id(tick.symbol_id), option_tick(tick) {}
};

// High-performance data ingestion engine
class DataIngestionEngine {
private:
    // Configuration
    struct Config {
        size_t num_worker_threads;
        size_t buffer_size;
        bool enable_market_data;
        bool enable_options_data;
        bool enable_level2_data;
        std::vector<std::string> tech_symbols;
        
        Config() : num_worker_threads(4), 
                   buffer_size(1024 * 1024),
                   enable_market_data(true),
                   enable_options_data(true),
                   enable_level2_data(false),
                   tech_symbols{"AAPL", "GOOGL", "MSFT", "AMZN", "TSLA", 
                               "NVDA", "META", "NFLX", "CRM", "ADBE"} {}
    };
    
    Config config_;
    std::atomic<bool> running_{false};
    std::vector<std::unique_ptr<IDataFeed>> feeds_;
    std::vector<std::thread> worker_threads_;
    
    // Lock-free event distribution
    CircularBuffer<DataEvent, 1024 * 1024> event_buffer_;
    std::vector<std::function<void(const DataEvent&)>> subscribers_;
    
    // Symbol mapping
    SymbolMapper symbol_mapper_;
    
    // Market data aggregation
    MarketDataAggregator market_aggregator_;
    
    // Performance monitoring
    std::atomic<uint64_t> events_processed_{0};
    std::atomic<uint64_t> events_dropped_{0};
    Timestamp start_time_;
    
public:
    explicit DataIngestionEngine(const Config& config = Config{});
    ~DataIngestionEngine();
    
    // Lifecycle management
    bool initialize();
    void start();
    void stop();
    
    // Data feed management
    void add_feed(std::unique_ptr<IDataFeed> feed);
    void subscribe_symbols(const std::vector<std::string>& symbols);
    
    // Event subscription
    void subscribe_to_events(std::function<void(const DataEvent&)> callback);
    
    // Data access
    bool get_latest_market_data(const std::string& symbol, MarketTick& tick);
    bool get_latest_option_data(const std::string& symbol, OptionTick& tick);
    std::vector<Price> get_price_history(const std::string& symbol, size_t count);
    
    // Performance metrics
    uint64_t get_events_processed() const { return events_processed_.load(); }
    uint64_t get_events_dropped() const { return events_dropped_.load(); }
    double get_processing_rate() const;
    
private:
    void worker_thread_main();
    void process_event(const DataEvent& event);
    void distribute_event(const DataEvent& event);
};

// IEX Cloud data feed implementation
class IEXCloudFeed : public IDataFeed {
private:
    std::string api_token_;
    std::string base_url_ = "https://cloud.iexapis.com/stable";
    std::vector<std::string> subscribed_symbols_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> running_{false};
    std::thread polling_thread_;
    
    // Callback for data events
    std::function<void(const MarketTick&)> market_callback_;
    std::function<void(const OptionTick&)> option_callback_;
    
public:
    explicit IEXCloudFeed(const std::string& token);
    ~IEXCloudFeed();
    
    // IDataFeed interface
    bool connect() override;
    void disconnect() override;
    bool is_connected() const override { return connected_.load(); }
    void subscribe_symbol(const std::string& symbol) override;
    void unsubscribe_symbol(const std::string& symbol) override;
    void start_feed() override;
    void stop_feed() override;
    
    // Callbacks
    void set_market_callback(std::function<void(const MarketTick&)> callback) {
        market_callback_ = callback;
    }
    
    void set_option_callback(std::function<void(const OptionTick&)> callback) {
        option_callback_ = callback;
    }
    
private:
    void polling_loop();
    std::string make_request(const std::string& endpoint);
    MarketTick parse_quote(const std::string& json_data, const std::string& symbol);
    std::vector<OptionTick> parse_options_chain(const std::string& json_data, const std::string& symbol);
};

// Historical data loader for backtesting
class HistoricalDataLoader {
private:
    std::string data_directory_;
    std::unordered_map<std::string, std::vector<MarketTick>> historical_data_;
    
public:
    explicit HistoricalDataLoader(const std::string& data_dir);
    
    // Load data from various formats
    bool load_csv_data(const std::string& filename, const std::string& symbol);
    bool load_binary_data(const std::string& filename);
    
    // Data access
    std::vector<MarketTick> get_historical_data(const std::string& symbol, 
                                               const Timestamp& start, 
                                               const Timestamp& end) const;
    
    // Statistics
    size_t get_data_points(const std::string& symbol) const;
    Timestamp get_earliest_timestamp(const std::string& symbol) const;
    Timestamp get_latest_timestamp(const std::string& symbol) const;
};

// Market data validator
class DataValidator {
public:
    // Validation rules
    struct ValidationRules {
        double max_price_change_pct;
        double min_bid_ask_ratio;
        double max_spread_pct;
        uint32_t min_volume;
        uint32_t max_volume;
        
        ValidationRules() : max_price_change_pct(20.0),
                           min_bid_ask_ratio(0.01),
                           max_spread_pct(5.0),
                           min_volume(1),
                           max_volume(1000000000) {}
    };
    
    explicit DataValidator(const ValidationRules& rules = ValidationRules{});
    
    // Validation functions
    bool validate_market_tick(const MarketTick& tick, const MarketTick* prev_tick = nullptr);
    bool validate_option_tick(const OptionTick& tick, const MarketTick& underlying);
    
    // Get validation statistics
    uint64_t get_validated_count() const { return validated_count_.load(); }
    uint64_t get_rejected_count() const { return rejected_count_.load(); }
    double get_rejection_rate() const;
    
private:
    ValidationRules rules_;
    std::atomic<uint64_t> validated_count_{0};
    std::atomic<uint64_t> rejected_count_{0};
    
    bool is_price_reasonable(const Price& price, const Price& prev_price) const;
    bool is_spread_reasonable(const Price& bid, const Price& ask) const;
};

// Data feed factory
class DataFeedFactory {
public:
    enum class FeedType {
        IEX_CLOUD,
        ALPHA_VANTAGE,
        YAHOO_FINANCE,
        CUSTOM_CSV,
        SIMULATION
    };
    
    static std::unique_ptr<IDataFeed> create_feed(FeedType type, 
                                                 const std::string& config_params);
    
    static std::vector<std::string> get_available_feeds();
};

} // namespace hft::data