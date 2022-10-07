#include <iButton.h>

iButton iBtn(2); // iButton на пине D2

byte code[] = {0x01, 0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0xCF}; // Код-вездеход для ...

void setup()
{
  Serial.begin(9600);
}

void loop() // Бесконечно отправляем код
{
	emulateRW1990(code); 
	//emulateCyfral(code);
}