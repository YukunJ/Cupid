#include <benchmark/benchmark.h>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include "default_engine.h"
#include "engine_types.h"

namespace cupid {

enum class action_type : int8_t {
  limit = 0,
  cancel = 1,
};

typedef struct benchmark_trace {
  action_type action;
  order_t order;
  orderid_t cancel_id;
} benchmark_trace_t;

static std::vector<benchmark_trace_t> load_trace(const std::string &trace_path) {
  std::ifstream trace_file(trace_path, std::ios::in | std::ios::binary);
  assert(trace_file.is_open());
  std::vector<benchmark_trace_t> traces;
  while (!trace_file.eof()) {
    benchmark_trace_t trace;
    trace_file.read(reinterpret_cast<char *>(&trace.action), sizeof(trace.action));
    trace_file.read(reinterpret_cast<char *>(&trace.order.id), sizeof(trace.order.id));
    trace_file.read(reinterpret_cast<char *>(&trace.order.px), sizeof(trace.order.px));
    trace_file.read(reinterpret_cast<char *>(&trace.order.qty), sizeof(trace.order.qty));
    trace_file.read(reinterpret_cast<char *>(&trace.order.side), sizeof(trace.order.side));
    trace_file.read(reinterpret_cast<char *>(&trace.order.instr), INSTRUMENT_LEN);
    trace_file.read(reinterpret_cast<char *>(&trace.order.trader), TRADER_LEN);
    trace_file.read(reinterpret_cast<char *>(&trace.cancel_id), sizeof(trace.cancel_id));
    if (!trace_file) {
      break;
    }
    traces.push_back(trace);
  }
  trace_file.close();
  return traces;
}

}  // namespace cupid


static void BM_LoadTrace(benchmark::State &state) {
  auto traces = cupid::load_trace("/home/debian/Cupid/trace.bin");
  for (auto _ : state) {
    cupid::default_engine engine;
    for (const auto& trace : traces) {
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
BENCHMARK(BM_LoadTrace);

BENCHMARK_MAIN();
