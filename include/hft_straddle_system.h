/*
 * ===================================================================
 *                    HFT STRADDLE OPTIONS TRADING SYSTEM
 * ===================================================================
 * 
 * Professional High-Frequency Trading System for Profitable Straddle
 * Options Strategies on Technology Stocks
 * 
 * SYSTEM ARCHITECTURE:
 * 
 * 1. DATA INGESTION LAYER
 *    - Real-time market data feeds (Level 1 & Level 2)
 *    - Historical options chain data
 *    - Volatility surface construction
 *    - News sentiment feeds
 *    - Zero-copy data structures for ultra-low latency
 * 
 * 2. STRATEGY ENGINE CORE
 *    - Volatility analysis and forecasting
 *    - Greeks calculation (Delta, Gamma, Theta, Vega)
 *    - Entry/exit signal generation
 *    - Risk management and position sizing
 *    - Real-time P&L tracking
 * 
 * 3. STOCK SELECTION MODULE
 *    - Tech stock universe screening
 *    - Liquidity and volume filters
 *    - Earnings calendar integration
 *    - Volatility ranking system
 *    - Market cap and options availability checks
 * 
 * 4. EXECUTION ENGINE
 *    - Low-latency order routing
 *    - Smart order routing (SOR)
 *    - Fill management and slippage control
 *    - Portfolio management
 *    - Real-time position monitoring
 * 
 * 5. ANALYTICS & VISUALIZATION
 *    - Real-time performance plotting
 *    - Risk metrics dashboard
 *    - Trade analysis and reporting
 *    - Backtesting framework
 *    - Strategy optimization tools
 * 
 * KEY DESIGN PRINCIPLES:
 * - Ultra-low latency (sub-microsecond)
 * - Lock-free data structures
 * - Memory-mapped I/O
 * - NUMA-aware memory allocation
 * - CPU cache optimization
 * - Vectorized computations (SIMD)
 * - Template metaprogramming
 * - Zero-allocation runtime paths
 * 
 * TARGET TECH STOCKS:
 * AAPL, GOOGL, MSFT, AMZN, TSLA, NVDA, META, NFLX, CRM, ADBE
 * 
 * PERFORMANCE TARGETS:
 * - Order-to-wire latency: < 10 microseconds
 * - Market data processing: < 1 microsecond
 * - Strategy calculation: < 5 microseconds
 * - Memory usage: < 1GB for full system
 * - CPU usage: < 50% single core
 * 
 * AUTHOR: Nadkalpur Manjunath
 * VERSION: 1.0
 * DATE: October 2025
 * 
 * ===================================================================
 */

#pragma once

#ifndef HFT_STRADDLE_SYSTEM_H
#define HFT_STRADDLE_SYSTEM_H

// System includes with optimization hints
#include <vector>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cmath>
#include <immintrin.h>  // SIMD intrinsics
#include <numa.h>       // NUMA support

// Forward declarations for ultra-fast compilation
namespace hft {
    namespace data {
        class MarketData;
        class OptionsChain;
        class VolatilitySurface;
    }
    
    namespace strategy {
        class StraddleStrategy;
        class RiskManager;
        class PositionSizer;
    }
    
    namespace execution {
        class OrderRouter;
        class PortfolioManager;
        class FillManager;
    }
    
    namespace analytics {
        class PerformanceTracker;
        class Visualizer;
        class BacktestEngine;
    }
    
    namespace selection {
        class TechStockSelector;
        class VolatilityRanker;
    }
}

// Core system constants
namespace hft::constants {
    constexpr size_t MAX_SYMBOLS = 1000;
    constexpr size_t MAX_OPTIONS_PER_SYMBOL = 200;
    constexpr double MIN_OPTION_VOLUME = 100.0;
    constexpr double MAX_BID_ASK_SPREAD = 0.05;
    constexpr int VOLATILITY_WINDOW = 20;
    constexpr double PROFIT_TARGET = 0.15;  // 15% profit target
    constexpr double STOP_LOSS = -0.25;     // 25% stop loss
}

#endif // HFT_STRADDLE_SYSTEM_H