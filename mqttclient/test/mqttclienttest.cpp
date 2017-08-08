#include <mqttclient.hpp>
#include <iostream>


void callback(std::string topic, std::string message) {
  std::cout << topic << ": " << message << std::endl;
}

//std::function<void(std::string)> stdf_callback = &callback;

std::function<void(std::string, std::string)> stdf_callback = &callback;

int main(void) {

	//MQTTClient m("localhost", 1884);
  mqtt_client::MQTTClient m("143.215.159.23", 1884);
  //MQTTClient m("192.168.1.32", 1883);
  m.subscribe("overhead_tracker/all_robot_pose_data", stdf_callback);
  m.start();

  std::cin.ignore();

  return 0;
}
