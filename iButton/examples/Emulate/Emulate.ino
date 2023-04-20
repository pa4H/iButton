#define keyData      17   // iButton Data
#define keyPullup    16   // Подтяжка резистором 2.4 кОм

#include <iButton.h>

iButton iBtn(keyData, keyPullup);

byte dallasCode[] = { 0x01, 0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0xCF }; // Код-вездеход для ...
byte cyfralCode[] = { 0x17, 0x7B, 0xDB, 0xEB, 0xD0, 0x00, 0x00, 0x00 }; // Код-вездеход для ...

void setup()
{
  Serial.begin(9600);
  Serial.println("\nStarted!");
  iBtn.pullUp(); // Включаем подтяжку
}

void loop() // Бесконечно отправляем код
{
	//iBtn.emulateRW1990(dallasCode); 
	iBtn.emulateCyfral(cyfralCode);
}