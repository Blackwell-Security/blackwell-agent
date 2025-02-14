# Find the blackwell shared library
find_library(BLACKWELLEXT NAMES libblackwellext.dylib HINTS "${SRC_FOLDER}")
if(BLACKWELLEXT)
  set(uname "Darwin")
else()
  set(uname "Linux")
endif()
find_library(BLACKWELLEXT NAMES libblackwellext.so HINTS "${SRC_FOLDER}")

if(NOT BLACKWELLEXT)
    message(FATAL_ERROR "libblackwellext not found! Aborting...")
endif()

# # Add compiling flags and set tests dependencies
if(${uname} STREQUAL "Darwin")
    set(TEST_DEPS ${BLACKWELLLIB} ${BLACKWELLEXT} -lpthread -ldl -fprofile-arcs -ftest-coverage)
    add_compile_options(-ggdb -O0 -g -coverage -DTEST_AGENT -I/usr/local/include -DENABLE_SYSC -DBLACKWELL_UNIT_TESTING)
else()
    add_compile_options(-ggdb -O0 -g -coverage -DTEST_AGENT -DENABLE_AUDIT -DINOTIFY_ENABLED -fsanitize=address -fsanitize=undefined)
    link_libraries(-fsanitize=address -fsanitize=undefined)
    set(TEST_DEPS ${BLACKWELLLIB} ${BLACKWELLEXT} -lpthread -lcmocka -ldl -fprofile-arcs -ftest-coverage)
endif()

if(NOT ${uname} STREQUAL "Darwin")
  add_subdirectory(client-agent)
  add_subdirectory(logcollector)
  add_subdirectory(os_execd)
endif()

add_subdirectory(blackwell_modules)
