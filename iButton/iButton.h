/*
  Liba by pa4H (c) 2022.
  Библиотека умеет считывать ключи типа RW1990.1 & RW1990.2 & TM01 & Cyfral.
  Позволяет записывать болванки  RW1990.1 & RW1990.2 & TM01.
  На ATmega328 работает всё.
  На остальных МК можно не будут считываться ключи Cyfral.
  Можете написать быстрое обращение к АЦП нужного вам микроконтроллера и отправить код мне, обновлю либу :)
  Библиотеку можно использовать и распространять абсолютно бесплатно.
*/

#pragma once
#include <Arduino.h>

#ifdef __AVR__
#define MOW_CLI() byte oldSreg = SREG; cli();
#define MOW_SEI() SREG = oldSreg
#else
#define MOW_CLI()
#define MOW_SEI()
#endif

class iButton {
  public:
    iButton(byte _iBtnPin) {
      iButtonPin = _iBtnPin;
    }

    bool readKey(byte* _readedCode) // Вернет true если найден ключ. Код записывается в переданную переменную
    {
      if (!reset()) {
        return false;
      }

      write(0x33); // Read ROM
      for (byte i = 0; i < 8; i++) {
        _readedCode[i] = read();
      }
      if (_readedCode[0] == 0x01)
      {
        if (crc8(_readedCode) != _readedCode[7])
        {
          return false; // CRC невалиден
        }
        return true; // CRC совпадает
      }
      // Ищем Цифрал
      return false;
    }

    byte test()
    {
      byte answer;
      reset(); write(0xD1); // проуем снять флаг записи для RW-1990.1
      writeBit(0);                 // отключаем запись
      delay(10);
      reset(); write(0xB5); // send 0xB5 - запрос на чтение флага записи
      answer = read();
      //Serial.print(F("\n Answer RW-1990.1: ")); Serial.println(answer, HEX);
      return answer;            // это RW-1990.1
    }

    byte crc8(byte* _hash) // Получить CRC8 хэш
    {
      byte crc = 0;
      for (byte i = 0; i < 7; i++) {
        byte data = _hash[i];
        for (int j = 8; j > 0; j--) {
          crc = ((crc ^ data) & 1) ? (crc >> 1) ^ 0x8C : (crc >> 1);
          data >>= 1;
        }
      }
      return crc;
    }

    byte getType(byte *_code)   // Получить тип прочитанного ключа в byte (0 - ???; 1 - Dallas; 2 - Cyfral;)
    {
		if (_code[0] == 0xFF) // А это ХЗ чё такое. Возможно ключ-вездеход
		{
			return 0;
		}
		if (_code[0] == 0x01) // Это Даллас
		{
			return 1;
		}
		if (_code[0] > 1) // У Цифрала первый байт не может быть равен 1
		{
			if ((_code[0] & 0b11110000) == 0b00010000) // Выделяем из *_code[0] левые четыре бита. Если они равны 0001, то...
			{
				return 2; // ...это Цифрал
			}
			
		}
    }

    bool writeKey(byte *_code)
    {
      byte kType, rwCmd, rwBit;

      // Пробуем определить RW1990.1
      reset();
      write(0xD1); // Команда установки режима записи
      writeBit(1); // Запрещаем запись
      delay(10);
      reset();
      write(0xB5); // Запрос на чтение флага записи
      if (read() == 0xFE) { // FE - запрещено. FF - разрешено
        kType = 1; // RW1990.1
      }
      // Пробуем определить RW1990.2
      reset();
      write(0x1D);  // Команда установки режима записи
      writeBit(1);  // Разрешаем запись
      delay(10);    // Выжидаем 10мсек
      reset();
      write(0x1E);  // Запрос на чтение флага записи
      if (read() == 0xFE) { // FE - разрешено. FF - запрещено
        reset();
        write(0x1D); writeBit(0); // Ставим запрет записи
        delay(10);
        kType = 2; // RW1990.2
      }
      reset();
      kType = 3; //TM01 а вообще надо найти способ определения ТМ01

      switch (kType) {
        case 1: rwCmd = 0xD1;  			 break; // RW1990.1
        case 2: rwCmd = 0x1D; rwBit = 1; break; // RW1990.2
        case 3: rwCmd = 0xC1; 		     break; // TM-01
      }

      reset();
      write(rwCmd); writeBit(rwBit); // Запрос на запись
      delay(10);
      reset();

      if (kType == 3)
      {
        write(0xC5); // Команда на запись для TM01

        //Пишем ТМ01
        // ???

        reset();
        write(0xCA); // Финализируем в Cyfral
        //write(0xCB); // send 0xCB - флаг финализации metacom
        writeBit(1); // записываем значение флага финализации = 1 - перевезти формат
        delay(10);
      }
      else
      {
        write(0xD5); // Команда на запись для RW1990
        for (byte i = 0; i < 8; i++)
        {
          if (kType == 1) {
            write(~_code[i]); // Запись RW1990.1 происходит инверсно
          } else {
            write(_code[i]);
          }
        }

        write(rwCmd); writeBit(!rwBit); //Отключаем запись
        delay(10);
      }
	  
      // Проверка корректности записи
	  byte buf[8];
	  readKey(buf);
	  if (buf == _code)
	  {
		  return true;
	  }
	  else
	  {
		  return false;
	  }
    }

    void emulateRW1990(byte* _sCode)
    {
      //onewire slave
    }
    void emulateCyfral(byte* _sCode)
    {

    }

  private:
    byte iButtonPin;

	// microOneWire by Nich1con
    bool reset() {
      pinMode(iButtonPin, OUTPUT);
      delayMicroseconds(600);
      pinMode(iButtonPin, INPUT);
      MOW_CLI();
      delayMicroseconds(60);
      bool pulse = digitalRead(iButtonPin);
      MOW_SEI();
      delayMicroseconds(600);
      return !pulse;
    }

    void write(byte data) {
      for (byte i = 8; i; i--) {
        writeBit(data & 1);
        data >>= 1;
      }
    }

    void writeBit(byte data) {
      pinMode(iButtonPin, OUTPUT);
      MOW_CLI();
      if (data == 1) {
        delayMicroseconds(5);
        pinMode(iButtonPin, INPUT);
        delayMicroseconds(60);
      } else {
        delayMicroseconds(60);
        pinMode(iButtonPin, INPUT);
        delayMicroseconds(5);
      }
      MOW_SEI();
    }

    byte read() {
      byte data = 0;
      for (byte i = 8; i; i--) {
        data >>= 1;
        MOW_CLI();
        pinMode(iButtonPin, OUTPUT);
        delayMicroseconds(2);
        pinMode(iButtonPin, INPUT);
        delayMicroseconds(8);
        if (digitalRead(iButtonPin)) data |= (1 << 7);
        delayMicroseconds(60);
        MOW_SEI();
      }
      return data;
    
	
	// Cyfral
	
	byte hPulse; // Сигнал высокого уровня
byte hPulseTime; // Время высокого испульса
uint32_t tT;
	void getHighImpulse() // Определяем длину импульса логической единицы
{
  byte volts;
  for (byte i = 0; i < 8; i++) // Определяем наивысшее напряжение
  {
    volts = adcRead();
    if (volts > hPulse) { // Ищем самое большое значение
      hPulse = volts;
    }
  }
  hPulse -= 10; // Отнимаем 10 для гарантии

  byte mksek;
  for (byte i = 0; i < 8; i++)  // Определяем наивысшее напряжение
  {
    while (adcRead() < hPulse); // Ничего не делаем пока сигнал низкого уровня
    tT = micros();              // Запоминаем начало времени
    while (adcRead() > hPulse); // Ничего не делаем пока сигнал высокого уровня
    mksek = micros() - tT;      // Получаем время высокого уровня сигнала
    if (mksek > hPulseTime) {   // Ищем самое большое значение
      hPulseTime = mksek;
    }
  }
  hPulseTime -= 10; // Отнимаем 10 мксек для гарантии
}

void configADC() // https://avrprog.blogspot.com/2013/03/blog-post_13.html
{
  ADMUX = (DEFAULT << 6) | ((iButtonPin < 8) ? iButtonPin : iButtonPin - 14) | (1 << ADLAR);    // Измерение до VCC | A0-A5 | левое выравнивание
  ADCSRB = 0b00000000; // Непрерывное преобразование
  ADCSRA = 0b011 | (1 << ADEN); // | (1 << ADSC) | (1 << ADIF); // CLK\8 | вкл АЦП | запуск преобразования
}
byte adcRead()
{
  ADCSRA |= (1 << ADSC); // Запускаем преобразование
  while (ADCSRA & (1 << ADSC)); // Ждем окончания преобразования
  return ADCH; // Возвращаем 8 бит
}

void readCyfralKey(byte* cyfralCode)
{
  byte buf[72];
  for (byte r = 0; r < 72; r++)
  {
    while (adcRead() < hPulse); // Ничего не делаем пока сигнал низкого уровня
    tT = micros();              // Запоминаем начало времени
    while (adcRead() > hPulse); // Ничего не делаем пока сигнал высокого уровня
    tT = micros() - tT;         // Получаем время высокого уровня сигнала

    if (tT > hPulseTime) { // Если время импульса логической 1 примерно 60%
      buf[r] = 1; // то пишем 1
    } else {
      buf[r] = 0;
    }
  }

  byte zeros = 0;       // Поиск стартового слова. Сюда пишем количество нулей
  bool founded = false; // Нашли стартовое слово
  byte a = 3; // Номер бита в который будем записывать код
  byte b = 0; // Номер байта в который записываем код
  for (byte i = 0; i < 72; i++)
  {
    if (!founded) // Тут ищем 0001
    {
      if (buf[i] == 1 && zeros < 3) {
        zeros = 0;  // Если на пути встретилась единица. Сбрасываем количество нулей. Например 001 будет сброс
      }
      if (buf[i] == 0)
      {
        zeros++;
      }
      if (zeros == 3 && buf[i] == 1) // Если три ноля и попалась единица (код 0001)
      {
        founded = true;
        cyfralCode[0] = 0b00010000; // Заносим стартовое слово в первый байт
        continue; // Пропускаем данную итерацию. Произойдет i++;
      }
    }
    else
    {
      if (buf[i] == 1) // Попалась единица
      {
        cyfralCode[b] |= (1 << a); // Ставим единицу в a. Например а = 6. Получается 0b01000000
      }
      else
      {
        cyfralCode[b] |= (0 << a);
      }
      if (a == 0) { // Записали байт? Идём писать следующий
        a = 7;
        b++;
      }
      else
      {
        a--; // Пишем следующий бит
      }
      if (b == 4 && a == 3) { // Если полностью записали 4 байта и четыре первых бита пятого байта. code[5] = 0b 1011 0000
        break;
      }
    }
  }
}

};