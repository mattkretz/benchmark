#include "benchmark/reporter.h"

#include <exception>
#include <iostream>
#include <sstream>

class TestError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

template <typename T>
void compare(T x, T y, const std::string &x_string, const std::string &y_string,
             const std::string &file, int line) {
  if (x == y) {
    return;
  }
  using std::to_string;
  const auto x_value = to_string(x);
  const auto y_value = to_string(y);
  std::stringstream what;
  what << file << ':' << line << ": COMPARE(" << x_string;
  if (x_value != x_string) {
    what << " [value: " << x_value << ']';
  }
  what << ", " << y_string;
  if (y_value != y_string) {
    what << " [value: " << y_value << ']';
  }
  what << ") failed.";
  throw TestError(what.str());
}
#define COMPARE(x, y) compare(x, y, #x, #y, __FILE__, __LINE__)

int main() {
  try {
    benchmark::BenchmarkReporter::Context context = {1, 1234., true, 12, "Benchmark"};
    std::vector<benchmark::BenchmarkReporter::Run> runs;
    runs.resize(2);
    runs[0].benchmark_name = "TestHTML";
    runs[0].benchmark_family = "TestHTML";
    runs[0].iterations = 123456789;
    runs[0].real_accumulated_time = 1.;
    runs[0].cpu_accumulated_time = 2.;
    runs[0].bytes_per_second = 0;
    runs[0].items_per_second = 4.;
    runs[1].benchmark_name = "TestHTML";
    runs[1].benchmark_family = "TestHTML";
    runs[1].iterations = 123456789;
    runs[1].real_accumulated_time = 1.5;
    runs[1].cpu_accumulated_time = 2.5;
    runs[1].bytes_per_second = 0;
    runs[1].items_per_second = 4.5;
    benchmark::HTMLReporter reporter;
    reporter.ReportContext(context);
    reporter.ReportRuns(runs);
    reporter.ReportRuns(runs);
    runs.resize(1);
    runs[0].benchmark_name = "TestHTML2";
    runs[0].benchmark_family = "TestHTML2";
    runs[0].iterations = 123456789;
    runs[0].real_accumulated_time = 1.1;
    runs[0].cpu_accumulated_time = 2.1;
    runs[0].bytes_per_second = 0;
    runs[0].items_per_second = 4.1;
    reporter.ReportRuns(runs);
    reporter.ReportRuns(runs);
    runs[0].has_arg1 = true;
    for (int i = 0; i < 10; ++i) {
      runs[0].arg1 = i;
      runs[0].real_accumulated_time += 1;
      reporter.ReportRuns(runs);
    }
    runs[0].arg1 = 10;
    runs[0].real_accumulated_time += 1;
    runs.push_back(runs[0]);
    runs[1].real_accumulated_time += 1;
    runs[1].cpu_accumulated_time += 1;
    auto runs2 = runs;
    runs2[0].benchmark_name = "Other";
    runs2[0].benchmark_family = runs2[0].benchmark_name;
    runs2[1].benchmark_name = runs2[0].benchmark_name;
    runs2[1].benchmark_family = runs2[0].benchmark_name;
    for (int i = 0; i < 10; ++i) {
      runs2[0].arg1 = i;
      runs2[0].real_accumulated_time += 1;
      runs2[0].cpu_accumulated_time -= 0.3;
      reporter.ReportRuns(runs2);
    }
    reporter.ReportRuns(runs);
    reporter.Finalize();
  } catch (const TestError &err) {
    std::cerr << err.what() << std::endl;
    return 1;
  }
  return 0;
}
