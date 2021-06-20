################################################################################
#
# net build
#
################################################################################

# nerfnet ######################################################################

add_executable(nerfnet
  nerfnet_main.cc
  nrf_radio_interface.cc
  radio_interface.cc
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
