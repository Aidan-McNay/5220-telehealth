# encryption

Run the **process.py** Python script to subscribe to TTN and obtain blood pressure data in CSV format (myData.csv) as described in the report.

Run the **process2.py** does the same thing as process.py except it records the data in a txt file (myData.txt). 

**encryption.cpp** and **encryption.h** contain the encryption function that can be called in the state machine in future implementations.

**rijndael.c** and **rijndael.h** are the 3rd party encryption libraries that we obtained from the BTStack GitHub and are included in my encryption.h file.

**process_decrypt** was the decryption python script. It is similar to process.py but with the added decryption component. It successfully subscribes to TTN.

**process.ipynb** is the Python notebook where I experimented and tested my decryption code.



Note: Our keys for the extra layer of AES-128 encryption are hardcoded into encryption.cpp and process.py. In the future, it might be helpful to keep the keys in a separate and hidden file for better security.
