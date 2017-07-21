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

// class MQTTClient;
//
// MQTTClient* global_client;

namespace mqtt_client
{

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

    //static std::unordered_map<std::string, std::function<void(std::string, std::string)>> subscriptions;
    std::unordered_map<std::string, std::function<void(std::string, std::string)>> subscriptions;
    std::function<void (struct mosquitto*, void*, const struct mosquitto_message*)> mosquitto_callback;

    int port;
    struct mosquitto *mosq;

  public:

    MQTTClient(std::string host, int port) {

      ThreadSafeQueue<std::shared_ptr<DataTuple>> q();

      // // TODO: Fix worst singleton pattern ever
      // if(global_client == NULL) {
      //   global_client = this;
      // }

      mosquitto_callback = generate_message_callback(this);

      this->host = host;
      this->port = port;

      int keepalive = 20;
      bool clean_session = true;

      mosquitto_lib_init();

      mosq = mosquitto_new("overhead_tracker", clean_session, NULL);

      if(!mosq){
        std::cerr << "Error: Couldn't allocate memory" << std::endl;
        throw 1;
      }

      if(mosquitto_connect_bind(mosq, host.c_str(), port, keepalive, NULL)) {
        std::cerr << "Unable to connect to MQTT broker at " << host << ":" << port << std::endl;
        throw 2;
      }

      //Set message callback and start the loop!
      mosquitto_message_callback_set(mosq, reinterpret_cast<void (*)(struct mosquitto*, void*, const struct mosquitto_message*)>(&mosquitto_callback));
    	mosquitto_loop_start(mosq);
    }

    ~MQTTClient(void) {
      q.enqueue(NULL);
      runner->join();
      runner.reset();
      mosquitto_destroy(mosq);
      mosquitto_lib_cleanup();
    }

    void start() {
      runner = std::make_shared<std::thread>(&MQTTClient::publish_loop, this);
    }

    void publish_loop(void) {
      while(true) {
        std::shared_ptr<DataTuple> message = q.dequeue();
        if(message == NULL) {
          std::cout << "Stopping MQTT client publisher..." << std::endl;
          break;
        }
        //TODO: remove std::cout << "Queue size: " << q.q.size() << std::endl;
        mosquitto_publish(mosq, NULL, message->topic.c_str() , message->message.length(), message->message.c_str(), 1, false);
      }
    }

    //TODO: Implement subscribe method!
    void subscribe(std::string topic, std::function<void(std::string, std::string)> f) {
      //Struct, ?, topic string, QOS
      subscriptions[topic] = f;
      mosquitto_subscribe(mosq, NULL, topic.c_str(), 1);
    }

    void unsubscribe(std::string topic) {
      mosquitto_unsubscribe(mosq, NULL, topic.c_str());
      subscriptions.erase(topic.c_str());
    }

    void async_publish(std::string topic, std::string message) {
      std::shared_ptr<DataTuple> msg = std::make_shared<DataTuple>(topic, message);
      q.enqueue(msg);
    }

  private:

    // Callback for handling incoming MQTT messages
    static std::function<void (struct mosquitto*, void*, const struct mosquitto_message*)> generate_message_callback(MQTTClient* client) {

      return [&client](struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
      {
        if(client->subscriptions.find(message->topic) != client->subscriptions.end()) {
          client->subscriptions[message->topic](std::string((char*) message->topic), std::string((char*) message->payload));
        }
      };
    }
    // static std::function<void (struct mosquitto*, void*, const struct mosquitto_message*) generate_message_callback(MQTTClient* client, struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
    //
    //   if(client->subscriptions.find(message->topic) != client->subscriptions.end()) {
    //     client->subscriptions[message->topic](std::string((char*) message->topic), std::string((char*) message->payload));
    //   }
    // }

  };

}

#endif
