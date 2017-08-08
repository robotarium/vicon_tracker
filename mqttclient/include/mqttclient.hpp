#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <unordered_map>
#include <string>
#include <mosquitto.h>
#include <thread>
#include <tsqueue.hpp>
#include <promise.hpp>
#include <iostream>
#include <functional>

namespace mqtt_client
{

  class MQTTClient;

  extern MQTTClient* global_client;

  class MQTTClient {

  private:

    class DataTuple {
    public:
      std::string topic;
      std::string message;
      DataTuple(std::string topic, std::string message) {
        this->topic = topic;
        this->message = message;
      }
    };

    std::string host;
    ThreadSafeQueue<std::shared_ptr<DataTuple>> q;
    std::shared_ptr<std::thread> runner;
    std::unordered_map<std::string, std::function<void(std::string, std::string)>> subscriptions;
    int port;
    struct mosquitto *mosq;

  public:

    MQTTClient(std::string host, int port);
    ~MQTTClient();

    void start();

    void publish_loop(void);

    //TODO: Implement subscribe method!
    void subscribe(std::string topic, std::function<void(std::string, std::string)> f);

    void unsubscribe(std::string topic);

    void async_publish(std::string topic, std::string message);

  private:

    // // Callback for handling incoming MQTT messages
    static void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);

  };

}

#endif
