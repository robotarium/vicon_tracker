#ifndef VRPN_TO_MQTT_CLIENT_H
#define VRPN_TO_MQTT_CLIENT_H

#include <mqttclient.hpp>
#include <vrpn_Tracker.h>
#include <vrpn_Connection.h>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <json.hpp>
#include <math.h>

/* Convenience declaration */
using json = nlohmann::json;

namespace vrpn_to_mqtt_client
{

  using namespace mqtt_client;

  /*
    Struct for holding relevant tracker data.  This is stored in a map and
    passed into the handle_pose callback.
  */
  struct tracker_data_t
  {
    std::chrono::time_point<std::chrono::high_resolution_clock> time_since_last_message; //std::chrono::high_resolution_clock::now()
    std::shared_ptr<std::string> name;
    std::shared_ptr<vrpn_Tracker_Remote> tracker;
    json* message;
  };

  class VrpnToMqttClient
  {
    public:

      typedef std::unordered_map<std::string, std::shared_ptr<tracker_data_t>> TrackerMap;

      /*
        Creates a new connection to a host.  Will register any new clients that
        are not in a banned.
      */
      VrpnToMqttClient(std::string host, std::string port, std::string mqtt_host, std::string mqtt_port, std::string mqtt_channel);
      ~VrpnToMqttClient();

      void main_loop();
      void check_for_new_trackers();
      void publish_mqtt_data();
      void prune_unresponsive_clients();

    private:

      std::shared_ptr<vrpn_Connection> vrpn_connection;
      std::shared_ptr<MQTTClient> mqtt_client;
      std::string full_host_name;
      std::string mqtt_channel;

      // Elapsed time before tracker is removed
      int timeout_millis = 5000;

      json message = {};

      /*
        Tracker map keeps track of the current trackers that are currently
        registered.  The 'banned' set keeps track of the markers that are banned
        to be tracked by the system.
      */
      TrackerMap current_trackers;
      std::unordered_set<std::string> banned;

      /*
        This is a callback that will receive pose / quaternion data.  'user_data' is just context that is passed in when
        the callback is registered.
      */
      static void VRPN_CALLBACK handle_pose(void * user_data, const vrpn_TRACKERCB tracker_data);
  };
}

#endif
