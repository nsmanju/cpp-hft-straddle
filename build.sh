#!/bin/bash

# ===================================================================
#                    HFT STRADDLE SYSTEM BUILD SCRIPT
# ===================================================================
# 
# Automated build script for beginners
# This script handles the complete build process with error checking
# 
# Usage: ./build.sh [clean|debug|release|profile]
# 
# ===================================================================

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Build configuration
BUILD_TYPE="Release"
BUILD_DIR="build"
JOBS=$(nproc)

# Parse command line arguments
case "${1:-}" in
    "clean")
        echo -e "${YELLOW}Cleaning build directory...${NC}"
        rm -rf ${BUILD_DIR}
        echo -e "${GREEN}✓ Clean complete${NC}"
        exit 0
        ;;
    "debug")
        BUILD_TYPE="Debug"
        echo -e "${BLUE}Building in Debug mode${NC}"
        ;;
    "release")
        BUILD_TYPE="Release"
        echo -e "${BLUE}Building in Release mode (optimized)${NC}"
        ;;
    "profile")
        BUILD_TYPE="RelWithDebInfo"
        echo -e "${BLUE}Building with profiling information${NC}"
        ;;
    "")
        echo -e "${BLUE}Building in default Release mode${NC}"
        ;;
    *)
        echo -e "${RED}Usage: $0 [clean|debug|release|profile]${NC}"
        exit 1
        ;;
esac

echo -e "${YELLOW}"
echo "==============================================="
echo "    HFT STRADDLE SYSTEM BUILD SCRIPT"
echo "==============================================="
echo -e "${NC}"

# Check dependencies
echo -e "${BLUE}Checking dependencies...${NC}"

# Check for cmake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}✗ CMake not found. Please install cmake${NC}"
    echo "  Ubuntu/Debian: sudo apt install cmake"
    echo "  CentOS/RHEL:   sudo yum install cmake"
    exit 1
fi
echo -e "${GREEN}✓ CMake found: $(cmake --version | head -1)${NC}"

# Check for make
if ! command -v make &> /dev/null; then
    echo -e "${RED}✗ Make not found. Please install build-essential${NC}"
    echo "  Ubuntu/Debian: sudo apt install build-essential"
    echo "  CentOS/RHEL:   sudo yum groupinstall 'Development Tools'"
    exit 1
fi
echo -e "${GREEN}✓ Make found${NC}"

# Check for g++
if ! command -v g++ &> /dev/null; then
    echo -e "${RED}✗ G++ compiler not found${NC}"
    echo "  Ubuntu/Debian: sudo apt install g++"
    echo "  CentOS/RHEL:   sudo yum install gcc-c++"
    exit 1
fi
echo -e "${GREEN}✓ G++ found: $(g++ --version | head -1)${NC}"

# Optional dependencies
if command -v pkg-config &> /dev/null && pkg-config --exists numa; then
    echo -e "${GREEN}✓ NUMA support available${NC}"
else
    echo -e "${YELLOW}⚠ NUMA support not found (optional optimization)${NC}"
    echo "  Ubuntu/Debian: sudo apt install libnuma-dev"
    echo "  CentOS/RHEL:   sudo yum install numa-devel"
fi

if pkg-config --exists libcurl; then
    echo -e "${GREEN}✓ libcurl found${NC}"
else
    echo -e "${YELLOW}⚠ libcurl not found (required for market data feeds)${NC}"
    echo "  Ubuntu/Debian: sudo apt install libcurl4-openssl-dev"
    echo "  CentOS/RHEL:   sudo yum install libcurl-devel"
fi

echo ""

# Create build directory
echo -e "${BLUE}Creating build directory...${NC}"
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# Configure with CMake
echo -e "${BLUE}Configuring build with CMake...${NC}"
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      .. || {
    echo -e "${RED}✗ CMake configuration failed${NC}"
    exit 1
}
echo -e "${GREEN}✓ CMake configuration successful${NC}"

# Build the project
echo -e "${BLUE}Building project with ${JOBS} parallel jobs...${NC}"
make -j${JOBS} || {
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
}
echo -e "${GREEN}✓ Build successful${NC}"

# Display binary information
if [ -f "hft_straddle" ]; then
    echo ""
    echo -e "${GREEN}Build completed successfully!${NC}"
    echo -e "${BLUE}Binary location: ${BUILD_DIR}/hft_straddle${NC}"
    echo -e "${BLUE}Binary size: $(du -h hft_straddle | cut -f1)${NC}"
    
    # Check if it's optimized
    if [ "${BUILD_TYPE}" = "Release" ]; then
        echo -e "${GREEN}✓ Optimized build (Release mode)${NC}"
    elif [ "${BUILD_TYPE}" = "Debug" ]; then
        echo -e "${YELLOW}⚠ Debug build (not optimized)${NC}"
    fi
    
    echo ""
    echo -e "${YELLOW}Quick start:${NC}"
    echo "  cd ${BUILD_DIR}"
    echo "  ./hft_straddle"
    echo ""
    echo -e "${YELLOW}Or run directly:${NC}"
    echo "  ${BUILD_DIR}/hft_straddle"
    
else
    echo -e "${RED}✗ Binary not found after build${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}Build script completed successfully!${NC}"