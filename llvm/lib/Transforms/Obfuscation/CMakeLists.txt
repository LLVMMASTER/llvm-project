# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.13.4)

# Define the project name and the languages used
project(ControlFlowFlatteningPass LANGUAGES CXX)

# Find the LLVM package required for building the pass
find_package(LLVM REQUIRED CONFIG)

# Print out the directories of the LLVM package
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Include the necessary directories for LLVM
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Add the source file for the pass
add_llvm_library(LLVMObfuscation MODULE
  ControlFlowFlatteningPass.cpp
)

# Ensure the target is properly linked with necessary LLVM libraries
target_link_libraries(LLVMObfuscation
  LLVMCore
  LLVMSupport
  LLVMIRReader
  LLVMPasses
)

# Specify the C++ standard to use
set_target_properties(LLVMObfuscation PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED YES)

# Set the RPATH to find LLVM libraries at runtime
set_target_properties(LLVMObfuscation PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

