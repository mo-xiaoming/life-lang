# Enable export of compile commands for tools like clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable verbose build output
option(ENABLE_VERBOSE_BUILD "Enable verbose build output" OFF)

if(ENABLE_VERBOSE_BUILD)
  set(CMAKE_VERBOSE_MAKEFILE ON)
  set(CMAKE_RULE_MESSAGES OFF)
endif()

# Set default build type to asan/ubsan debug if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

message(STATUS "build type: ${CMAKE_BUILD_TYPE}")

# Enable IPO/LTO for Release builds if supported
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT result OUTPUT output)

  if(result)
    message(STATUS "supports IPO/LTO")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_Release TRUE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RelWithDebInfo TRUE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MinSizeRel TRUE)
  else()
    message(WARNING "IPO/LTO not supported: ${output}")
  endif()
endif()

# Define default compile features
add_library(default_compile_features INTERFACE)
target_compile_features(default_compile_features INTERFACE cxx_std_20)

# Define default compile options
add_library(default_compile_options INTERFACE)

# remove -D_GLIBCXX_DEBUG, it causes other libraries, like gtest, built without these flags crash
target_compile_options(default_compile_options INTERFACE "$<$<NOT:$<CONFIG:Release>>:-U_FORTIFY_SOURCE;-D_GLIBCXX_ASSERTIONS;-O0;-ggdb3;-fno-omit-frame-pointer;-fno-inline;-fno-sanitize-recover=all>")

# target_compile_options(default_compile_options INTERFACE -fno-exceptions -fno-rtti)
target_compile_options(default_compile_options INTERFACE "$<$<CONFIG:Release>:-march=native>")
target_compile_options(default_compile_options INTERFACE "$<$<CXX_COMPILER_ID:GNU>:-fdiagnostics-color=always>")
target_compile_options(default_compile_options INTERFACE "$<$<CXX_COMPILER_ID:AppleClang,Clang>:-fcolor-diagnostics>")

# Define default compile warnings
add_library(default_compile_warnings INTERFACE)
set(CMAKE_CXX_FLAGS_DEBUG "")
target_compile_options(default_compile_warnings INTERFACE "$<$<CONFIG:Release>:-Werror;-Wfatal-errors>")
target_compile_options(default_compile_warnings INTERFACE -Wall -Wextra -Wshadow -Wold-style-cast -Wcast-align -Wcast-qual -Wunused -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wfloat-equal -Wmissing-include-dirs -Wredundant-decls -Wundef -Wzero-as-null-pointer-constant)
target_compile_options(default_compile_warnings INTERFACE "$<$<CXX_COMPILER_ID:GNU>:-Wmisleading-indentation;-Wduplicated-cond;-Wduplicated-branches;-Wlogical-op;-Wuseless-cast>")

add_library(default_sanitizer_compile_options INTERFACE)
add_library(default_sanitizer_link_options INTERFACE)
target_compile_options(default_sanitizer_compile_options INTERFACE -fsanitize=address,undefined -fno-optimize-sibling-calls -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow)
target_link_options(default_sanitizer_link_options INTERFACE -fsanitize=address,undefined -fno-optimize-sibling-calls -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow)
target_compile_options(default_sanitizer_compile_options INTERFACE "$<$<CXX_COMPILER_ID:AppleClang,Clang>:-fsanitize=local-bounds,float-divide-by-zero,implicit-conversion,nullability,integer>")
target_link_options(default_sanitizer_link_options INTERFACE "$<$<CXX_COMPILER_ID:AppleClang,Clang>:-fsanitize=local-bounds,float-divide-by-zero,implicit-conversion,nullability,integer>")

# Define default sanitizer compile options
option(ENABLE_ASAN_AND_UBSAN "Enable AddressSanitizer" OFF)

set(SANITIZER_OPTIONS)

if(ENABLE_ASAN_AND_UBSAN)
  message(STATUS "Enabling AddressSanitizer and UndefinedBehaviorSanitizer")
  set(SANITIZER_OPTIONS default_sanitizer_compile_options default_sanitizer_link_options)
endif()

# Define project defaults, it can be added to any target with target_link_libraries
add_library(project_defaults INTERFACE)
target_link_libraries(project_defaults INTERFACE
  default_compile_features
  default_compile_options
  default_compile_warnings
  ${SANITIZER_OPTIONS}
)
