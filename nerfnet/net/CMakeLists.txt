################################################################################
#
# net build
#
################################################################################

# nerfnet ######################################################################

add_executable(nerfnet
  nerfnet_main.cc
  radio_interface.cc
  primary_radio_interface.cc
  secondary_radio_interface.cc
)

target_include_directories(nerfnet PRIVATE
  ${tclap_INCLUDE_DIRS}
)

target_include_directories(nerfnet PUBLIC
  ${PROJECT_SOURCE_DIR}
)

target_link_libraries(nerfnet PUBLIC
  pthread
  rf24
  util
)

# nerfnet_v2 ###################################################################

add_executable(nerfnet_v2
  nerfnet_v2_main.cc
  nrf_radio_interface.cc
  radio_interface_v2.cc
)

target_include_directories(nerfnet_v2 PRIVATE
  ${tclap_INCLUDE_DIRS}
)

target_include_directories(nerfnet_v2 PUBLIC
  ${PROJECT_SOURCE_DIR}
)

target_link_libraries(nerfnet_v2 PUBLIC
  pthread
  rf24
  util
)
