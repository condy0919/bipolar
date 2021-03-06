#include "bipolar/core/function.hpp"

#include <benchmark/benchmark.h>

using ValueType = benchmark::State::StateIterator::Value;

template <typename Maker>
static void do_benchmark(benchmark::State& state, Maker make) {
    auto lambda = [](ValueType i) {
        benchmark::DoNotOptimize(i);
        return i;
    };

    for (auto _ : state) {
        auto f = make(lambda);
        benchmark::DoNotOptimize(f(_));
    }
}

static void BM_std_function(benchmark::State& state) {
    do_benchmark(state, [](auto& f) {
        return std::function<ValueType(ValueType)>(f);
    });
}
BENCHMARK(BM_std_function);

static void BM_bipolar_function(benchmark::State& state) {
    do_benchmark(state, [](auto& f) {
        return bipolar::Function<ValueType(ValueType)>(f);
    });
}
BENCHMARK(BM_bipolar_function);
