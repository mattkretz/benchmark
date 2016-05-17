# - Compile and run code to check for C++ features
#
# This functions compiles a source file under the `cmake` folder
# and adds the corresponding `HAVE_[FILENAME]` flag to the CMake
# environment
#
#  cxx_feature_check(<FLAG> [<VARIANT>])
#
# - Example
#
# include(CXXFeatureCheck)
# cxx_feature_check(STD_REGEX)
# Requires CMake 2.6+

if(__cxx_feature_check)
  return()
endif()
set(__cxx_feature_check INCLUDED)

function(cxx_feature_check FILE)
  string(TOLOWER ${FILE} FILE)
  string(TOUPPER ${FILE} VAR)
  set(FEATURE "HAVE_${VAR}")
  if(DEFINED ${FEATURE})
    if(${FEATURE})
      add_definitions(-D${FEATURE})
    endif()
    return()
  endif()
  message(STATUS "Performing Test ${FEATURE}")
  try_run(RUN_${FEATURE} COMPILE_${FEATURE}
          ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${FILE}.cpp)
  if(RUN_${FEATURE} EQUAL 0)
    message(STATUS "Performing Test ${FEATURE} -- success")
    set(${FEATURE} ON)
    add_definitions(-D${FEATURE})
  else()
    set(${FEATURE} OFF)
    if(NOT COMPILE_${FEATURE})
      message(STATUS "Performing Test ${FEATURE} -- failed to compile")
    else()
      message(STATUS "Performing Test ${FEATURE} -- compiled but failed to run")
    endif()
  endif()
  set(${FEATURE} ${${FEATURE}} CACHE BOOL "Whether the ${FILE} feature is supported.")
  mark_as_advanced(${FEATURE})
endfunction()

