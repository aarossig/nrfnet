################################################################################
#
# util build
#
################################################################################

# file #########################################################################

add_library(util_file
  file.cc
)

add_executable(file_test
  file_test.cc
)

target_link_libraries(file_test PUBLIC
  gtest_main
  gtest
  pthread
  util_file
)

gtest_discover_tests(file_test WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# util #########################################################################

add_library(util
  string.cc
  time.cc
)
