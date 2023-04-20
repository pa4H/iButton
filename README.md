# iButton
Library for interacting with keys _RW1990.1, RW1990.2, Cyfral_
100% work on __ATmega328__.

__ESP8266__ does not support reading _Cyfral_ keys. You can make support and send the code to me :)

## How to connect
![aaaaaaa](https://github.com/pa4H/iButton/blob/main/Connections.png)

## Example
```cpp
#define keyData      17   // iButton Data pin
#define keyPullup    16   // Pullup pin (2.4 kOhm resistor)

#include <iButton.h>
iButton iBtn(keyData, keyPullup);
String keyName;
byte keyType;
byte keyCode[8];
byte keyCRC8;

byte newCode[] = { 0x01, 0xC7, 0xA4, 0xC8, 0x11, 0x00, 0x00, 0xCF };

void setup()
{
  Serial.begin(9600);
  Serial.println("\nStarted!");
  iBtn.pullUp(); // Enable pullup
}

void loop()
{
  //if (iBtn.readKey(keyCode, false)) // If you want read key without CRC8 check
  if (iBtn.readKey(keyCode)) // Trying to read the key and write its code to a variable
  {
    keyType = iBtn.getType(keyCode); // Get key type: [0 - Unknown; 1 - Dallas; 2 - Cyfral]
    keyName = iBtn.getKeyName(keyType);      // Get key name: [Unknown, Dallas, Cyfral]
    keyCRC8 = iBtn.crc8(keyCode);    // Get CRC8 hash of key
	
	// Validity of Cyfral code you can check with this function
	// if (keyType == 2 && iBtn.checkCyfralCode(keyCode)) { Serial.println("Cyfral valid!"); }
	
    if (Serial.read() == 'w') // To write the key, write in the Serial letter 'w'
    {
	  Serial.println("Writing key...");
      if (iBtn.writeKey(newCode))  // trying to write down the key. If it fails, the code below will run
      {
        Serial.println("Write done!");
      }
      else
      {
        Serial.println("Error");  // Error. Failed to write key
      }

      //iBtn.writeKey(newCode); // You can write the key like this
    }
    sendToSerial(); // Display the received data
  }
  else
  {
    Serial.println("Key not found");
  }
  delay(1000);
}

void sendToSerial()
{
  Serial.println("Find a key: " + keyName);
  Serial.print("Key code: ");
  for (byte i = 0; i < 8; i++)  {
    Serial.print(keyCode[i], HEX); Serial.print(" ");
  }
  Serial.println();
  if (keyType != 2) {
    Serial.print("CRC8: ");
    Serial.println(keyCRC8, HEX);
  }
  Serial.println();
}
```
