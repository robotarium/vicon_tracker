import paho.mqtt.client as mqtt
import time
import json

mqttc = mqtt.Client();

mqttc.connect("143.215.159.23", port=1884)
mqttc.loop_start()

count = 0
start = time.time()

def on_message(client, userdata, message):
  global count
  count += 1
  print(count/(time.time() - start))
  msg = json.loads(message.payload.decode(encoding="UTF-8"), encoding="UTF-8")
  print(msg['105'])

mqttc.on_message = on_message
mqttc.subscribe("overhead_tracker/all_robot_pose_data")

while True:
    pass
