################################################################################
#
# net build
#
################################################################################

# nerfnet ######################################################################

add_executable(nerfnet
  nerfnet_main.cc
  nrf_link.cc
)

target_include_directories(nerfnet PRIVATE
  ${tclap_INCLUDE_DIRS}
)

target_link_libraries(nerfnet PUBLIC
  link
  pthread
  radio_transport
  rf24
  util_string
  util_time
)

# nerfnet proto ##############################################################

PROTOBUF_GENERATE_CPP(nerfnet_proto_hdrs
  nerfnet_proto_srcs
  nerfnet.proto
)

add_library(nerfnet_proto
  ${nerfnet_proto_hdrs}
  ${nerfnet_proto_srcs}
)

target_include_directories(nerfnet_proto PUBLIC ${PROJECT_BINARY_DIR})
target_include_directories(nerfnet_proto PUBLIC ${Protobuf_INCLUDE_DIRS})
target_link_libraries(nerfnet_proto ${Protobuf_LIBRARIES})

# link #########################################################################

add_library(link
  link.cc
)

target_link_libraries(link PUBLIC
  util_time
)

# transport ####################################################################

add_library(transport
  transport.cc
)

# mock link ####################################################################

add_library(mock_link
  mock_link.cc
)

target_link_libraries(mock_link PUBLIC
  gtest
  link
  pthread
  util_time
)

# radio transport ##############################################################

add_library(radio_transport
  radio_transport.cc
)

target_link_libraries(radio_transport PUBLIC
  transport
  radio_transport_receiver
  util_crc16
  util_time
)

add_executable(radio_transport_test
  radio_transport_test.cc
)

target_link_libraries(radio_transport_test PUBLIC
  gtest_main
  gtest
  mock_link
  radio_transport
  util_encode_decode
)

gtest_discover_tests(radio_transport_test)

# radio transport receiver #####################################################

add_library(radio_transport_receiver
  radio_transport_receiver.cc
)

target_link_libraries(radio_transport_receiver PUBLIC
  link
  util_time
)

add_executable(radio_transport_receiver_test
  radio_transport_receiver_test.cc
)

target_link_libraries(radio_transport_receiver_test PUBLIC
  gtest_main
  gtest
  pthread
  radio_transport_receiver
  util_time
)

gtest_discover_tests(radio_transport_receiver_test)
