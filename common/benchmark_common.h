#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace slc {

struct BenchmarkItem {
    const char *name;
    int repeat_count;
};

struct BenchmarkSample {
    std::string name;
    int repeat_count = 0;
    double best_ms = 0.0;
    double median_ms = 0.0;
    double average_ms = 0.0;
    std::uint64_t checksum = 0;
    std::vector<double> runs_ms;
};

inline std::uint64_t g_sink = 0;

inline void consume(std::uint64_t value) {
    g_sink ^= value + 0x9e3779b97f4a7c15ULL + (g_sink << 6) + (g_sink >> 2);
}

inline std::string format_double(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << value;
    return out.str();
}

inline std::string json_escape(const std::string &text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (char ch : text) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}

template <typename Fn>
BenchmarkSample run_benchmark_sample(const BenchmarkItem &item, Fn &&fn) {
    using clock = std::chrono::steady_clock;

    BenchmarkSample sample;
    sample.name = item.name;
    sample.repeat_count = item.repeat_count;

    std::uint64_t checksum = 0;
    for (int i = 0; i < 2; ++i) {
        checksum ^= static_cast<std::uint64_t>(fn(item.repeat_count));
    }

    constexpr int measured_runs = 7;
    sample.runs_ms.reserve(measured_runs);
    for (int i = 0; i < measured_runs; ++i) {
        const auto start = clock::now();
        checksum ^= static_cast<std::uint64_t>(fn(item.repeat_count));
        const auto end = clock::now();
        const std::chrono::duration<double, std::milli> elapsed = end - start;
        sample.runs_ms.push_back(elapsed.count());
    }

    auto sorted = sample.runs_ms;
    std::sort(sorted.begin(), sorted.end());
    sample.best_ms = sorted.front();
    sample.median_ms = sorted[sorted.size() / 2];
    sample.average_ms = std::accumulate(sorted.begin(), sorted.end(), 0.0) / sorted.size();
    sample.checksum = checksum;
    consume(checksum);
    return sample;
}

inline void write_results_json(const std::filesystem::path &path,
                               const std::string &engine,
                               const std::vector<BenchmarkSample> &samples) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("failed to open results file: " + path.string());
    }

    out << "{\n";
    out << "  \"engine\": \"" << json_escape(engine) << "\",\n";
    out << "  \"samples\": [\n";
    for (std::size_t i = 0; i < samples.size(); ++i) {
        const auto &sample = samples[i];
        out << "    {\n";
        out << "      \"name\": \"" << json_escape(sample.name) << "\",\n";
        out << "      \"repeat_count\": " << sample.repeat_count << ",\n";
        out << "      \"best_ms\": " << std::fixed << std::setprecision(6) << sample.best_ms << ",\n";
        out << "      \"median_ms\": " << std::fixed << std::setprecision(6) << sample.median_ms << ",\n";
        out << "      \"average_ms\": " << std::fixed << std::setprecision(6) << sample.average_ms << ",\n";
        out << "      \"checksum\": " << sample.checksum << ",\n";
        out << "      \"runs_ms\": [";
        for (std::size_t j = 0; j < sample.runs_ms.size(); ++j) {
            if (j != 0) {
                out << ", ";
            }
            out << std::fixed << std::setprecision(6) << sample.runs_ms[j];
        }
        out << "]\n";
        out << "    }" << (i + 1 == samples.size() ? "\n" : ",\n");
    }
    out << "  ]\n";
    out << "}\n";
}

} // namespace slc
