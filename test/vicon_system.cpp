#include <vrpn_to_mqtt_client.h>
#include <iostream>
#include <functional>
#include <chrono>
#include <future>

/*
GLOBALS
*/

vrpn_to_mqtt_client::VrpnToMqttClient* vrpn_to_mqtt;

/* -------------------------------------
 *		MQTT Callback functions
 * ------------------------------------- */
void power_data_callback(std::string topic, std::string message) {
  bool debug = false;

  /* Parse ID from topic
   * topic names are of the form ID/power_data
   */
  int id = -1;
  std::string tmp;
  std::stringstream ss(topic);
  ss >> id >> tmp;

  // Should auto-unlock when this goes out of scope
  std::lock_guard<std::mutex> lock(vrpn_to_mqtt->message_mutex);

  //std::cout << vrpn_to_mqtt->message << std::endl;

  // Skip out if we're not tracking this robot
  if(vrpn_to_mqtt->message.count(std::to_string(id)) == 0)
  {
    return;
  }

	if(debug) {
		std::cout << "Topic				: " << topic << std::endl;
		std::cout << "Message			: " << message << std::endl;
		std::cout << "Extracted ID: " << id << std::endl;
	}

  /* Parse message string into JSON dictionary */
  try {
	auto data = json::parse(message);

	/* Write battery voltage to global powerData JSON dictionary */
	/* Check if an entry vBat exists in the message */
	if (data.find("vBat") != data.end()) {
			if(debug) {
			     std::cout << "Power data in callback: " << data["vBat"] << std::endl;
			}

			/* Check if robot id has been extracted from topic */
			if(id >= 0) {
			     vrpn_to_mqtt->message[std::to_string(id)]["powerData"] = data["vBat"];
			}
	}

  /* Write charging status to global powerData JSON di-ctionary */
  /* Check if an entry vBat exists in the message */
  if (data.find("charging") != data.end()) {
      if(debug) {
           std::cout << "Power data in callback: " << data["charging"] << std::endl;
      }

      /* Check if robot id has been extracted from topic */
      if(id >= 0) {
        vrpn_to_mqtt->message[std::to_string(id)]["charging"] = data["charging"]; //((float)  > 0.0);
      }
  }
  } catch (const std::exception& e) {
    std::cout << e.what() << "Exception..." << std::endl;
  }
}

/*
  Main file that will handle connecting to the Vicon tracking system.
*/
int main(int argc, char**argv) {

  // Assume that arguments are mqtt host, port then refresh rate in MS
  std::string mqtt_host = "";
  std::string mqtt_port = "";
  int check_every = 33;

  mqtt_host = std::string(argv[1]);
  mqtt_port = std::string(argv[2]);
  check_every = atoi(argv[3]);

  std::cout << "Connecting to MQTT host: " << mqtt_host << ":" << mqtt_port << " at refresh rate (ms): " << check_every << std::endl;

  vrpn_to_mqtt = new vrpn_to_mqtt_client::VrpnToMqttClient(
    std::string("192.168.10.1"), std::to_string(3883), // Vicon address
    mqtt_host, mqtt_port, // MQTT address
    std::string("overhead_tracker/all_robot_pose_data"));

  // std::function<void(std::string, std::string)> stdf_callback = &power_data_callback;
  // for (int i = 100; i <= 200; ++i)
  // {
  //   vrpn_to_mqtt->mqtt_client->subscribe(std::to_string(i) + "/power_data", stdf_callback);
  // }

  /*
    TODO: Change system to accept publishing time and accepted trackers as CLI
    arguments!
  */

  auto current_time = std::chrono::high_resolution_clock::now();
  auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - current_time);

  while(true) {
    current_time = std::chrono::high_resolution_clock::now();
    vrpn_to_mqtt->main_loop();
    vrpn_to_mqtt->check_for_new_trackers();
    vrpn_to_mqtt->prune_unresponsive_clients();
    vrpn_to_mqtt->publish_mqtt_data();
    elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - current_time);

    std::cout << (check_every - elapsed_time.count()) << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(check_every - elapsed_time.count()));
  }

  return 0;
}
