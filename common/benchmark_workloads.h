#pragma once

#include "benchmark_common.h"

#include <vector>

namespace slc {

inline const std::vector<BenchmarkItem> &benchmark_items() {
    static const std::vector<BenchmarkItem> items = {
        {"native_loop", 1},
        {"fibonacci_loop", 3},
        {"fibonacci_recursive", 1},
        {"mandelbrot", 1},
    };
    return items;
}

} // namespace slc
