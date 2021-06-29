################################################################################
#
# util build
#
################################################################################

# crc16 ########################################################################

add_library(util_crc16
  crc16.cc
)

add_executable(crc16_test
  crc16_test.cc
)

target_link_libraries(crc16_test PUBLIC
  gtest_main
  gtest
  pthread
  util_crc16
)

gtest_discover_tests(crc16_test WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# encode decode ################################################################

add_library(util_encode_decode
  encode_decode.cc
)

add_executable(encode_decode_test
  encode_decode_test.cc
)

target_link_libraries(encode_decode_test PUBLIC
  gtest_main
  gtest
  pthread
  util_encode_decode
)

gtest_discover_tests(encode_decode_test WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# file #########################################################################

add_library(util_file
  file.cc
)

target_link_libraries(util_file PUBLIC
  ${PROTOBUF_LIBRARY}
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

# time #########################################################################

add_library(util_time
  time.cc
)

add_executable(time_test
  time_test.cc
)

target_link_libraries(time_test PUBLIC
  gtest_main
  gtest
  pthread
  util_time
)

gtest_discover_tests(time_test WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# string #######################################################################

add_library(util_string
  string.cc
)
