# iButton
Библиотека для взаимодействия с ключами _RW1990.1, RW1990.2, Cyfral, TM01_
Гарантирована работоспособность на __ATmega328__.

__ESP8266__ не поддерживает чтение ключей _Cyfral_. Можете запилить поддержку и отправить код мне :)

## Инициализация
```cpp
iButton iBtn(A3); // Ключ на пине A3
pinMode(5, OUTPUT); digitalWrite(5, HIGH); // Подаем напряжение на резистор подтяжки
byte keyCode[8]; // Здесь храним считанный ключ
```

## Чтение ключа
```cpp
if (iBtn.readKey(keyCode)) // Пытаемся считать ключ и записать его код в переменную keyCode
  {
     iBtn.getType(keyCode[0]); // Получаем тип ключа. 0 - ???; 1 - Dallas; 2 - Cyfral
     iBtn.crc8(keyCode);       // Получаем CRC кода ключа
     iBtn.writeKey(newCode)    // Пытаемся записать ключ. Тип ключа определяется автоматически
  }
```

## Эмуляция ключа
```cpp
iBtn.emulateRW1990(keyCode); // Эмуляция самой популярной таблетки RW1990
iBtn.emulateCyfral(keyCode); // Эмуляция ключа Cyfral
```
