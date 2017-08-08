#include "vrpn_to_mqtt_client.h"
#include <iostream>
#include <chrono>
#include <functional>
#include <mutex>

namespace vrpn_to_mqtt_client
{
  /*
    Constructor.  Make sure that we attempt to connection to the VRPN server here.
  */
  VrpnToMqttClient::VrpnToMqttClient(std::string host, std::string port, std::string mqtt_host, std::string mqtt_port, std::string mqtt_channel)
  {
      this->mqtt_channel = mqtt_channel;
      full_host_name = host + ":" + port;

      /*
        These are the banned names.  Eventually, I should lift this part
        out of the function.
      */
      banned.insert(std::string("VRPN Control"));

      std::cout << "Connecting to server at: " << full_host_name << std::endl;
      vrpn_connection = std::shared_ptr<vrpn_Connection>(vrpn_get_connection_by_name(full_host_name.c_str()));

      /*
        Make sure that we're connected to the server.
      */
      if(!vrpn_connection->connected())
      {
        std::cout << "Couldn't connect to server!" << std::endl;
        assert(vrpn_connection->connected());
      }

      std::cout << "Connected to server!" << std::endl;

      this->mqtt_client = std::make_shared<mqtt_client::MQTTClient>(mqtt_host, std::stoi(mqtt_port));
      this->mqtt_client->start();
  }

  /*
    Destructor.  Not much to do here.  Just loop through all the trackers and
    "unregister" the change handles (i.e., callbacks).
  */
  VrpnToMqttClient::~VrpnToMqttClient()
  {
    for (VrpnToMqttClient::TrackerMap::iterator it = current_trackers.begin(); it != current_trackers.end(); ++it)
    {
      it->second->tracker->unregister_change_handler(this, &VrpnToMqttClient::handle_pose);
    }
  }

  void VrpnToMqttClient::main_loop()
  {

    /*
      Call the VRPN main loop and check if the connection is okay.
    */
    vrpn_connection->mainloop();

    if(!vrpn_connection->doing_okay())
    {
      std::cout << "Warning: VRPN connection unstable!" << std::endl;
    }

    /*
      Make sure that the connection is still connected.
    */
    if(!vrpn_connection->connected())
    {
      std::cout << "Error: VRPN connection disconnected!" << std::endl;
    }

    /*
      Update all currently registered trackers.
    */
    for (VrpnToMqttClient::TrackerMap::iterator it = current_trackers.begin(); it != current_trackers.end(); ++it)
    {
      it->second->tracker->mainloop();
    }
  }

  void VrpnToMqttClient::prune_unresponsive_clients()
  {
    // Get the current time
    auto current_time = std::chrono::high_resolution_clock::now();
    // std::vector<std::string> to_remove;

    for (VrpnToMqttClient::TrackerMap::iterator it = current_trackers.begin(); it != current_trackers.end(); ++it)
    {
      auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - it->second->time_since_last_message);

      if(time_elapsed.count() > timeout_millis)
      {
        //If tracker has timed out, remove it from the message
        //TODO: Make this cleaner.
        // to_remove.push_back(it->first);
        // banned.insert(it->first); // Make sure that we can't retrack these

        // Basically,1 don't send data if we're not tracking this anymore
        if(message.count(it->first) != 0) // Only erase if it's actually in the message
        {
          std::cout << "Vicon tracker no longer getting data for: " << it->first << std::endl;
          message.erase(it->first);
        }
      }
    }

    // // Make sure that these timed-out trackers are removed from current trackers.
    // for(auto it = to_remove.begin(); it != to_remove.end(); ++it)
    // {
    //   current_trackers.erase(*it);
    // }
  }

  void VrpnToMqttClient::check_for_new_trackers()
  {
    int i = 0;

    /*
      The purpose of this block is to check for new trackers and register them.  If the new tracker is not in the
      list of current trackers or in the banned, then we add it to the trackers that are currently
      being tracked.
    */
    while(vrpn_connection->sender_name(i) != NULL)
    {
      if(current_trackers.count(vrpn_connection->sender_name(i)) == 0 && banned.count(vrpn_connection->sender_name(i)) == 0)
      {
        std::cout << "Found a new tracker: " << vrpn_connection->sender_name(i) << std::endl;

        // Make shared and register the change handler for the pose
        std::shared_ptr<tracker_data_t> data = std::make_shared<tracker_data_t>();
        data->name = std::make_shared<std::string>(vrpn_connection->sender_name(i));
        data->tracker = std::make_shared<vrpn_Tracker_Remote>(data->name->c_str(), vrpn_connection.get());
        data->time_since_last_message = std::chrono::high_resolution_clock::now();
        data->tracker->shutup = true; // Turn off messages from tracker
        data->message = &this->message;
        data->message_mutex = &this->message_mutex;

        this->message_mutex.lock();
        this->message[*data->name]["x"] = -1;
        this->message[*data->name]["y"] = -1;
        this->message[*data->name]["z"] = -1;
        this->message[*data->name]["theta"] = -1;
        this->message[*data->name]["powerData"] = -1;
        this->message[*data->name]["charging"] = -1;
        this->message_mutex.unlock();
        //TODO: Should probably provide 'this' as context.  I have no idea why unregister_change_handler works
        // here...
        data->tracker.get()->register_change_handler(data.get(), &VrpnToMqttClient::handle_pose);

        // Put the new name/tracker pair into the map
        current_trackers.insert(std::make_pair(
          vrpn_connection->sender_name(i),
          data
        ));
      }

      ++i;
    }
  }

  void VrpnToMqttClient::publish_mqtt_data()
  {
    //std::cout << message.dump(4) << std::endl;
    mqtt_client->async_publish(mqtt_channel, message.dump());
  }

  /*
    Will be the method through which all messages are handled.  Basically,
    this method just needs to aggregate
  */
  void VRPN_CALLBACK VrpnToMqttClient::handle_pose(void * user_data, const vrpn_TRACKERCB tracker_data)
  {
    tracker_data_t* data = static_cast<tracker_data_t*>(user_data);

    // TODO: Add timestamp to message

    (data->time_since_last_message) = std::chrono::high_resolution_clock::now();

    data->message_mutex->lock();
    (*data->message)[data->name->c_str()]["x"] = tracker_data.pos[0]*1;
    (*data->message)[data->name->c_str()]["y"] = tracker_data.pos[1];
    (*data->message)[data->name->c_str()]["z"] = tracker_data.pos[2]*1;

    double qx = tracker_data.quat[0]*1;
    double qy = tracker_data.quat[1];
    double qz = tracker_data.quat[2]*1;
    double qw = tracker_data.quat[3];

    (*data->message)[data->name->c_str()]["theta"] = std::atan2(2.0f*(qw*qz + qx*qy), 1.0f - 2.0f*(qy*qy + qz*qz));

    // (*data->message)[data->name->c_str()]["theta"] += M_PI;
    // (*data->message)[data->name->c_str()]["theta"] = std::atan2(std::sin((*data->message)[data->name->c_str()]["theta"]), std::cos((*data->message)[data->name->c_str()]["theta"]))

    std::cout << *data->message << std::endl;
    data->message_mutex->unlock();
  }
}
