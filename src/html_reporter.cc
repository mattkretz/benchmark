#include "benchmark/reporter.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "benchmark_util.h"
#include "walltime.h"

static const char *const high_chart_bar_function =
    "\n$('#${BENCHMARK_ID}').highcharts({"
    "\n  chart: {"
    "\n    type: 'column',"
    "\n    zoomType: 'xy'"
    "\n  },"
    "\n  title: {"
    "\n    text: '${BENCHMARK_NAME}'"
    "\n  },"
    "\n  legend: {"
    "\n    enabled: false"
    "\n  },"
    "\n  xAxis: { type: 'category' },"
    "\n  yAxis: {"
    "\n    title: {"
    "\n      text: '${BENCHMARK_NAME} [${UNIT}]'"
    "\n    },"
    "\n    allowDecimals: true"
    "\n  },"
    "\n  plotOptions: {"
    "\n    column: { colorByPoint: true }"
    "\n   ,errorbar: {"
    "\n      tooltip: {"
    "\n        shared: true"
    "\n       ,valueDecimals: 3"
    "\n       ,pointFormat: ' (<small>stddev:</small> {point.low} – {point.high})'"
    "\n      }"
    "\n    }"
    "\n  },"
    "\n  tooltip: {"
    "\n    shared: true"
    "\n   ,useHTML: true"
    "\n   ,valueDecimals: 3"
    "\n   ,headerFormat: '<small>{point.key}</small>'"
    "\n   ,pointFormat: '<br/><span style=\"color: "
    "{series.color}\">{series.name}:</span> <b>{point.y}</b>'"
    "\n   ,valueSuffix: ' ${UNIT}'"
    "\n  },"
    "\n  scrollbar: {"
    "\n    enabled: true"
    "\n  },"
    "\n  series:[${SERIES}]"
    "\n});"
    "${CHART}";

static const char *const high_chart_line_function =
    "\n$('#${BENCHMARK_ID}').highcharts({"
    "\n  chart: {"
    "\n    type: 'line',"
    "\n    zoomType: 'xy'"
    "\n  },"
    "\n  title: {"
    "\n    text: '${BENCHMARK_NAME}'"
    "\n  },"
    "\n  legend: {"
    "\n    align: 'right',"
    "\n    verticalAlign: 'top',"
    "\n    layout: 'vertical'"
    "\n  },"
    "\n  xAxis: {"
    "\n    title: {"
    "\n      text: 'Argument 1',"
    "\n      enabled: true"
    "\n    },"
    "\n    allowDecimals: true"
    "\n  },"
    "\n  yAxis: {"
    "\n    min: 0,"
    "\n    title: {"
    "\n      text: '${BENCHMARK_NAME} [${UNIT}]'"
    "\n    },"
    "\n    allowDecimals: true"
    "\n  },"
    "\n  tooltip: {"
    "\n    shared: true"
    "\n   ,useHTML: true"
    "\n   ,valueDecimals: 3"
    "\n   ,headerFormat: '<small>{point.key}</small><table><tr>'"
    "\n   ,pointFormat: '</tr><tr>"
    "<td style=\"color: {series.color}\">{series.name}:</td>"
    "<td style=\"text-align:right\"><b>{point.y}</b></td>'"
    "\n   ,footerFormat: '</tr></table>'"
    "\n   ,valueSuffix: ' ${UNIT}'"
    "\n  },"
    "\n  scrollbar: {"
    "\n    enabled: true"
    "\n  },"
    "\n  navigator: {"
    "\n    enabled: true"
    "\n  },"
    "\n  plotOptions: {"
    "\n    errorbar: {"
    "\n      tooltip: {"
    "\n        shared: true"
    "\n       ,useHTML: true"
    "\n       ,valueDecimals: 3"
    "\n       ,pointFormat: '<td>(<small>stddev:</small> {point.low} – {point.high})</td>'"
    "\n      }"
    "\n    }"
    "\n   ,line: {"
    "\n      marker: {"
    "\n        enabled: true,"
    "\n        radius: 3"
    "\n      },"
    "\n      lineWidth: 1"
    "\n    }"
    "\n  },"
    "\n  series:[${SERIES}]"
    "\n});"
    "${CHART}";

static const char *const div_element =
    "      <div class = \"chart\" id = \"${DIV_NAME}\"></div>\n${DIV}";

static const char *const html_base =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "   <head>\n"
    "       <meta charset = \"UTF-8\"/>\n"
    "       <title>${TITLE}</title>\n"
    "       <style type = \"text/css\">\n"
    "           .chart{\n"
    "                height:700px;\n"
    "           }\n"
    "       </style>\n"
    "@CMAKE_JAVASCRIPT_REPLACEMENT@"
    "       <script>\n"
    "            $(function () {\n"
    "${CHART}"
    "            });\n"
    "       </script>\n"
    "   </head>\n"
    "   <body>\n"
    "${DIV}"
    "${CONTEXT}"
    "   </body>\n"
    "</html>";

namespace benchmark {
namespace {
std::string ReplaceTags(
    std::string boilerplate,
    const std::unordered_map<std::string, std::string> &replacements) {
    size_t pos = 0;
    while (std::string::npos != (pos = boilerplate.find("${", pos))) {
      if (pos + 3 >= boilerplate.size()) {
        break;
      }
      const size_t end = boilerplate.find('}', pos + 2);
      if (end == std::string::npos) {
        break;
      }
      const auto &key = boilerplate.substr(pos + 2, end - pos - 2);
      const auto &it = replacements.find(key);
      if (it == replacements.end()) {
        std::clog << "WARN: No replacement for HTML template '" << key
                  << "'. Skipping.\n";
        pos = end + 1;
        continue;
      }
      boilerplate.replace(pos, end - pos + 1, it->second);
      pos += it->second.size();
    }
    return std::move(boilerplate);
}

using Run = BenchmarkReporter::Run;

struct ChartIt {
  const char *const chart_title;
  const char *const unit;
  bool (*is_valid)(const Run &);
  std::string (*value)(const Run &);
  std::function<std::string(const Run &, const Run &)> error;
};

static double fixup_time(double accumulated_seconds, double iterations) {
  constexpr double nano_sec_multiplier = 1e9;
  const double r = accumulated_seconds * nano_sec_multiplier;
  return iterations >= 2 ? r / iterations : r;
}
static const std::array<ChartIt, HTMLReporter::NumberOfPresentations> charts = {
    {{"Real time", "ns",
      [](const Run &mean) { return mean.real_accumulated_time > 0; },
      [](const Run &mean) {
        return std::to_string(
            fixup_time(mean.real_accumulated_time, mean.iterations));
      },
      [](const Run &mean, const Run &stddev) {
        const double value =
            fixup_time(mean.real_accumulated_time, mean.iterations);
        const double deviation =
            fixup_time(stddev.real_accumulated_time, stddev.iterations);
        return std::to_string(value - deviation) + ',' +
               std::to_string(value + deviation);
      }},
     {"CPU time", "ns",
      [](const Run &mean) { return mean.cpu_accumulated_time > 0; },
      [](const Run &mean) {
        return std::to_string(
            fixup_time(mean.cpu_accumulated_time, mean.iterations));
      },
      [](const Run &mean, const Run &stddev) {
        const double value =
            fixup_time(mean.cpu_accumulated_time, mean.iterations);
        const double deviation =
            fixup_time(stddev.cpu_accumulated_time, stddev.iterations);
        return std::to_string(value - deviation) + ',' +
               std::to_string(value + deviation);
      }},
     {"Items per second", "items/s",
      [](const Run &mean) { return mean.items_per_second > 0; },
      [](const Run &mean) { return std::to_string(mean.items_per_second); },
      [](const Run &mean, const Run &stddev) {
        const double value = mean.items_per_second;
        const double deviation = stddev.items_per_second;
        return std::to_string(value - deviation) + ',' +
               std::to_string(value + deviation);
      }},
     {"Bytes per second", "B/s",
      [](const Run &mean) { return mean.bytes_per_second > 0; },
      [](const Run &mean) { return std::to_string(mean.bytes_per_second); },
      [](const Run &mean, const Run &stddev) {
        const double value = mean.bytes_per_second;
        const double deviation = stddev.bytes_per_second;
        return std::to_string(value - deviation) + ',' +
               std::to_string(value + deviation);
      }}}};

HTMLReporter::Presentation &operator++(HTMLReporter::Presentation &p) {
  return p = static_cast<HTMLReporter::Presentation>(p + 1);
}
HTMLReporter::Presentation next(HTMLReporter::Presentation p) {
  return static_cast<HTMLReporter::Presentation>(p + 1);
}

std::string ReadFile(const std::string &file) {
  if (file.empty()) {
    return {};
  }
  std::fstream fin(file);
  const auto start = fin.tellg();
  fin.seekg(0, std::ios::end);
  const auto length = fin.tellg() - start;
  fin.seekg(start);
  std::string r;
  r.resize(length, ' ');
  fin.read(&*r.begin(), length);
  fin.close();
  return r;
}
}

bool HTMLReporter::ReportContext(const Context &context) {
  std::ostringstream stream;
  stream << "<div>Run on (" << context.num_cpus << " X " << context.mhz_per_cpu
         << " MHz CPU" << ((context.num_cpus > 1) ? "s" : "") << ")<br/>"
         << LocalDateTimeString() << "<br/>";
  if (context.cpu_scaling_enabled) {
    stream << "CPU scaling was enabled. The benchmark real time measurements "
              "may be noisy and with extra overhead.<br/>";
    std::cerr << "***WARNING*** CPU scaling is enabled, the benchmark "
                 "real time measurements may be noisy and will incur extra "
                 "overhead.\n";
  }

#ifndef NDEBUG
  stream << "The benchmark library was built in Debug mode. Timings may be "
            "affected.<br/>";
  std::cerr << "***WARNING*** Library was built as DEBUG. Timings may be "
               "affected.\n";
#endif
  stream << "</div>";
  context_output = stream.str();
  return true;
}

void HTMLReporter::ReportRuns(std::vector<Run> const &reports) {
  if (reports[0].has_arg2) {
    std::clog << "Warning for Benchmark \"" << reports[0].benchmark_name
              << "\": 3D plotting is not implemented! Data will be plotted as "
                 "a bar chart.\n";
  }
  const Run *run = &reports[0];
  const bool need_stddev = reports.size() > 1;
  const bool is_barchart = reports[0].has_arg2 || !reports[0].has_arg1;
  const std::string &caption =
      is_barchart
          ? ReplaceHTMLSpecialChars(run->benchmark_name)
          : GenerateInstanceName(ReplaceHTMLSpecialChars(run->benchmark_family),
                                 0, 0, 0, run->min_time, run->use_real_time,
                                 run->multithreaded, run->threads);
  ChartData *const chart_data_array =
      is_barchart ? barcharts : linecharts[caption];
  Run mean, run_stddev;
  if (need_stddev) {
    // we need to plot the mean and stddev of all the values in reports
    BenchmarkReporter::ComputeStats(reports, &mean, &run_stddev);
    run = &mean;
  }

  for (Presentation i = FirstPresentation; i < NumberOfPresentations; ++i) {
    ChartData &chart_data = chart_data_array[i];
    if (!charts[i].is_valid(*run)) {
      continue;
    }
    std::stringstream values;
    std::string &stddev = chart_data.stddev;
    values << ",[";
    if (need_stddev) {
      stddev.append(",[");
    }
    const auto &x_string =
        is_barchart ? '\'' + caption + '\'' : std::to_string(reports[0].arg1);
    values << x_string << ',' << charts[i].value(*run) << ']';
    chart_data.values.append(values.str());
    if (need_stddev) {
      stddev.append(x_string)
          .append(1, ',')
          .append(charts[i].error(*run, run_stddev))
          .append(1, ']');
    }
  }
}

HTMLReporter::ChartOutput HTMLReporter::LineChartOutput(
    Presentation p) {
  if (p == NumberOfPresentations) {
    return BarChartOutput(FirstPresentation);
  }
  ChartOutput next_output = LineChartOutput(next(p));
  std::stringstream concat;
  for (const auto &data : linecharts) {
    const std::string &name = data.first;
    const ChartData &chart_data = data.second[p];
    if (chart_data.values.empty()) {
      return next_output;
    }
    concat << ",{name: '" << name << "', data: ["
           << chart_data.values.substr(1);  // drop the initial comma
    if (!chart_data.stddev.empty()) {
      concat << "]}, {"
                "\n  type: 'errorbar'"
                "\n ,data: ["
             << chart_data.stddev.substr(1);  // drop the initial comma
    }
    concat << "]}";
  }
  if (concat.tellp() == 0) {
    return next_output;
  }
  const auto &div_id = std::string("linechart") + std::to_string(p);
  return {ReplaceTags(high_chart_line_function,
                      {{"BENCHMARK_ID", div_id},
                       {"BENCHMARK_NAME", charts[p].chart_title},
                       {"CHART", next_output.chart},
                       {"SERIES", concat.str().substr(1)},
                       {"UNIT", charts[p].unit}}),
          ReplaceTags(div_element,
                      {{"DIV_NAME", div_id}, {"DIV", next_output.div}})};
}

HTMLReporter::ChartOutput HTMLReporter::BarChartOutput(
    Presentation p) {
  if (p == NumberOfPresentations) {
    return {{}, {}};
  }
  ChartOutput next_output = BarChartOutput(next(p));
  const ChartData &chart_data = barcharts[p];
  if (chart_data.values.empty()) {
    return next_output;
  }
  const auto &div_id = std::string("barchart") + std::to_string(p);
  return {
      ReplaceTags(
          high_chart_bar_function,
          {{"BENCHMARK_ID", div_id},
           {"BENCHMARK_NAME", charts[p].chart_title},
           {"CHART", next_output.chart},
           {"SERIES",
            std::string("{name: '") + charts[p].chart_title + "', data: [" +
                chart_data.values.substr(1)  // drop the initial comma
                + (chart_data.stddev.empty()
                       ? "]}"
                       : ("]},\n{type: 'errorbar', data: [" +
                          chart_data.stddev.substr(1)  // drop the initial comma
                          + "]}"))},
           {"UNIT", charts[p].unit}}),
      ReplaceTags(div_element,
                  {{"DIV_NAME", div_id}, {"DIV", next_output.div}})};
}

void HTMLReporter::Finalize() {
  const ChartOutput &output = LineChartOutput(FirstPresentation);
  std::cout << ReplaceTags(
      html_base, {{"CHART", output.chart},
                  {"CONTEXT", context_output},
                  {"DIV", output.div},
                  {"HIGHCHART_MORE", ReadFile("@CMAKE_HIGHSTOCK_MORE_PATH@")},
                  {"HIGHCHART", ReadFile("@CMAKE_HIGHSTOCK_PATH@")},
                  {"JQUERY", ReadFile("@CMAKE_JQUERY_PATH@")},
                  {"TITLE", "Benchmark"}});
}

std::string HTMLReporter::ReplaceHTMLSpecialChars(
    const std::string &label) const {
  std::string new_label(label);

  for (auto pos = new_label.find_first_of("<'>"); pos != std::string::npos;
       pos = new_label.find_first_of("<'>", pos)) {
    switch (new_label[pos]) {
      //< and > can't displayed by highcharts, so they have to be replaced
      case '<':
        new_label.replace(pos, 1, "⋖");
        break;
      case '>':
        new_label.replace(pos, 1, "⋗");
        break;
      // For ' it is enough to set a \ before it
      case '\'':
        new_label.insert(pos, 1, '\\');
        pos++;
        break;
    }
  }

  return new_label;
}
}  // end namespace benchmark
