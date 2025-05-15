import sys
import subprocess

def install_if_needed(package):
    try:
        __import__(package)
    except ImportError:
        subprocess.check_call([sys.executable, "-m", "pip", "install", package])

# Package names for import vs pip can differ
install_if_needed("Crypto")         # pycryptodome
install_if_needed("dateutil")       # python-dateutil
install_if_needed("pytz")           # pytz
install_if_needed("paho.mqtt.client")  # paho-mqtt

import struct
import csv
import os
from datetime import datetime
from Crypto.Cipher import AES
from datetime import date
from dateutil.parser import parse
import pytz
import paho.mqtt.client as mqttClient
import time
import json
import base64

def decryption(key: bytes, ciphertext: bytes) -> bytes:
    """
    Decrypt a 16-byte AES-128 ciphertext using ECB mode.

    Parameters:
        key (bytes): 16-byte AES key.
        ciphertext (bytes): 16-byte ciphertext.

    Returns:
        bytes: Decrypted plaintext.
    """
    assert len(key) == 16, "Key must be 16 bytes for AES-128"
    assert len(ciphertext) == 16, "Ciphertext must be 16 bytes"

    cipher = AES.new(key, AES.MODE_ECB)
    plaintext = cipher.decrypt(ciphertext)
    return plaintext

#Hardcoded encryption key
key = bytes.fromhex("e7a5c3f2d48a0e3bc96117b5fdfba247")

#Subscribe to TTN
def on_connect(client, userdata, flags, rc):
    if rc == 0:

        print("Connected to broker")

        global Connected                #Use global variable
        Connected = True                #Signal connection

    else:

        print("Connection failed")

def on_message(client, userdata, message):

#Process TTN
print("\nMessage received")

payload_str = message.payload.decode('utf-8')
payload_json = json.loads(payload_str)

frm_payload_base64 = payload_json['uplink_message']['frm_payload']
ciphertext = base64.b64decode(frm_payload_base64)

key = b'[AES key]'

#Decrypt
plaintext = decryption(key, ciphertext)

print("Decrypted plaintext:", plaintext)

# Extract systolic, diastolic, bpm (2 bytes each)
systolic = int.from_bytes(plaintext[0:2], byteorder='big')
diastolic = int.from_bytes(plaintext[2:4], byteorder='big')
bpm = int.from_bytes(plaintext[4:6], byteorder='big')

print(f"Systolic: {systolic}, Diastolic: {diastolic}, BPM: {bpm}")

 # Prepare CSV logging
timestamp = datetime.now().isoformat()

file_exists = os.path.isfile('bp_log.csv')

# Write the new reading to CSV
with open('bp_log.csv', mode='a', newline='') as f:
    writer = csv.DictWriter(f, fieldnames=["time", "systolic_pressure", "diastolic_pressure", "bpm"])

    # If the file is new, write the header
    if not (file_exists):
        writer.writeheader()

    writer.writerow({
    "time": timestamp,
    "systolic_pressure": systolic,
    "diastolic_pressure": diastolic,
    "bpm": bpm
    })

Connected = False   #global variable for the state of the connection

broker_address= "nam1.cloud.thethings.network"  #host
port = 1883                         #Broker port
user = "mae4220-telehealth@ttn" #Connection username
password = "NNSXS.5I5BKPVDVCVBZLXYKADNM47PPGYYKGAD5XGGP3I.RNGEWQB3N6Y7R3QCIWDZYRZFC5KCR6Q32VECNUXXHT5KX4TFIB3A" #Connection password

client = mqttClient.Client("Python")               #create new instance
client.username_pw_set(user, password=password)    #set username and password
client.on_connect= on_connect                      #attach function to callback
client.on_message= on_message                      #attach function to callback
client.connect(broker_address,port,60) #connect
client.subscribe(f"v3/{user}/devices/+/up") #subscribe
client.loop_forever() #then keep listening forever
