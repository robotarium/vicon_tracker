#include "mqttclient.hpp"
#include <unordered_map>
#include <string>
#include <mosquitto.h>
#include <thread>
#include <tsqueue.hpp>
#include <iostream>
#include <functional>


namespace mqtt_client
{

  MQTTClient* global_client;

  MQTTClient::MQTTClient(std::string host, int port)
  {

    ThreadSafeQueue<std::shared_ptr<DataTuple>> q();

    // TODO: Fix worst singleton pattern ever
    if(global_client == NULL) {
      global_client = this;
    }

    this->host = host;
    this->port = port;

    int keepalive = 20;
    bool clean_session = true;

    mosquitto_lib_init();

    // Make sure we add the time on to ensure that this client is unique.
    std::string client_name = "overhead_tracker" + std::to_string(std::time(0));
    mosq = mosquitto_new(client_name.c_str(), clean_session, NULL);

    if(!mosq){
      std::cerr << "Error: Couldn't allocate memory" << std::endl;
      throw 1;
    }

    if(mosquitto_connect_bind(mosq, host.c_str(), port, keepalive, NULL)) {
      std::cerr << "Unable to connect to MQTT broker at " << host << ":" << port << std::endl;
      throw 2;
    }

    //Set message callback and start the loop!
    mosquitto_message_callback_set(mosq, &MQTTClient::message_callback);
    int loop = mosquitto_loop_start(mosq);
    if(loop != MOSQ_ERR_SUCCESS)
    {
      fprintf(stderr, "Unable to start loop: %i\n", loop);
      exit(1);
    }
  }

  MQTTClient::~MQTTClient()
  {
    q.enqueue(NULL);
    runner->join();
    runner.reset();
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
  }

  void MQTTClient::start()
  {
    runner = std::make_shared<std::thread>(&MQTTClient::publish_loop, this);
  }

  void MQTTClient::publish_loop(void)
  {
    while(true) {
      std::shared_ptr<DataTuple> message = q.dequeue();
      if(message == NULL) {
        std::cout << "Stopping MQTT client publisher..." << std::endl;
        break;
      }
      mosquitto_publish(mosq, NULL, message->topic.c_str() , message->message.length(), message->message.c_str(), 1, false);
    }
  }

  //TODO: Implement subscribe method!
  void MQTTClient::subscribe(std::string topic, std::function<void(std::string, std::string)> f)
  {
    //Struct, ?, topic string, QOS
    global_client->subscriptions[topic] = f;
    mosquitto_subscribe(mosq, NULL, topic.c_str(), 1);
  }

  void MQTTClient::unsubscribe(std::string topic)
  {
    mosquitto_unsubscribe(mosq, NULL, topic.c_str());
    global_client->subscriptions.erase(topic.c_str());
  }

  void MQTTClient::async_publish(std::string topic, std::string message)
  {
    std::shared_ptr<DataTuple> msg = std::make_shared<DataTuple>(topic, message);
    q.enqueue(msg);
  }

  // Callback for handling incoming MQTT messages
  void MQTTClient::message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
  {
    if(global_client->subscriptions.find(message->topic) != global_client->subscriptions.end()) {
      global_client->subscriptions[message->topic](std::string((char*) message->topic), std::string((char*) message->payload));
    }
  }
}
