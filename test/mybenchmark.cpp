#include <benchmark/benchmark.h>
#include <string>
#include "engine_types.h"

enum class action_type : int8_t {
  limit = 0;
  cancel = 1;
};

struct benchmark_action {
  action_type action;
  order_t order;
  order_id cancel_id;
};

static void BM_StringCreation(benchmark::State& state) {
  for (auto _ : state)
    std::string empty_string;
}
// Register the function as a benchmark
BENCHMARK(BM_StringCreation);

// Define another benchmark
static void BM_StringCopy(benchmark::State& state) {
  std::string x = "hello";
  for (auto _ : state)
    std::string copy(x);
}
BENCHMARK(BM_StringCopy);

BENCHMARK_MAIN();