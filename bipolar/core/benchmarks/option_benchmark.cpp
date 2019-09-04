#include "bipolar/core/option.hpp"

#include <string>
#include <cstdint>

#include <benchmark/benchmark.h>

using namespace std::literals;

static void BM_old_style(benchmark::State& state) {
    const std::size_t n = state.range(0);
    const auto fn = [](std::size_t arg, std::string& out) -> bool {
        if (arg % 2 == 0) {
            return false;
        }
        out.assign("foo");
        return true;
    };

    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) {
            std::string s;
            const bool b = fn(i, s);
            benchmark::DoNotOptimize(s);
            benchmark::DoNotOptimize(b);
        }
    }
}
BENCHMARK(BM_old_style)->Range(1, 1024);

static void BM_bipolar_option(benchmark::State& state) {
    const std::size_t n = state.range(0);
    const auto fn = [](std::size_t arg) -> bipolar::Option<std::string> {
        if (arg % 2 == 0) {
            return bipolar::None;
        }
        return bipolar::Some("foo"s);
    };

    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) {
            auto opt = fn(i);
            benchmark::DoNotOptimize(opt);
        }
    }
}
BENCHMARK(BM_bipolar_option)->Range(1, 1024);
