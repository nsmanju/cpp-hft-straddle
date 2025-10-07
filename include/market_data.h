/*
 * ===================================================================
 *                      MARKET DATA STRUCTURES
 * ===================================================================
 * 
 * Ultra-low latency data structures for real-time market data
 * Optimized for CPU cache efficiency and zero-copy operations
 * 
 * PERFORMANCE FEATURES:
 * - Cache-aligned structures (64-byte alignment)
 * - Packed data layout to minimize memory footprint
 * - Lock-free atomic operations
 * - SIMD-friendly data organization
 * - Memory pool allocation
 * 
 * ===================================================================
 */

#pragma once

#include "hft_straddle_system.h"
#include <string>
#include <cstdint>
#include <array>

namespace hft::data {

// Cache-aligned timestamp for ultra-precision timing
struct alignas(8) Timestamp {
    uint64_t nanoseconds_since_epoch;
    
    Timestamp() : nanoseconds_since_epoch(0) {}
    Timestamp(uint64_t ns) : nanoseconds_since_epoch(ns) {}
    
    static Timestamp now() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
        return Timestamp(static_cast<uint64_t>(ns));
    }
    
    double to_seconds() const {
        return nanoseconds_since_epoch / 1e9;
    }
};

// Optimized price structure for financial data
struct alignas(8) Price {
    int64_t value;  // Price in basis points (avoid floating point errors)
    
    Price() : value(0) {}
    Price(double price) : value(static_cast<int64_t>(price * 10000)) {}
    
    double to_double() const { return value / 10000.0; }
    Price operator+(const Price& other) const { return {(value + other.value) / 10000.0}; }
    Price operator-(const Price& other) const { return {(value - other.value) / 10000.0}; }
    bool operator>(const Price& other) const { return value > other.value; }
    bool operator<(const Price& other) const { return value < other.value; }
};

// Cache-aligned market data tick - exactly 64 bytes
struct alignas(64) MarketTick {
    Timestamp timestamp;        // 8 bytes
    uint32_t symbol_id;        // 4 bytes - mapped from symbol string
    Price bid;                 // 8 bytes
    Price ask;                 // 8 bytes
    Price last;                // 8 bytes
    uint32_t bid_size;         // 4 bytes
    uint32_t ask_size;         // 4 bytes
    uint32_t volume;           // 4 bytes
    uint32_t sequence_number;  // 4 bytes
    uint32_t exchange_id;      // 4 bytes
    uint64_t padding;          // 8 bytes - align to 64 bytes
    
    MarketTick() = default;
    
    // Fast midpoint calculation
    Price midpoint() const {
        return Price((bid.to_double() + ask.to_double()) / 2.0);
    }
    
    // Bid-ask spread calculation
    double spread() const {
        return ask.to_double() - bid.to_double();
    }
    
    // Spread as percentage
    double spread_pct() const {
        double mid = midpoint().to_double();
        return mid > 0 ? (spread() / mid) * 100.0 : 0.0;
    }
};

// Options-specific data structure
struct alignas(64) OptionTick {
    Timestamp timestamp;        // 8 bytes
    uint32_t symbol_id;        // 4 bytes
    uint32_t underlying_id;    // 4 bytes
    Price strike;              // 8 bytes
    Price bid;                 // 8 bytes
    Price ask;                 // 8 bytes
    Price last;                // 8 bytes
    uint32_t expiration_date;  // 4 bytes (YYYYMMDD format)
    uint16_t days_to_expiry;   // 2 bytes
    uint8_t option_type;       // 1 byte (0=call, 1=put)
    uint8_t exercise_style;    // 1 byte (0=american, 1=european)
    uint32_t volume;           // 4 bytes
    uint32_t open_interest;    // 4 bytes
    double implied_volatility; // 8 bytes
    uint32_t padding;          // 4 bytes - align to 64 bytes
    
    OptionTick() = default;
    
    // Greeks calculation (simplified Black-Scholes)
    struct Greeks {
        double delta;
        double gamma;
        double theta;
        double vega;
        double rho;
    };
    
    Greeks calculate_greeks(const Price& underlying_price, double risk_free_rate = 0.02) const;
    
    // Option value using Black-Scholes
    double black_scholes_price(const Price& underlying_price, double risk_free_rate = 0.02) const;
    
    // Time to expiration in years
    double time_to_expiry() const {
        return days_to_expiry / 365.0;
    }
    
    // Moneyness calculation
    double moneyness(const Price& underlying_price) const {
        return underlying_price.to_double() / strike.to_double();
    }
};

// Volatility surface point
struct VolatilityPoint {
    double strike;
    double time_to_expiry;
    double implied_volatility;
    double volume;
    Timestamp last_update;
};

// High-performance circular buffer for market data
template<typename T, size_t Size>
class alignas(64) CircularBuffer {
private:
    std::array<T, Size> buffer_;
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
    
public:
    // Lock-free push operation
    bool push(const T& item) {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % Size;
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }
        
        buffer_[current_tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    
    // Lock-free pop operation
    bool pop(T& item) {
        size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }
        
        item = buffer_[current_head];
        head_.store((current_head + 1) % Size, std::memory_order_release);
        return true;
    }
    
    size_t size() const {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_acquire);
        return (t >= h) ? (t - h) : (Size - h + t);
    }
    
    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
    
    bool full() const {
        size_t current_tail = tail_.load(std::memory_order_acquire);
        size_t next_tail = (current_tail + 1) % Size;
        return next_tail == head_.load(std::memory_order_acquire);
    }
};

// Symbol ID mapping for fast lookups
class SymbolMapper {
private:
    std::unordered_map<std::string, uint32_t> symbol_to_id_;
    std::vector<std::string> id_to_symbol_;
    std::atomic<uint32_t> next_id_{1};
    
public:
    uint32_t get_id(const std::string& symbol) {
        auto it = symbol_to_id_.find(symbol);
        if (it != symbol_to_id_.end()) {
            return it->second;
        }
        
        uint32_t id = next_id_.fetch_add(1);
        symbol_to_id_[symbol] = id;
        if (id >= id_to_symbol_.size()) {
            id_to_symbol_.resize(id + 1);
        }
        id_to_symbol_[id] = symbol;
        return id;
    }
    
    const std::string& get_symbol(uint32_t id) const {
        static const std::string empty_string = "";
        return id < id_to_symbol_.size() ? id_to_symbol_[id] : empty_string;
    }
};

// Market data aggregator with SIMD optimizations
class MarketDataAggregator {
private:
    static constexpr size_t BUFFER_SIZE = 1024 * 1024; // 1M entries
    CircularBuffer<MarketTick, BUFFER_SIZE> tick_buffer_;
    std::vector<MarketTick> price_history_;
    
public:
    void add_tick(const MarketTick& tick);
    
    // Calculate VWAP using SIMD
    double calculate_vwap(uint32_t symbol_id, size_t window) const;
    
    // Calculate volatility using SIMD
    double calculate_volatility(uint32_t symbol_id, size_t window) const;
    
    // Get latest tick for symbol
    bool get_latest_tick(uint32_t symbol_id, MarketTick& tick) const;
    
    // Get price history for analysis
    std::vector<Price> get_price_history(uint32_t symbol_id, size_t count) const;
};

} // namespace hft::data