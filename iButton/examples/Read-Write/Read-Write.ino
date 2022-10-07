#include <iButton.h>

iButton iBtn(A3); // iButton на пине A3
String keyName;
byte keyType;
byte keyCode[8];
byte keyCRC8;

byte newCode[] = { 0x01, 0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0xCF }; // Код-вездеход для ...

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  if (iBtn.readKey(keyCode)) // Пытаемся считать ключ и записать его код в переменную keyCode
  {
    // Ключ найден!
    keyType = iBtn.getType(keyCode[0]); // Получаем тип ключа. 0 - ???; 1 - Dallas; 2 - Cyfral
    keyName = getName(keyType);         // Получаем называние ключа. (Dallas, Cyfral)

    keyCRC8 = iBtn.crc8(keyCode); // Получаем CRC кода ключа

    if (Serial.read() == 'w') // Для записи ключа пишем в serial букву w
    {
      if (!iBtn.writeKey(newCode))  // Пытаемся записать ключ. В случае неудачи отработает код ниже
      {
        Serial.println("Error");  // Ошибка. Не удалось записать ключ
      }
      else
      {
        Serial.println("Write done!");
      }

      //iBtn.writeKey(newCode); // Можно записывать ключ так
    }
    sendToSerial(); // Отправляем полученные данные в serial
    delay(1000);
  }
  else
  {
    Serial.println("Key not found");
  }
}

String getName(byte _keyType) // Получить имя ключа (RW1990, Cyfral)
{
  switch (_keyType)
  {
    case 0:
      return "Opener of everything :)"; // Возможно это ключ-вездеход
      break;
    case 1:
      return "Dallas";
      break;
    case 2:
      return "Cyfral";
      break;
  }
}

void sendToSerial()
{
  Serial.println();
  Serial.println("Find a key: " + keyName);
  Serial.print("Key code: ");
  for (byte i = 0; i < 8; i++)
  {
    Serial.print(keyCode[i], HEX); Serial.print(" ");
  }
  Serial.println();  Serial.print("CRC8: "); Serial.println(keyCRC8, HEX);
}