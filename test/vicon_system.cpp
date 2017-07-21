#include <vrpn_to_mqtt_client.h>
#include <iostream>
#include <functional>
#include <chrono>
#include <future>
/*
  Main file that will handle connecting to the Vicon tracking system.
*/
int main(void) {

  vrpn_to_mqtt_client::VrpnToMqttClient vrpn_to_mqtt(
    std::string("192.168.10.1"), std::to_string(3883), // Vicon address
    std::string("192.168.1.2"), std::to_string(1884), // MQTT address
    std::string("overhead_tracker/all_robot_pose_data"));

  /*
    TODO: Change system to accept publishing time and accepted trackers as CLI
    arguments!
  */

  auto current_time = std::chrono::high_resolution_clock::now();
  auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - current_time);

  int check_every = 33;

  while(true) {
    current_time = std::chrono::high_resolution_clock::now();
    vrpn_to_mqtt.main_loop();
    vrpn_to_mqtt.check_for_new_trackers();
    vrpn_to_mqtt.prune_unresponsive_clients();
    vrpn_to_mqtt.publish_mqtt_data();
    elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - current_time);

    std::this_thread::sleep_for(std::chrono::milliseconds(check_every - elapsed_time.count()));
  }

  return 0;
}