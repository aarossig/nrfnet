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
  radio_transport.cc
  transport.cc
)

target_include_directories(nerfnet PRIVATE
  ${tclap_INCLUDE_DIRS}
)

target_include_directories(nerfnet PUBLIC
  ${PROJECT_SOURCE_DIR}
)

target_link_libraries(nerfnet PUBLIC
  nerfnet_proto
  pthread
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
