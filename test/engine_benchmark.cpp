#include <benchmark/benchmark.h>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <string>
#include <vector>
#include "benchmark_engine.h"
#include "default_engine.h"
#include "engine_types.h"

namespace cupid {

enum class action_type : int8_t {
  limit = 0,
  cancel = 1,
};

struct benchmark_trace {
  action_type action;
  order_t order;
  orderid_t cancel_id;

  [[nodiscard]] constexpr bool is_limit() const noexcept { return action == action_type::limit; }

  [[nodiscard]] constexpr bool is_cancel() const noexcept { return action == action_type::cancel; }
};

static std::vector<benchmark_trace> load_trace(const std::string &trace_path) {
  std::ifstream trace_file(trace_path, std::ios::in | std::ios::binary);
  if (!trace_file.is_open()) {
    throw std::runtime_error("Failed to open trace file: " + trace_path);
  }
  std::vector<benchmark_trace> traces;
  while (!trace_file.eof()) {
    benchmark_trace trace;
    trace_file.read(reinterpret_cast<char *>(&trace.action), sizeof(trace.action));
    trace_file.read(reinterpret_cast<char *>(&trace.order.id), sizeof(trace.order.id));
    trace_file.read(reinterpret_cast<char *>(&trace.order.px), sizeof(trace.order.px));
    trace_file.read(reinterpret_cast<char *>(&trace.order.qty), sizeof(trace.order.qty));
    trace_file.read(reinterpret_cast<char *>(&trace.order.side), sizeof(trace.order.side));
    trace_file.read(reinterpret_cast<char *>(&trace.order.instr), INSTRUMENT_LEN);
    trace_file.read(reinterpret_cast<char *>(&trace.order.trader), TRADER_LEN);
    trace_file.read(reinterpret_cast<char *>(&trace.cancel_id), sizeof(trace.cancel_id));
    if (!trace_file) {  // passed the last trace
      break;
    }
    traces.push_back(trace);
  }
  trace_file.close();
  return traces;
}

}  // namespace cupid

// global default namespace

const std::vector<std::filesystem::path> trace_paths = {
    std::filesystem::path(PROJECT_ROOT_PATH) / "100k_default.bin",
    std::filesystem::path(PROJECT_ROOT_PATH) / "100k_major_cancel.bin",
    std::filesystem::path(PROJECT_ROOT_PATH) / "100k_major_depth.bin",
    std::filesystem::path(PROJECT_ROOT_PATH) / "500k_default.bin",
};

template <typename EngineType>
static void BM_Engine(benchmark::State &state) {  // NOLINT(runtime/references)
  auto traces = cupid::load_trace(trace_paths[state.range(0)].string());
  state.counters["traces"] = traces.size();
  state.counters["limit_order"] = std::accumulate(
      traces.begin(), traces.end(), 0ULL,
      [](uint64_t accumulator, const auto &trace) { return accumulator + static_cast<uint64_t>(trace.is_limit()); });
  state.counters["cancel_order"] = std::accumulate(
      traces.begin(), traces.end(), 0ULL,
      [](uint64_t accumulator, const auto &trace) { return accumulator + static_cast<uint64_t>(trace.is_cancel()); });
  state.counters["memory_mb"] =
      benchmark::Counter(static_cast<double>(traces.size()) * sizeof(cupid::benchmark_trace) / (1024.0 * 1024.0));
  state.counters["operations_per_second"] =
      benchmark::Counter(static_cast<double>(traces.size()), benchmark::Counter::kIsRate);

  for (auto _ : state) {
    EngineType engine;
    for (const auto &trace : traces) {
      if (trace.action == cupid::action_type::limit) {
        engine.limit(trace.order);
      } else if (trace.action == cupid::action_type::cancel) {
        engine.cancel(trace.cancel_id);
      } else {
        assert(false);
      }
    }
  }
}

BENCHMARK_TEMPLATE(BM_Engine, cupid::benchmark_engine)
    ->Name("BenchmarkEngine/100k_default")
    ->Args({0})
    ->Unit(benchmark::kMillisecond)
    ->Iterations(3)
    ->MeasureProcessCPUTime();

BENCHMARK_TEMPLATE(BM_Engine, cupid::benchmark_engine)
    ->Name("BenchmarkEngine/100k_major_cancel")
    ->Args({1})
    ->Unit(benchmark::kMillisecond)
    ->Iterations(3)
    ->MeasureProcessCPUTime();

BENCHMARK_TEMPLATE(BM_Engine, cupid::benchmark_engine)
    ->Name("BenchmarkEngine/100k_major_depth")
    ->Args({2})
    ->Unit(benchmark::kMillisecond)
    ->Iterations(3)
    ->MeasureProcessCPUTime();

BENCHMARK_TEMPLATE(BM_Engine, cupid::benchmark_engine)
    ->Name("BenchmarkEngine/500K_default")
    ->Args({3})
    ->Unit(benchmark::kMillisecond)
    ->Iterations(1)
    ->MeasureProcessCPUTime();

BENCHMARK_MAIN();
