import json, paho.mqtt.client as mqtt

def on_message(client, userdata, msg):
    data = json.loads(msg.payload)
    print(f"Received: {data}")
    if data["temperature"] > 30:
        client.publish("esp32/control", "ON")
    else:
        client.publish("esp32/control", "OFF")

client = mqtt.Client()
client.connect("broker.mqtt.cool", 1883, 60)
client.subscribe("esp32/data")
client.on_message = on_message
client.loop_forever()
