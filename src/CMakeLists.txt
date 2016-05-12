# Allow the source files to find headers in src/
include_directories(${PROJECT_SOURCE_DIR}/src)

# Set JavaScript paths for the HTML reporter
set(_js_dependencies)
set(CMAKE_JAVASCRIPT_INCLUDES "")
macro(add_javascript_dependency url_prefix filename)
  set(_url "${url_prefix}${filename}")
  if(BENCHMARK_DOWNLOAD_JS)
    file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/share/benchmark")
    set(CMAKE_JAVASCRIPT_INCLUDES
      "${CMAKE_JAVASCRIPT_INCLUDES} {\"${CMAKE_INSTALL_PREFIX}/share/benchmark/${filename}\", \"${_url}\"},")
    set(_js_dependencies ${_js_dependencies} "${PROJECT_BINARY_DIR}/share/benchmark/${filename}")
    add_custom_command(OUTPUT "${PROJECT_BINARY_DIR}/share/benchmark/${filename}"
      COMMAND ${CMAKE_COMMAND} "-Dfilename=${filename}" "-Durl=${_url}"
      -P ${PROJECT_SOURCE_DIR}/cmake/download.cmake
      DEPENDS ${PROJECT_SOURCE_DIR}/cmake/download.cmake
      WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/share/benchmark"
      COMMENT "Downloading ${_url}"
      VERBATIM)
  else()
    set(CMAKE_JAVASCRIPT_INCLUDES "${CMAKE_JAVASCRIPT_INCLUDES} {\"\", \"${_url}\"},")
  endif()
endmacro()
add_javascript_dependency("https://code.jquery.com/" "jquery-2.2.1.min.js")
add_javascript_dependency("https://code.highcharts.com/stock/" "highstock.js")
add_javascript_dependency("https://code.highcharts.com/stock/" "highcharts-more.js")
add_javascript_dependency("https://code.highcharts.com/modules/" "exporting.js")
add_javascript_dependency("https://code.highcharts.com/modules/" "offline-exporting.js")
add_javascript_dependency("https://highcharts.github.io/export-csv/" "export-csv.js")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/javascript_includes.h.in ${CMAKE_CURRENT_BINARY_DIR}/javascript_includes.h @ONLY)

# Define the source files
set(SOURCE_FILES "benchmark.cc" "benchmark_util.cc" "colorprint.cc" "commandlineflags.cc"
                 "console_reporter.cc" "csv_reporter.cc" "html_reporter.cc" "json_reporter.cc"
                 "log.cc" "reporter.cc" "sleep.cc" "string_util.cc"
                 "sysinfo.cc" "walltime.cc")
# Determine the correct regular expression engine to use
if(HAVE_STD_REGEX)
  set(RE_FILES "re_std.cc")
elseif(HAVE_GNU_POSIX_REGEX)
  set(RE_FILES "re_posix.cc")
elseif(HAVE_POSIX_REGEX)
  set(RE_FILES "re_posix.cc")
else()
  message(FATAL_ERROR "Failed to determine the source files for the regular expression backend")
endif()

add_library(benchmark ${SOURCE_FILES} ${RE_FILES})
if (BENCHMARK_DOWNLOAD_JS)
  add_custom_target(download-js ALL DEPENDS ${_js_dependencies})
  add_dependencies(benchmark download-js)
endif ()

set_target_properties(benchmark PROPERTIES
  OUTPUT_NAME "benchmark"
  VERSION ${GENERIC_LIB_VERSION}
  SOVERSION ${GENERIC_LIB_SOVERSION}
  INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}"
)

# Link threads.
target_link_libraries(benchmark ${CMAKE_THREAD_LIBS_INIT})

# We need extra libraries on Windows
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_link_libraries(benchmark Shlwapi)
endif()

# Expose public API
target_include_directories(benchmark PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Install target (will install the library to specified CMAKE_INSTALL_PREFIX variable)
install(
  TARGETS benchmark
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  COMPONENT library)

install(
  DIRECTORY "${PROJECT_SOURCE_DIR}/include/benchmark"
  DESTINATION include
  FILES_MATCHING PATTERN "*.*h")

if(BENCHMARK_DOWNLOAD_JS)
  install(
    DIRECTORY "${PROJECT_BINARY_DIR}/share/benchmark"
    DESTINATION share
    FILES_MATCHING PATTERN "*.js")
endif()
