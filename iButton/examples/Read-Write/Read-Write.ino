#define keyData      17   // iButton Data
#define keyPullup    16   // Подтяжка резистором 2.4 кОм

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
  iBtn.pullUp(); // Включаем подтяжку
}

void loop()
{
  //if (iBtn.readKey(keyCode, false)) // Чтение ключа без проверки корректности crc8
  if (iBtn.readKey(keyCode)) // Пытаемся считать ключ и записать его код в переменную keyCode
  {
    keyType = iBtn.getType(keyCode); // Получаем тип ключа. 0 - Unknown; 1 - Dallas; 2 - Cyfral
    keyName = iBtn.getKeyName(keyType);      // Получаем называние ключа. (Unknown, Dallas, Cyfral)
    keyCRC8 = iBtn.crc8(keyCode);    // Получаем CRC кода ключа
	
	// Валидность кода Cyfral можно проверить при помощи этой функции
	// if (keyType == 2 && iBtn.checkCyfralCode(keyCode)) { Serial.println("Cyfral valid!"); }
	
    if (Serial.read() == 'w') // Для записи ключа пишем в Serial букву w
    {
	  Serial.println("Writing key...");
      if (iBtn.writeKey(newCode))  // Пытаемся записать ключ. В случае неудачи отработает код ниже
      {
        Serial.println("Write done!");
      }
      else
      {
        Serial.println("Error");  // Ошибка. Не удалось записать ключ
      }

      //iBtn.writeKey(newCode); // Можно записывать ключ так
    }
    sendToSerial(); // Выводим полученные данные
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