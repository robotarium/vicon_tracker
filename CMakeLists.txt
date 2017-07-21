cmake_minimum_required(VERSION 2.8.3)
project(vrpn_client_mqtt)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Make sure that we compile with C++11 or higher.
set_directory_properties(PROPERTIES COMPILE_OPTIONS "-std=c++11;-Wall")

set(MQTT_CLIENT_DIR "mqttclient/src/;mqttclient/tsqueue/;mqttclient/promise/")

include_directories(
  include
  ${MQTT_CLIENT_DIR}
)

add_library(vrpn_client_mqtt
  src/vrpn_to_mqtt_client.cpp
)

target_link_libraries(vrpn_client_mqtt
  quat
  vrpn
  pthread
  mosquitto
)

add_executable(vicon_system test/vicon_system.cpp)
add_dependencies(vicon_system vrpn_client_mqtt)
target_link_libraries(vicon_system vrpn_client_mqtt vrpn quat)
