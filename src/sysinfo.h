#ifndef BENCHMARK_SYSINFO_H_
#define BENCHMARK_SYSINFO_H_

#include <string>

namespace benchmark {
double MyCPUUsage();
double ChildrenCPUUsage();
int NumCPUs();
double CyclesPerSecond();
bool CpuScalingEnabled();
std::string CPUModel();
}  // end namespace benchmark

#endif  // BENCHMARK_SYSINFO_H_
