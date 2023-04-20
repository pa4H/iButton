/*
  Liba by pa4H (c) 2022.
  Библиотека умеет считывать ключи RW1990.1 & RW1990.2 & Cyfral.
  И записывать болванки RW1990.1 & RW1990.2.
  На ATmega328 работает всё.

  Библиотеку можно использовать и распространять абсолютно бесплатно.
  compiled on Arduino IDE 1.8.13
*/

/* Dallas: keyCode[0] = family code. keyCode[1-6] = код ключа. keyCode[7] = CRC8
   Cyfral:
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

class iButton
{
  public:
    iButton(byte _iBtnPin, byte _pullUpPin)
    {
      iButtonPin = _iBtnPin;
      pullUpPin = _pullUpPin;
    }

    void pullUp()
    {
      pinMode(pullUpPin, OUTPUT); digitalWrite(pullUpPin, HIGH);
    }
    void pullDown()
    {
      pinMode(pullUpPin, INPUT);
    }

    String getKeyName(byte _keyType) // Получить имя ключа (RW1990, Cyfral)
    {
      switch (_keyType)
      {
        case 0:
          return "Unknown"; // Возможно это ключ-вездеход
          break;
        case 1:
          return "Dallas";
          break;
        case 2:
          return "Cyfral";
          break;
      }
    }

    bool readKey(byte *_readedCode, bool checkHash = true) // Вернет true если найден ключ. Код записывается в переданную переменную
    {
      for (byte i = 0; i < 8; i++) {
        _readedCode[i] = 0x00;
      }
      if (readCyfralKey(_readedCode)) // Сначала ищем Cyfral
      {
        return true; // Нашли ключ
      }

      // Не нашли Cyfral? Ищем Dallas
      if (!reset()) { // Если уровень HIGH
        return false;
      }

      write(0x33); // Read ROM
      for (byte i = 0; i < 8; i++) {
        _readedCode[i] = read();
      }
	  
      if (!checkHash) {
        return true;
      }

      if (_readedCode[0] == 0x01) // Dallas
      {
        if (crc8(_readedCode) != _readedCode[7]) {
          return false; // CRC невалиден
        }
        return true; // CRC совпадает
      }
      return false; // Вообще ничего не нашли
    }

    byte crc8(byte *_hash) // Получить CRC8 хэш
    {
      byte crc = 0;
      for (byte i = 0; i < 7; i++)
      {
        byte data = _hash[i];
        for (int j = 8; j > 0; j--)
        {
          crc = ((crc ^ data) & 1) ? (crc >> 1) ^ 0x8C : (crc >> 1);
          data >>= 1;
        }
      }
      return crc;
    }

    bool checkCyfralCode(byte *_cyfralCode)
    {
      for (byte i = 0; i < 5; i++)
      {
        byte buf = _cyfralCode[i] & 0b11110000;
        if (buf != 0b11100000 && buf != 0b11010000 && buf != 0b10110000 && buf != 0b01110000 && buf != 0b00010000)
        {
          return false;
        }
        if (i != 4)
        {
          buf = _cyfralCode[i] & 0b00001111;
          if (buf != 0b00001110 && buf != 0b00001101 && buf != 0b00001011 && buf != 0b00000111 && buf != 0b00000001)
          {
            return false;
          }
        }
      }
      return true;
    }

    byte getType(byte *_code) // Получить тип прочитанного ключа в byte (0 - ???; 1 - Dallas; 2 - Cyfral;)
    {
      if (_code[0] == 0x01) // Это Даллас
      {
        return 1;
      }
      if (_code[0] > 1) // У Cyfral первый байт не может быть равен 1
      {
        if ((_code[0] & 0b11110000) == 0b00010000) // Выделяем из *_code[0] левые четыре бита. Если они равны 0001, то...
        {
          return 2; // ...это Cyfral
        }
      }
      return 0; // Неизвестный ключ. Возможно ключ-вездеход
    }

    bool writeKey(byte *_code)
    {
      byte kType, rwCmd, rwBit;
      // Пробуем определить RW1990.1
      reset();
      write(0xD1); // Команда установки режима записи
      writeBit(1); // Запрещаем запись
      reset();
      write(0xB5); // Запрос на чтение флага записи
      if (read() == 0xFE) // FE - запрещено. FF - разрешено
      {
        kType = 1; // RW1990.1
      }
      else
      {
        // Пробуем определить RW1990.2
        reset();
        write(0x1D); // Команда установки режима записи
        writeBit(1); // Разрешаем запись
        delay(10);   // Выжидаем 10мсек
        reset();
        write(0x1E); // Запрос на чтение флага записи
        if (read() == 0xFE)
        { // FE - разрешено. FF - запрещено
          reset();
          write(0x1D);
          writeBit(0); // Ставим запрет записи
          delay(10);
          kType = 2; // RW1990.2
        }
      }
      switch (kType)
      {
        case 1:
          rwCmd = 0xD1;
          rwBit = 0;
          break; // RW1990.1
        case 2:
          rwCmd = 0x1D;
          rwBit = 1;
          break; // RW1990.2
        case 3:
          rwCmd = 0xC1;
          break; // TM-01
      }

      reset();
      write(rwCmd);
      writeBit(rwBit); // Запрос на запись
      reset();

      if (kType == 3)
      {
        //write(0xC5); // Команда на запись для TM01
        // Пишем ТМ01
        //reset();
        //write(0xCA); // Финализируем в Cyfral
        // write(0xCB); // send 0xCB - флаг финализации metacom
        //writeBit(1); // записываем значение флага финализации = 1 - перевезти формат
        //delay(10);
      }
      else
      {
        write(0xD5); // Команда на запись для RW1990
        for (byte i = 0; i < 8; i++)
        {
          if (kType == 1)
          {
            write(~_code[i]); // Запись RW1990.1 происходит инверсно
          }
          else
          {
            write(_code[i]);
          }
        }
        reset();
        write(rwCmd);
        writeBit(!rwBit); // Отключаем запись
      }

      byte buf[8];
      readKey(buf, false); // Проверка корректности записи
	  
	  for (byte i = 0; i < 2; i++)
  {
    if (buf[i] != _code[i])
    {
      return false;
    }
  }
  return true;
    }

    void emulateRW1990(byte *_dKey)
    {
      
    }

    // Тп(среднее) 140мкс
    // 0 логический 0.4 * Тп = 56
    // 1 логическая 0.6 * Тп = 84
#define cLow 56
#define cHigh 84
    void emulateCyfral(byte *_cyfKey)
    {
      for (byte i = 0; i < 5; i++) // У Cyfral код состоит из 4 байт
      {
        for (byte b = 8; b > 0; b--)
        {
          if (i == 4 && b == 4) { // В последнем байте у Cyfral только 4 левых бита
            break;
          }
          MOW_CLI();
          pinMode(iButtonPin, INPUT);
          if (((_cyfKey[i] >> b - 1) & 0x01) == 1) // Перебираем биты. Если бит[b] == 1, то...
          {
            delayMicroseconds(cHigh);
            pinMode(iButtonPin, OUTPUT);
            delayMicroseconds(cLow);
          }
          else
          {
            delayMicroseconds(cLow);
            pinMode(iButtonPin, OUTPUT);
            delayMicroseconds(cHigh);
          }
          MOW_SEI();
        }
      }
    }
	
	void emulDomofon()
	{
		 if (!reset()) { // Если уровень HIGH
        return false;
      }

      write(0x33); // Read ROM
	}
	
	bool waitForPresence()
	{
		uint32_t timeout = millis();
		while (!digitalRead(iButtonPin) && millis() - timeout < 300); // Ждём пока идёт reset 
		MOW_CLI();
		delayMicroseconds(30); // Должны показать присутствие не позднее 60мкс
		pinMode(iButtonPin, OUTPUT);
		delayMicroseconds(140); // Прижимаем линию от 60 до 240мкс
		pinMode(iButtonPin, INPUT);
		MOW_SEI();
		if (!digitalRead(iButtonPin)) { return false; } // Если линия прижата к земле, значит идёт reset
		return true;
	}
	
	 byte readSlave()
    {
      byte data = 0;
		uint32_t timeout;
      for (byte i = 8; i; i--)
      {
		data >>= 1;
        MOW_CLI();
		timeout = millis();
		while (!digitalRead(iButtonPin) && millis() - timeout < 300); // Ничего не делаем пока 0
		timeout = millis();
		while (digitalRead(iButtonPin) && millis() - timeout < 300); // Ничего не делаем пока 1
		delayMicroseconds(30); // Выжидаем от 15 до 60мкс
        if (digitalRead(iButtonPin))
        {
          data |= (1 << 7);
        }
        MOW_SEI();
      }
      return data;
    }
	void writeSlave(byte data)
    {
      for (byte i = 8; i; i--)
      {
        writeBitSlave(data & 1);
        data >>= 1;
      }
    }
	void writeBitSlave(byte data)
    {
		MOW_CLI();
		uint32_t timeout = millis();
		while (!digitalRead(iButtonPin) && millis() - timeout < 300); // Ждём пока уровень низкий 
      
      if (data == 0) // Если надо передать 1, то ничего не делаем
      {
        pinMode(iButtonPin, OUTPUT);
		delayMicroseconds(30);
		pinMode(iButtonPin, INPUT);
      }
     
	  timeout = millis();
		while (digitalRead(iButtonPin) && millis() - timeout < 300); // Ждём пока уровень высокий 
		MOW_SEI();
    }

  private:
  #define onewire_low() pinMode(iButtonPin, OUTPUT)
  #define onewire_high() pinMode(iButtonPin, INPUT)
    byte iButtonPin, pullUpPin;

// microOneWireSlave by pa4H


    // microOneWire by Nich1con
    bool reset()
    {
      onewire_low();
      delayMicroseconds(600);
      onewire_high();
      MOW_CLI();
      delayMicroseconds(60); // Ждём не менее 60мс
      bool pulse = digitalRead(iButtonPin); // Считываыем presence
      MOW_SEI();
	  delayMicroseconds(600); // Следующий импульс сброса должен посылаться не раньше, чем через 480мкс
      return !pulse;
    }

    void write(byte data)
    {
      for (byte i = 8; i; i--)
      {
        writeBit(data & 1);
        data >>= 1;
      }
    }

    void writeBit(byte data)
    {
      onewire_low();
      MOW_CLI();
      if (data == 1)
      {
        delayMicroseconds(5);
        onewire_high();
        delayMicroseconds(90);
      }
      else
      {
        delayMicroseconds(90);
        onewire_high();
        delayMicroseconds(5);
      }
      MOW_SEI();
	  delay(10);
    }

    byte read()
    {
      byte data = 0;
      for (byte i = 8; i; i--)
      {
        data >>= 1;
        MOW_CLI();
        onewire_low();
        delayMicroseconds(2);
        onewire_high();
        delayMicroseconds(8);
        if (digitalRead(iButtonPin))
        {
          data |= (1 << 7);
        }
        delayMicroseconds(80);
        MOW_SEI();
      }
      return data;
    }

    // Cyfral \\

    byte _admux, _adcsrb, _adcsra;
    void configADC() // https://avrprog.blogspot.com/2013/03/blog-post_13.html
    {
      _admux = ADMUX;   // Запоминаем дефолтные значения АЦП
      _adcsrb = ADCSRB;
      _adcsra = ADCSRA;
      ADMUX = (DEFAULT << 6) | ((iButtonPin < 8) ? iButtonPin : iButtonPin - 14) | (1 << ADLAR); // Измерение до VCC | A0-A5 | левое выравнивание
      delay(2);                                          // Ждем пока АЦП придёт в себя
      ADCSRB = 0b00000000;                                                       // Непрерывное преобразование
      ADCSRA = 0b011 | (1 << ADEN);                                                // CLK\8 | вкл АЦП
    }
    void returnADC() // Возвращаем регистры АЦП в исходное состояние
    {
      ADMUX  = _admux;
      ADCSRB = _adcsrb;
      ADCSRA = _adcsra;
    }
    byte adcRead()
    {
      ADCSRA |= (1 << ADSC);    // Запускаем преобразование
      while (ADCSRA & (1 << ADSC)); // Ждем окончания преобразования
      return ADCH;          // Возвращаем 8 бит
    }

    byte hPulse;     // Сигнал высокого уровня
    byte hPulseTime; // Время высокого испульса
    uint32_t tT;
    void getHighImpulse() // Определяем длину импульса логической единицы
    {
      hPulse = 0;
      hPulseTime = 0;
      byte volts;
      for (byte i = 0; i < 16; i++) // Определяем наивысшее напряжение
      {
        volts = adcRead();
        if (volts > hPulse) // Ищем самое большое значение
        {
          hPulse = volts;
        }
      }
      if (hPulse < 70 || hPulse > 120) {
        return;  // Это точно не Cyfral. Выходим из функции
      }
      hPulse -= 10; // Отнимаем 10 для гарантии

      byte mksek;
      byte timeout;
      for (byte i = 0; i < 8; i++)  // Определяем наивысшее напряжение
      {
        timeout = 0;
        do {
          timeout++;
        }
        while (adcRead() < hPulse && timeout < 255); // Ничего не делаем пока сигнал НИЗКОГО уровня или не истечёт таймаут
        tT = micros();        // Запоминаем начало времени

        timeout = 0;
        do {
          timeout++;
        }
        while (adcRead() > hPulse && timeout < 255); // Ничего не делаем пока сигнал ВЫСОКОГО уровня или не истечёт таймаут
        mksek = micros() - tT;      // Получаем длительность высокого уровня сигнала
        if (mksek > hPulseTime)     // Ищем самое большое значение
        {
          hPulseTime = mksek;
        }
      }
      hPulseTime -= 10; // Отнимаем 10 мксек для гарантии
    }

    bool readCyfralKey(byte *cyfralCode)
    {
      configADC();
      getHighImpulse();
      if (hPulseTime == 0) {
        returnADC();  // Для ускорения работы функции
        return false;
      }

      byte buf[72];
      byte mksek;
      byte timeout;
      for (byte r = 0; r < 72; r++)
      {
        timeout = 0;
        do {
          timeout++;
        }
        while (adcRead() < hPulse && timeout < 255); // Ничего не делаем пока сигнал низкого уровня или не истечёт таймаут
        tT = micros(); // Запоминаем начало времени
        timeout = 0;
        do {
          timeout++;
        }
        while (adcRead() > hPulse && timeout < 255); // Ничего не делаем пока сигнал высокого уровня или не истечёт таймаут
        mksek = micros() - tT; // Получаем время высокого уровня сигнала

        if (mksek >= hPulseTime) // Если время импульса логической 1 примерно 60%
        {
          buf[r] = 1; // то пишем 1
        }
        else
        {
          buf[r] = 0;
        }
      }

      returnADC(); // Возвращаем ЭЦП в исходное состояние

      byte zeros = 0;       // Поиск стартового слова. Сюда пишем количество нулей
      bool founded = false; // Нашли стартовое слово
      byte a = 3;           // Номер бита в который будем записывать код
      byte b = 0;           // Номер байта в который записываем код
      for (byte i = 0; i < 72; i++)
      {
        if (!founded) // Тут ищем 0001
        {
          if (buf[i] == 1 && zeros < 3)
          {
            zeros = 0; // Если на пути встретилась единица. Сбрасываем количество нулей. Например 001 будет сброс
          }
          if (buf[i] == 0)
          {
            zeros++;
          }
          if (zeros == 3 && buf[i] == 1) // Если три ноля и попалась единица (код 0001)
          {
            founded = true;
            cyfralCode[0] = 0b00010000; // Заносим стартовое слово в первый байт
            continue;         // Пропускаем данную итерацию. Произойдет i++;
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
          if (a == 0) // Записали байт? Идём писать следующий
          {
            a = 7;
            b++;
          }
          else
          {
            a--; // Пишем следующий бит
          }
          if (b == 4 && a == 3) // Если полностью записали 4 байта и четыре первых бита пятого байта. code[5] = 0b 1011 0000
          {
            if (checkCyfralCode(cyfralCode)) {
              return true;
            }
            return false;
          }
        }
      }
      return false;
    }
};