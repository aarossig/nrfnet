################################################################################
#
# net build
#
################################################################################

# nerfnet ######################################################################

add_executable(nerfnet
  link.cc
  nerfnet_main.cc
  nrf_link.cc
)

target_include_directories(nerfnet PRIVATE
  ${tclap_INCLUDE_DIRS}
)

target_link_libraries(nerfnet PUBLIC
  pthread
  radio_transport
  rf24
  util
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

# transport ####################################################################

add_library(transport
  transport.cc
)

target_link_libraries(transport
  nerfnet_proto
)

# radio transport ##############################################################

add_library(radio_transport
  radio_transport.cc
)

target_link_libraries(radio_transport PUBLIC
  transport
  util
)

add_executable(radio_transport_test
  radio_transport_test.cc
)

target_link_libraries(radio_transport_test PUBLIC
  gtest_main
  gtest
  radio_transport
)

gtest_discover_tests(radio_transport_test)
