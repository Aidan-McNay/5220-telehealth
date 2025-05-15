#!/usr/bin/env python3

import paho.mqtt.client as mqttClient
import time
import csv
import os
import json

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
        global Connected
        Connected = True
    else:
        print("Connection failed")

def on_message(client, userdata, message):
    print("\nMessage received.")

    try:
        payload = json.loads(message.payload.decode("utf-8"))
        decoded = payload["uplink_message"]["decoded_payload"]
        bpm = decoded.get("bpm")
        systolic = decoded.get("systolic")
        diastolic = decoded.get("diastolic")

        # File path
        file_path = "myData.csv"
        file_exists = os.path.isfile(file_path)

        with open(file_path, 'a', newline='') as csvfile:
            writer = csv.writer(csvfile)

            # Write header if file doesn't exist yet
            if not file_exists:
                writer.writerow(["bpm", "systolic", "diastolic"])

            # Write the actual data
            writer.writerow([bpm, systolic, diastolic])

        print(f"Logged: bpm={bpm}, systolic={systolic}, diastolic={diastolic}")

    except Exception as e:
        print(f"Error processing message: {e}")

# --- MQTT setup ---
Connected = False

broker_address = "nam1.cloud.thethings.network"
port = 1883
user = "mae4220-telehealth@ttn"
password = "NNSXS.5I5BKPVDVCVBZLXYKADNM47PPGYYKGAD5XGGP3I.RNGEWQB3N6Y7R3QCIWDZYRZFC5KCR6Q32VECNUXXHT5KX4TFIB3A"

client = mqttClient.Client("Python")
client.username_pw_set(user, password=password)
client.on_connect = on_connect
client.on_message = on_message

client.connect(broker_address, port, 60)
client.subscribe(f"v3/{user}/devices/+/up")

client.loop_forever()
