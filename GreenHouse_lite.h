#include <Wire.h>
#include <TimeLib.h>        //Бибилотека для работы с Tiny RTC
#include <DS1307RTC.h>        //Бибилотека для работы с Tiny RTC
#include <OneWire.h>    //Бибилотека для работы с DS1820
#include "DHT.h"    //Бибилотека для работы с DHT11
#include <SoftwareSerial.h>         // Библиотека програмной реализации обмена по UART-протоколу

// Номер пина Arduino с подключенным датчиком
#define PIN_DS18B20 6
// Номер пина Arduino с DHT11 
#define DHTPIN 5
#define DHTTYPE DHT11

// RX, TX GSM
SoftwareSerial SIM800(2, 3);                               

//****Переменные для работы с Tiny RTC
  uint8_t wday_now = 0, month_now = 0, day_now = 0, hour_now = 0, min_now = 0, sec_now = 0;
//*****

//******Переменные для работы с DS1820 
  // Создаем объект OneWire
  OneWire DS1820(PIN_DS18B20);
  //Значение температуры на DS1820
  float temp_out_DS1820;
//*****

//*************Настройка DHT11***************
  DHT dht(DHTPIN, DHTTYPE);
  float temp_IN = 0;
  float humidity_IN = 0;


//Таймеры
  uint32_t timer_1 = 0, timer_2 = 0;

//***Переменнные для работы с SIM800
  String _response = "";    // Переменная для хранения ответа модуля
  String phones = "+79137857684, +79133731599";   // Белый список телефонов
  String msgphone;                        // Переменная для хранения номера отправителя
  String msgbody;                         // Переменная для хранения текста СМС
  String outSMS;
  bool hasmsg = false;                                              // Флаг наличия сообщений к удалению
//****

//****************Данные для полива*********
  uint8_t mas_pins_valve[5] = {0, 7, 8, 9, 10};
  uint8_t watering_timer=2;
  uint8_t watering_time_1 = 13;
  uint8_t watering_time_2 = 18;
  bool watering_1 = false;
  bool watering_2 = false;
  bool watering_off = false;



void(* resetFunc) (void) = 0;      //Функция перезагрузки

//__________________________Функция получения времени от Tiny RTC_____________________________________
void ReadTime(){
   // Serial.println("ReadTime()>>>>>>>>>>>>");
   tmElements_t tm;
   if (RTC.read(tm)) {
    hour_now = tm.Hour;
    min_now = tm.Minute;
    sec_now = tm.Second;
    day_now = tm.Day;
    month_now = tm.Month;
    wday_now = tm.Wday;
   } else {
    if (RTC.chipPresent()) {
      // Serial.println("The DS1307 is stopped.  Please run the SetTime");
      // Serial.println();
    } else {
      // Serial.println("DS1307 read error!  Please check the circuitry.");
      // Serial.println();
    }
  }
    // Serial.print(hour_now);
    // Serial.print(".");
    // Serial.print(min_now);
    // Serial.print(".");
    // Serial.println(sec_now);
  // Serial.print("wd: ");
  // Serial.println(wday_now);
  // Serial.print(day_now);
  // Serial.print(".");
  // Serial.println(month_now);
}
//___________________________________________________________________________________________________


//______________________Получение температуры на улице________________________________
void GetTempDS1820() {
// Определяем температуру от датчика DS18b20
  byte data[2]; // Место для значения температуры
  
  DS1820.reset(); // Начинаем взаимодействие со сброса всех предыдущих команд и параметров
  DS1820.write(0xCC); // Даем датчику DS18b20 команду пропустить поиск по адресу. В нашем случае только одно устрйоство 
  DS1820.write(0x44); // Даем датчику DS18b20 команду измерить температуру. Само значение температуры мы еще не получаем - датчик его положит во внутреннюю память
  
  delay(1000); // Микросхема измеряет температуру, а мы ждем.  
  
  DS1820.reset(); // Теперь готовимся получить значение измеренной температуры
  DS1820.write(0xCC); 
  DS1820.write(0xBE); // Просим передать нам значение регистров со значением температуры
  // Получаем и считываем ответ
  data[0] = DS1820.read(); // Читаем младший байт значения температуры
  data[1] = DS1820.read(); // А теперь старший
  // Формируем итоговое значение: 
  //    - сперва "склеиваем" значение, 
  //    - затем умножаем его на коэффициент, соответсвующий разрешающей способности (для 12 бит по умолчанию - это 0,0625)
  temp_out_DS1820 =  ((data[1] << 8) | data[0]) * 0.0745 + 6;
  
  // Выводим полученное значение температуры в монитор порта
  // Serial.print("Temp OUT: ");
  // Serial.println(temp_out_DS1820);
}
//___________________________________________________________________________________

//*************************Получение температуры и влажности с DHT11***************************
void GetTempIN()
{
  // Serial.println("GetTempIN()>>>>>>>>>>>>>>");
  temp_IN = dht.readTemperature();
  humidity_IN = dht.readHumidity();
  // Serial.print("TempIN: ");Serial.println(temp_IN);
  // Serial.print("HumidityIN: ");Serial.println(humidity_IN);
  
  
 }
//-----------------------------------------------------------------------------------

//***************************Отправка команды модему******************************
String sendATCommand(String cmd, bool waiting) {
  delay(50);
  String _resp = "";                                              // Переменная для хранения результата
  // Serial.println("SendATCommand>>>>>");
  // Serial.println(cmd);                                            // Дублируем команду в монитор порта
  SIM800.println(cmd);                                            // Отправляем команду модулю SIM
  if (waiting) {                                                  // Если необходимо дождаться ответа...
    _resp = waitResponse();                                       // ... ждем, когда будет передан ответ
    // Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать
    if (_resp.startsWith(cmd)) {                                  // Убираем из ответа дублирующуюся команду
      _resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
    }
    Serial.println(_resp);                                        // Дублируем ответ в монитор порта
  }
  return _resp;                                                   // Возвращаем результат. Пусто, если проблема
  
}
//-----------------------------------------------------------------------------------

//***************Функциsя ожидания ответа и возврата полученного результата***************
String waitResponse() {                                           // 
  String _resp = "";                                              // Переменная для хранения результата
  Serial.println("Wait...");
  long _timeout = millis() + 10000;                               // Переменная для отслеживания таймаута (10 секунд)

  while (!SIM800.available() && millis() < _timeout)  {};         // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
    if (SIM800.available()) {                                       // Если есть, что считывать...
      _resp = SIM800.readString();                                  // ... считываем и запоминаем
       Serial.println(_resp);
    }
    else {                                                          // Если пришел таймаут, то...
      Serial.println("Timeout...");                                 // ... оповещаем об этом и...
    }
  return _resp;                                                   // ... возвращаем результат. Пусто, если проблема
}
//-----------------------------------------------------------------------------------

//**************************Проверка новых СМС***************************************
void CheckSMS (){
  
  Serial.println("CheckSMS>>>>>>>");
  do {
    _response = sendATCommand("AT+CMGL=\"REC UNREAD\",1", true);// Отправляем запрос чтения непрочитанных сообщений
    if (_response.indexOf("+CMGL: ") > -1) {                    // Если есть хоть одно, получаем его индекс
      uint8_t msgIndex = _response.substring(_response.indexOf("+CMGL: ") + 7, _response.indexOf("\"REC UNREAD\"", _response.indexOf("+CMGL: ")) - 1).toInt();
      char i = 0;                                               // Объявляем счетчик попыток
      do {
        i++;                                                    // Увеличиваем счетчик
        _response = sendATCommand("AT+CMGR=" + (String)msgIndex + ",1", true);  // Пробуем получить текст SMS по индексу
        _response.trim();                                       // Убираем пробелы в начале/конце
        if (_response.endsWith("OK")) {                         // Если ответ заканчивается на "ОК"
          if (!hasmsg) hasmsg = true;                           // Ставим флаг наличия сообщений для удаления
            sendATCommand("AT+CMGR=" + (String)msgIndex, true);   // Делаем сообщение прочитанным
            sendATCommand("\n", true);                            // Перестраховка - вывод новой строки
            msgphone = parseSMS(_response);                       // Отправляем текст сообщения на обработку
            break;                                                // Выход из do{}
        }
        else {                                                  // Если сообщение не заканчивается на OK
          //Serial.println ("Error answer");                      // Какая-то ошибка
          sendATCommand("\n", true);                            // Отправляем новую строку и повторяем попытку
        }
      } while (i < 10);
      break;
    }
    else {
      if (hasmsg) {
        sendATCommand("AT+CMGDA=\"DEL READ\"", true);           // Удаляем все прочитанные сообщения
        hasmsg = false;
      }
      break;
    }

  } while (1);
}
//-----------------------------------------------------------------------------------

// *****************************Парсим SMS*******************************
String parseSMS(String msg) {                                   
  String msgheader  = "";
  String msgbody    = "";
  //String msgphone   = "";
  // Serial.println("Parse!");
  msg = msg.substring(msg.indexOf("+CMGR: "));
  msgheader = msg.substring(0, msg.indexOf("\r"));            // Выдергиваем телефон

  msgbody = msg.substring(msgheader.length() + 2);
  msgbody = msgbody.substring(0, msgbody.lastIndexOf("OK"));  // Выдергиваем текст SMS
  msgbody.trim();

  uint8_t firstIndex = msgheader.indexOf("\",\"") + 3;
  uint8_t secondIndex = msgheader.indexOf("\",\"", firstIndex);
  msgphone = msgheader.substring(firstIndex, secondIndex);

  // Serial.println("Phone: " + msgphone);                       // Выводим номер телефона
  // Serial.println("Message: " + msgbody);                      // Выводим текст SMS

  if (msgphone.length() > 6 && phones.indexOf(msgphone) > -1) { // Если телефон в белом списке, то...
    //Serial.println("SMSSelectIN!");
    SMSSelect(msgbody);                           // ...выполняем команду
    
  }
  else {
    //Serial.println("Unknown phonenumber");
    }
  return msgphone;
}
//-----------------------------------------------------------------------------------

//****************************отправка SMS**************************************
void sendSMS(String phone, String message)
{
  // Serial.println("SendSMS>>>>>"); 
  // Serial.println("sendSMS!   " + phone+"    " + message);
  sendATCommand("AT+CMGS=\"" + phone + "\"", true);             // Переходим в режим ввода текстового сообщения
  sendATCommand(message + "\r\n" + (String)((char)26), true);   // После текста отправляем перенос строки и Ctrl+Z
}
//-----------------------------------------------------------------------------------



//*************************Функция обаработки запроса в СМС***************************
void SMSSelect(String sms){
  //Serial.println("SMSSelect!");
  if (sms.length() > 2){
    char * strtokIndx; // это используется функцией strtok() как индекс
      strtokIndx = strtok(sms.c_str(), "."); // получаем значение первой переменной
      uint8_t wtf = atoi(strtokIndx);     // конвертируем эту составляющую в integer
      if (wtf == 1){
        watering_off = false;
        strtokIndx = strtok(NULL, ".");
        watering_time_1 = atoi(strtokIndx);     // преобразовываем этот кусок текста
        strtokIndx = strtok(NULL, ".");
        watering_time_2 = atoi(strtokIndx);     // преобразовываем этот кусок текста 
        strtokIndx = strtok(NULL, ".");
        watering_timer = atoi(strtokIndx);     // преобразовываем этот кусок текста 
      } else if (wtf == 0){
          watering_off = true;
        }
        // Serial.print("watering_off set-");
        // Serial.println(watering_off);
        // Serial.print("watering_time_1 set - ");
        // Serial.println(watering_time_1);
        // Serial.print("watering_time_2 set - ");
        // Serial.println(watering_time_2);
        // Serial.print("watering_timer set - ");
        // Serial.println(watering_timer);
  } 
  else { 
    //+++++++++Перезагрузка+++++++++++++
    if (sms == "0" || sms == "r" || sms == "R")
    {
      // Serial.println("Reboot");
      String sSMS5 = ("Reboot!");
      sendSMS(msgphone, sSMS5);
      delay(15000);
      resetFunc();
    }

    //+++++++++Полив+++++++++++++
    if (sms == "1" || sms == "p1" || sms == "P1")
    {
      outSMS = "Zapusk poliva";
      sendSMS(msgphone, outSMS);
      // Serial.println("Запуск полива_1");
      Setup_Watering(1, watering_timer);
    }
    if (sms == "2" || sms == "p2" || sms == "P2")
    {
      outSMS = "Zapusk poliva vecher";
      sendSMS(msgphone, outSMS);
      // Serial.println("Запуск полива_2");
      Setup_Watering(2, watering_timer);
    }
    //+++++++++Проветривание+++++++++++++
    if (sms == "O" || sms == "o")
    {
      // Serial.println("Открытие окна");
    }
    //+++++++++Температура внутри+++++++++++++
    if (sms == "Ti" || sms == "ti" || sms == "TI")
    {
      outSMS = ("Temp: " + String(temp_IN) + " Humidity: " + String(humidity_IN));
      // Serial.println("Температура и влажность внутри");
      Serial.println(outSMS);
      sendSMS(msgphone, outSMS);
    }
    //+++++++++Температура снаружи++++++++++++
    if (sms == "To" || sms == "to" || sms == "TO")
    {
      outSMS = ("Temp out: " + String(temp_out_DS1820));
      // Serial.print("Температура снаружи: "); Serial.println(temp_out_DS1820);
      Serial.println(outSMS);
      sendSMS(msgphone, outSMS);
    }
    if (sms == "5" || sms == "U800")
  {
    //Serial.println("Запрос напряжения!");
    _response = sendATCommand("AT+CBC", true);
    sendSMS(msgphone, _response); 
  }
  //+++++++++Качество сигнала+++++++++++++  
  if (sms == "3" || sms == "s" || sms == "S")
  {
    _response = sendATCommand("AT+CSQ", true);
    sendSMS(msgphone, _response);
  }
  }
  
}
//-----------------------------------------------------------------------------------

//*************************Инициализация SIM800***************************
void SIMinit()
{
  // Serial.println("SIMInit>>>>>");
  sendATCommand("AT", true);                                  // Отправили AT для настройки скорости обмена данными
  sendATCommand("AT+CMGDA=\"DEL ALL\"", true);               // Удаляем все SMS, чтобы не забивать память
  _response = sendATCommand("AT+CLIP=1", true);             // Включаем АОН
  _response = sendATCommand("AT+DDET=1", true);             // Включаем DTMF
  sendATCommand("AT+CMGF=1;&W", true);                        // Включаем текстовый режима SMS (Text mode) и сразу сохраняем значение (AT&W)!
}
//-----------------------------------------------------------------------------------


//*************************Включение полива***************************
void Setup_Watering(uint8_t num_valve, uint8_t timer){
  uint8_t start_min = min_now;
  uint8_t start_sec = sec_now;
  // Serial.print("Num pin valve: ");Serial.println(mas_pins_valve[num_valve]);
  // Serial.print("Timer: ");Serial.println(timer);
  digitalWrite(mas_pins_valve[num_valve], HIGH);
  while((min_now - start_min) < timer){
    // Serial.println((min_now - start));
    // Serial.println(min_now);
    // Serial.println(start);
    ReadTime();
    
  }
  digitalWrite(mas_pins_valve[num_valve], LOW);
  outSMS = ("Poliv" + String(num_valve) + " zavershen: " + String(hour_now) + ":" + String(min_now));
      Serial.print("Полив Завершен");
      Serial.println(outSMS);
      sendSMS("+79133731599", outSMS);
}

void Check_Period_Watering (){
  if (watering_off == false){
      if (watering_time_1 == hour_now && watering_1 == false){
        Setup_Watering(1,watering_timer);
        watering_1 = true;
      }
    if (watering_time_2 == hour_now && watering_2 == false){
        Setup_Watering(1,watering_timer);
        watering_2 = true;
      }
    
  }
}

void setup() {
  //-------------------Выходы сброса модулей---------------------
  pinMode(mas_pins_valve[1], OUTPUT);                   //PIN реле 1>клапана1
  pinMode(mas_pins_valve[2], OUTPUT);                   //PIN реле 2>клапана2
  pinMode(mas_pins_valve[3], OUTPUT);                   //PIN реле 3
  pinMode(mas_pins_valve[4], OUTPUT);                   //PIN реле 4
  
  delay(50);

  Serial.begin(9600);
  delay(200);
  SIM800.begin(9600);
  dht.begin();
  SIMinit();
}

void loop() {

  if ((millis() - timer_1)>10000){
    timer_1 = millis();
    ReadTime();
  }
 if ((millis() - timer_2)>60000){
     timer_2 = millis();
     GetTempDS1820();
     GetTempIN();
     CheckSMS();
     Check_Period_Watering();
 }
 if (hour_now == 0 && min_now == 0){
     watering_1 = false;
     watering_1 = false;
 }
   
}
