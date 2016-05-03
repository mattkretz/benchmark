# Allow the source files to find headers in src/
include_directories(${PROJECT_SOURCE_DIR}/src)

# Set JavaScript paths for the HTML reporter
set(JQUERY_NAME "jquery-2.2.1.min.js")
set(HIGHSTOCK_NAME "highstock.js")
set(HIGHSTOCK_MORE_NAME "highcharts-more.js")
set(JQUERY_URL "https://code.jquery.com/${JQUERY_NAME}")
set(HIGHSTOCK_URL "http://code.highcharts.com/stock/${HIGHSTOCK_NAME}")
set(HIGHSTOCK_MORE_URL "http://code.highcharts.com/stock/${HIGHSTOCK_MORE_NAME}")
if (BENCHMARK_DOWNLOAD_JS)
  set(_dependencies)
  macro(download filename url)
    add_custom_command(OUTPUT "${PROJECT_BINARY_DIR}/include/benchmark/${filename}"
      COMMAND ${CMAKE_COMMAND} "-Dfilename=${filename}" "-Durl=${url}" -P ${PROJECT_SOURCE_DIR}/cmake/download.cmake
      DEPENDS ${PROJECT_SOURCE_DIR}/cmake/download.cmake
      WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/include/benchmark"
      COMMENT "Downloading ${url}"
      VERBATIM)
   set(_dependencies ${_dependencies} "${PROJECT_BINARY_DIR}/include/benchmark/${filename}")
  endmacro()
  download(${JQUERY_NAME} "${JQUERY_URL}")
  download(${HIGHSTOCK_NAME} "${HIGHSTOCK_URL}")
  download(${HIGHSTOCK_MORE_NAME} "${HIGHSTOCK_MORE_URL}")
  add_custom_target(download-js ALL DEPENDS ${_dependencies})

  set(CMAKE_JQUERY_PATH "${CMAKE_INSTALL_PREFIX}/share/benchmark/${JQUERY_NAME}")
  set(CMAKE_HIGHSTOCK_PATH "${CMAKE_INSTALL_PREFIX}/share/benchmark/${HIGHSTOCK_NAME}")
  set(CMAKE_HIGHSTOCK_MORE_PATH "${CMAKE_INSTALL_PREFIX}/share/benchmark/${HIGHSTOCK_MORE_NAME}")

  set(CMAKE_JAVASCRIPT_REPLACEMENT
    "<script type = \\\"text/javascript\\\">\${JQUERY}</script>\
     <script type = \\\"text/javascript\\\">\${HIGHCHART}</script>\
     <script type = \\\"text/javascript\\\">\${HIGHCHART_MORE}</script>"
    )
else ()
  set(CMAKE_JQUERY_PATH "")
  set(CMAKE_HIGHSTOCK_PATH "")
  set(CMAKE_HIGHSTOCK_MORE_PATH "")
  set(CMAKE_JAVASCRIPT_REPLACEMENT
    "<script type = \\\"text/javascript\\\" src = \\\"${JQUERY_URL}\\\"></script>\
     <script type = \\\"text/javascript\\\" src = \\\"${HIGHSTOCK_URL}\\\"></script>\
     <script type = \\\"text/javascript\\\" src = \\\"${HIGHSTOCK_MORE_URL}\\\"></script>"
    )
endif ()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/html_reporter.cc ${CMAKE_CURRENT_BINARY_DIR}/html_reporter.cc @ONLY)

# Define the source files
set(SOURCE_FILES "benchmark.cc" "benchmark_util.cc" "colorprint.cc" "commandlineflags.cc"
                 "console_reporter.cc" "csv_reporter.cc" "${CMAKE_CURRENT_BINARY_DIR}/html_reporter.cc" "json_reporter.cc"
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
  add_dependencies(benchmark download-js)
endif ()

set_target_properties(benchmark PROPERTIES
  OUTPUT_NAME "benchmark"
  VERSION ${GENERIC_LIB_VERSION}
  SOVERSION ${GENERIC_LIB_SOVERSION}
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
    DIRECTORY "${PROJECT_BINARY_DIR}/include/benchmark"
    DESTINATION "share"
    FILES_MATCHING PATTERN "*.js")
endif()
