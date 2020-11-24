################################################################################
#
# net build
#
################################################################################

# nerfnet ######################################################################

add_executable(nerfnet
  nerfnet_main.cc
)

target_include_directories(nerfnet PRIVATE
  ${tclap_INCLUDE_DIRS}
)

target_include_directories(nerfnet PUBLIC
  ${PROJECT_SOURCE_DIR}
)

target_link_libraries(nerfnet PUBLIC
  rf24
)

# net proto ####################################################################

PROTOBUF_GENERATE_CPP(net_proto_hdrs
  net_proto_srcs
  net.proto
)

add_library(net-proto
  ${net_proto_hdrs}
  ${net_proto_srcs}
)

target_include_directories(net-proto PUBLIC ${PROJECT_BINARY_DIR})
target_include_directories(net-proto PUBLIC ${Protobuf_INCLUDE_DIRS})
target_link_libraries(net-proto ${Protobuf_LIBRARIES})
