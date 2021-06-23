################################################################################
#
# util build
#
################################################################################

# file #########################################################################

add_library(util_file
  file.cc
)

target_link_libraries(util_file PUBLIC
  ${PROTOBUF_LIBRARY}
  util
)

PROTOBUF_GENERATE_CPP(file_test_proto_hdrs
  file_test_proto_srcs
  file_test.proto
)

add_library(file_test_proto
  ${file_test_proto_hdrs}
  ${file_test_proto_srcs}
)

target_include_directories(file_test_proto PUBLIC ${PROJECT_BINARY_DIR})
target_include_directories(file_test_proto PUBLIC ${Protobuf_INCLUDE_DIRS})
target_link_libraries(file_test_proto ${Protobuf_LIBRARIES})

add_executable(file_test
  file_test.cc
)

target_link_libraries(file_test PUBLIC
  file_test_proto
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
