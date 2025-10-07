# 🤝 Contributing to HFT Straddle Options Trading System

Thank you for your interest in contributing to the HFT Straddle Options Trading System! This document provides guidelines and information for contributors.

## 📋 Table of Contents

1. [Code of Conduct](#-code-of-conduct)
2. [Getting Started](#-getting-started)
3. [Development Setup](#-development-setup)
4. [Contribution Workflow](#-contribution-workflow)
5. [Code Standards](#-code-standards)
6. [Testing Requirements](#-testing-requirements)
7. [Performance Guidelines](#-performance-guidelines)
8. [Documentation Standards](#-documentation-standards)
9. [Review Process](#-review-process)

## 📜 Code of Conduct

This project follows a Code of Conduct to ensure a welcoming environment for all contributors. Please read and follow these guidelines:

- **Be respectful** and inclusive in all interactions
- **Be constructive** in feedback and discussions
- **Focus on the code**, not the person
- **Help create a positive** learning environment

## 🚀 Getting Started

### Prerequisites

Before contributing, ensure you have:
- **C++17 compatible compiler** (GCC 11+, Clang 14+, or MSVC 2019+)
- **CMake 3.20+**
- **Git** for version control
- **Basic understanding** of HFT concepts and options trading

### Areas for Contribution

We welcome contributions in these areas:
- 🐛 **Bug fixes**
- 🚀 **New features**
- ⚡ **Performance optimizations**
- 📚 **Documentation improvements**
- 🧪 **Test coverage enhancements**
- 🔒 **Security improvements**

## 💻 Development Setup

### 1. Fork and Clone
```bash
# Fork the repository on GitHub, then clone your fork
git clone https://github.com/YOUR_USERNAME/cpp-hft-straddle.git
cd cpp-hft-straddle
```

### 2. Set Up Development Environment
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y build-essential cmake libnuma-dev libcurl4-openssl-dev

# Create development build
mkdir build-dev
cd build-dev
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=ON
make -j$(nproc)
```

### 3. Run Tests
```bash
# Ensure all tests pass before making changes
ctest --output-on-failure
```

## 🔄 Contribution Workflow

### 1. Create Feature Branch
```bash
git checkout -b feature/your-feature-name
# or
git checkout -b bugfix/issue-123
```

### 2. Make Your Changes
- Write clean, well-documented code
- Follow our coding standards (see below)
- Add appropriate tests
- Update documentation if needed

### 3. Test Your Changes
```bash
# Build and test
cd build-dev
make -j$(nproc)
ctest --output-on-failure

# Run additional checks
make lint        # Code style check
make memcheck    # Memory leak detection
make benchmark   # Performance validation
```

### 4. Commit Changes
```bash
git add .
git commit -m "feat: add new volatility calculation method

- Implement advanced volatility calculation using GARCH model
- Add comprehensive unit tests
- Update documentation with usage examples
- Performance improvement: 15% faster execution"
```

### Commit Message Format
Use conventional commits format:
- `feat:` for new features
- `fix:` for bug fixes
- `perf:` for performance improvements
- `docs:` for documentation changes
- `test:` for test additions/modifications
- `refactor:` for code refactoring
- `ci:` for CI/CD changes

### 5. Push and Create Pull Request
```bash
git push origin feature/your-feature-name
```

Then create a pull request on GitHub using our PR template.

## 📏 Code Standards

### C++ Style Guidelines

#### Naming Conventions
```cpp
// Classes: PascalCase
class VolatilityCalculator {
public:
    // Public methods: snake_case
    double calculate_implied_volatility(const OptionData& data);
    
    // Constants: UPPER_SNAKE_CASE
    static constexpr double DEFAULT_RISK_FREE_RATE = 0.02;
    
private:
    // Private members: trailing underscore
    double risk_free_rate_;
    std::vector<double> price_history_;
};

// Namespaces: snake_case
namespace market_data {
    // Functions: snake_case
    bool validate_option_tick(const OptionTick& tick);
}

// Variables: snake_case
double current_price = 150.0;
size_t option_count = 0;
```

#### Performance-Critical Guidelines
```cpp
// Use cache-aligned structures for hot paths
struct alignas(64) MarketTick {
    Timestamp timestamp;
    Price bid, ask, last;
    uint32_t volume;
};

// Prefer const references for read-only parameters
void process_market_data(const MarketTick& tick);

// Use move semantics for expensive objects
void store_tick_data(TickData&& data);

// Mark hot functions as inline when appropriate
inline double calculate_midpoint(Price bid, Price ask) {
    return (bid + ask) / 2.0;
}
```

#### Modern C++ Features
```cpp
// Use auto for type deduction
auto market_data = load_market_data("AAPL");

// Use range-based for loops
for (const auto& tick : market_ticks) {
    process_tick(tick);
}

// Use smart pointers
std::unique_ptr<Strategy> strategy = std::make_unique<StraddleStrategy>();

// Use constexpr for compile-time constants
constexpr double SECONDS_PER_DAY = 86400.0;
```

### Code Organization
```cpp
// Header file structure
#pragma once

#include <system_headers>
#include "project_headers.h"

namespace hft_straddle {

class YourClass {
public:
    // Public interface first
    
private:
    // Private implementation last
};

} // namespace hft_straddle
```

## 🧪 Testing Requirements

### Unit Testing
Every new feature must include comprehensive unit tests:

```cpp
#include <gtest/gtest.h>
#include "your_feature.h"

class YourFeatureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup
    }
    
    void TearDown() override {
        // Test cleanup
    }
};

TEST_F(YourFeatureTest, TestBasicFunctionality) {
    // Arrange
    auto calculator = VolatilityCalculator();
    
    // Act
    double result = calculator.calculate_volatility(test_data);
    
    // Assert
    EXPECT_GT(result, 0.0);
    EXPECT_LT(result, 1.0);
}

TEST_F(YourFeatureTest, TestEdgeCases) {
    // Test boundary conditions
    // Test error conditions
    // Test performance requirements
}
```

### Performance Testing
For performance-critical code, include benchmarks:

```cpp
#include <benchmark/benchmark.h>

static void BenchmarkVolatilityCalculation(benchmark::State& state) {
    auto calculator = VolatilityCalculator();
    auto test_data = generate_test_data(state.range(0));
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(calculator.calculate_volatility(test_data));
    }
}
BENCHMARK(BenchmarkVolatilityCalculation)->Range(100, 10000);
```

## ⚡ Performance Guidelines

### Latency Requirements
- **Critical path functions**: < 10 microseconds
- **Data processing**: < 100 microseconds per tick
- **Strategy calculations**: < 1 millisecond

### Memory Management
```cpp
// Prefer stack allocation
TickData tick_data;  // Good

// Avoid unnecessary heap allocations in hot paths
auto tick_data = std::make_unique<TickData>();  // Avoid in critical paths

// Use object pools for frequent allocations
ObjectPool<TickData> tick_pool(1000);
auto tick = tick_pool.acquire();
```

### Cache Optimization
```cpp
// Group frequently accessed data
struct HotData {
    double price;
    uint32_t volume;
    Timestamp time;
} __attribute__((packed));

// Minimize cache misses with sequential access
for (size_t i = 0; i < data.size(); ++i) {
    process_sequential(data[i]);  // Good
}
```

## 📚 Documentation Standards

### Code Documentation
```cpp
/**
 * @brief Calculates implied volatility using Black-Scholes model
 * 
 * This function implements the Newton-Raphson method to find the implied
 * volatility that makes the theoretical option price equal to the market price.
 * 
 * @param option_price Current market price of the option
 * @param strike_price Strike price of the option
 * @param time_to_expiry Time to expiration in years
 * @param risk_free_rate Risk-free interest rate
 * 
 * @return Implied volatility as a decimal (e.g., 0.20 for 20%)
 * 
 * @throws std::invalid_argument if any parameter is negative
 * @throws std::runtime_error if convergence fails after max iterations
 * 
 * @note This function is performance-critical and should complete < 10μs
 * 
 * @example
 * ```cpp
 * double iv = calculate_implied_volatility(5.50, 150.0, 0.25, 0.02);
 * // Returns approximately 0.23 (23% volatility)
 * ```
 */
double calculate_implied_volatility(double option_price, 
                                  double strike_price,
                                  double time_to_expiry,
                                  double risk_free_rate);
```

### README Updates
When adding features, update relevant sections:
- Usage examples
- Configuration options
- Performance characteristics
- Build requirements

## 👥 Review Process

### Pull Request Requirements
- [ ] All tests pass
- [ ] Code coverage maintained/improved
- [ ] Performance benchmarks provided
- [ ] Documentation updated
- [ ] No memory leaks (Valgrind clean)
- [ ] Code style compliant

### Review Criteria
Reviewers will evaluate:
1. **Correctness**: Does the code work as intended?
2. **Performance**: Does it meet latency requirements?
3. **Safety**: Is it memory-safe and thread-safe?
4. **Maintainability**: Is the code clear and well-organized?
5. **Testing**: Is test coverage adequate?

### Response Time
- **Initial review**: Within 48 hours
- **Follow-up reviews**: Within 24 hours
- **Final approval**: Requires 2 approvals for significant changes

## 🔧 Development Tools

### Recommended IDE Setup
- **VS Code** with C++ extensions
- **CLion** for advanced debugging
- **Compiler Explorer** for optimization analysis

### Debugging Tools
```bash
# Memory debugging
valgrind --tool=memcheck --leak-check=full ./your_program

# Performance profiling
perf record -g ./your_program
perf report

# Static analysis
cppcheck --enable=all src/
clang-tidy src/*.cpp
```

## 🤔 Questions?

If you have questions about contributing:
- **Open an issue** with the `question` label
- **Join discussions** in existing issues
- **Check the FAQ** in our documentation

## 🎉 Recognition

Contributors will be acknowledged in:
- **README.md** contributors section
- **Release notes** for significant contributions
- **GitHub Insights** contributor statistics

Thank you for helping make the HFT Straddle Options Trading System better! 🚀