"""
This script acts as a station between the LoRa router hub and the dashboard. Because the dashboard cannot interpret
the data sent by the LoRa router hub, this script is developed to interpret the data sent by the LoRa router hub
and prepare the data in the format that the dashboard can understand.
"""

import paho.mqtt.client as mqtt
import json
import base64
import random

# Data format used by dashboard
data = {
            "temp_West": 0,
            "humidity_West": 0,
            "pressure_West": 0,
            "rain_level": 0,
            "wind_direction1": "Not initiated",
            "wind_speed1": 0,
          }


def on_connect(client, userdata, flags, rc):
  """
  This function is used to listen the channel "topic/test" of the LoRa router hub
  to receive the data sent by the sensors
  :param client: the MQTT client object
  :param userdata: Not being used currently
  :param flags: Not being used currently
  :param rc: the connection status code
  """
  print("Connected with result code "+str(rc))
  client.subscribe("topic/test")

def on_message(client, userdata, msg):
  """
  This function runs the custom_decoder() function when there is a message sent by
  the LoRa router hub
  :param client: the MQTT client object
  :param userdata: Not being used currently
  :param msg: the data sent by the LoRa router hub
  """
  if msg.payload.decode():
    decoded_msg = msg.payload.decode()
    # print(decoded_msg) # DEBUG line, print out the decoded message
    custom_decoder(decoded_msg)

def custom_decoder(message):
  """
  This function receives the data sent by the LoRa router hub, decodes the data
  using the function prepare_publish_data() to construct the sensor data into the format used
  by the dashboard. Then, this function sends the constructed data through the channel "topic/MQTT2DashBoard"
  to the dashboard.
  :param message: the received data sent by the LoRa router hub
  """
  message_json = json.loads(message);
  device_name = message_json['deviceName']
  message_data = message_json['data']
  # Decode the base64
  base64_bytes = message_data.encode('ascii')
  message_bytes = base64.b64decode(base64_bytes)
  message_array = message_bytes.decode('ascii').split("::")
  # print(message_array, device_name)
  publishing_data = prepare_publish_data(device_name, message_array)
  # print(publishing_data) # DEBUG line, print out the prepared data
  publish_data("topic/MQTT2DashBoard", publishing_data)
  
def prepare_publish_data(device_name, data_array):
  """
  This function interprets different data based on device name and
  construct the data into the format used by dashboard.
  :param device_name: the name of the device sending the data
  :param data_array: the data sent by the sensors through the LoRa router hub
  :return:
    data: the constructed data in the format used by the dashboard
  """
  if device_name == "arduino_ABP":
    data["temp_West"] = round((float(data_array[1]) + float(data_array[2])) / 2, 1)
    data["humidity_West"] = float(data_array[0])
    data["pressure_West"] = float(data_array[3])
    data["rain_level"] = data_array[4]
  elif device_name == "ABP-2":
    data["wind_direction1"] = data_array[0]
    data["wind_speed1"] = data_array[1]
  return data

def publish_data(topic_name, data):
  """
  This function publishes the data to a channel in a MQTT server
  :param topic_name: the name of the channel in the MQTT server
  :param data: the data to be published
  """
  client.publish(topic_name, json.dumps(data)); # convert dictionary to string

if __name__ == "__main__":
  # Create a MQTT client and connect to the server
  client = mqtt.Client()
  client.connect("localhost",1883,60)

  # Set up the on connect event and on message event
  client.on_connect = on_connect
  client.on_message = on_message

  # Continuously listen and send data to the server
  client.loop_forever()
