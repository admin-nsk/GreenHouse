0#include <Wire.h>          //Бибилотека для работы с Tiny RTC
#include <TimeLib.h>        //Бибилотека для работы с Tiny RTC
#include <DS1307RTC.h>        //Бибилотека для работы с Tiny RTC
#include <OneWire.h>    //Бибилотека для работы с DS1820
#include "DHT.h"    //Бибилотека для работы с DHT11
#include <SoftwareSerial.h>         // Библиотека програмной реализации обмена по UART-протоколу

// RX, TX GSM
#define RX_SIM800 2
#define TX_SIM800 3
// Номер пина Arduino с DHT11 
#define DHT_PIN 5
#define DHT_TYPE DHT11
// Номер пина Arduino с подключенным датчиком
#define PIN_DS18B20 6

SoftwareSerial SIM800(RX_SIM800, TX_SIM800);                               

//****Переменные для работы с Tiny RTC
int wday_now = 0, month_now = 0, day_now = 0, hour_now = 0, min_now = 0, sec_now = 0;

//******Настройка работы с DS1820 - температура снаружи*************
OneWire DS1820(PIN_DS18B20);
//Значение температуры на DS1820
float temp_out = 0;

//*************Настройка DHT11 - температура внутри***************
DHT dht(DHT_PIN, DHT_TYPE);
float temp_in = 0;
float humidity_in = 0;


//Таймеры для loop
uint32_t timer_1 = 0, timer_2 = 0;

//***Переменнные парсинга СМС
String ALLOW_PHONES = "+79137777777, +79666666666, +79133333333";   // Белый список телефонов
String phone_sender;                        // Переменная для хранения номера отправителя
String text_sms;
bool flgHasMsg = false;                                              // Флаг наличия сообщений к удалению

//****Данные для полива*********
int PINS_VALVE[5] = {0, 7, 8, 9, 10};
int WATERING_TIME = 10;

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
void GetTempOUT() {
// Определяем температуру от датчика DS18b20
  byte _data_temp[2]; // Место для значения температуры
  
  DS1820.reset(); // Начинаем взаимодействие со сброса всех предыдущих команд и параметров
  DS1820.write(0xCC); // Даем датчику DS18b20 команду пропустить поиск по адресу. В нашем случае только одно устрйоство 
  DS1820.write(0x44); // Даем датчику DS18b20 команду измерить температуру. Само значение температуры мы еще не получаем - датчик его положит во внутреннюю память
  
  delay(1000); // Микросхема измеряет температуру, а мы ждем.  
  
  DS1820.reset(); // Теперь готовимся получить значение измеренной температуры
  DS1820.write(0xCC); 
  DS1820.write(0xBE); // Просим передать нам значение регистров со значением температуры
  // Получаем и считываем ответ
  _data_temp[0] = DS1820.read(); // Читаем младший байт значения температуры
  _data_temp[1] = DS1820.read(); // А теперь старший
  // Формируем итоговое значение: 
  //    - сперва "склеиваем" значение, 
  //    - затем умножаем его на коэффициент, соответсвующий разрешающей способности (для 12 бит по умолчанию - это 0,0625)
  temp_out =  ((_data_temp[1] << 8) | _data_temp[0]) * 0.0625;
  
  // Выводим полученное значение температуры в монитор порта
  // Serial.print("Temp OUT: ");
  // Serial.println(temp_out);
}
//___________________________________________________________________________________

//*************************Получение температуры и влажности с DHT11***************************
void GetTempIN()
{
  // Serial.println("GetTempIN()>>>>>>>>>>>>>>");
  temp_in = dht.readTemperature();
  humidity_in = dht.readHumidity();
  // Serial.print("TempIN: ");Serial.println(temp_in);
  // Serial.print("HumidityIN: ");Serial.println(humidity_in);

 }
//-----------------------------------------------------------------------------------

//***************************Отправка команды модему******************************
String sendATCommand(String cmd, bool waiting) {
  SIM800.begin(9600);
  SIM800.listen();
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
    // Serial.println(_resp);                                        // Дублируем ответ в монитор порта
  }
  SIM800.end();
  return _resp;                                                   // Возвращаем результат. Пусто, если проблема
  
}
//-----------------------------------------------------------------------------------

//***************Функциsя ожидания ответа и возврата полученного результата***************
String waitResponse() {                                           // 
  String _resp = "";                                              // Переменная для хранения результата
  // Serial.println("Wait...");
  long _timeout = millis() + 10000;                               // Переменная для отслеживания таймаута (10 секунд)

  while (!SIM800.available() && millis() < _timeout)  {};         // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
    if (SIM800.available()) {                                       // Если есть, что считывать...
      _resp = SIM800.readString();                                  // ... считываем и запоминаем
      //  Serial.println(_resp);
    }
    else {                                                          // Если пришел таймаут, то...
      // Serial.println("Timeout...");                                 // ... оповещаем об этом и...
    }
  return _resp;                                                   // ... возвращаем результат. Пусто, если проблема
}
//-----------------------------------------------------------------------------------

//**************************Проверка новых СМС***************************************
void CheckSMS (){
  SIM800.begin(9600);
  SIM800.listen();
  String _response = "";    // Переменная для хранения ответа модуля
  // Serial.println("CheckSMS>>>>>>>");
  do {
    _response = sendATCommand("AT+CMGL=\"REC UNREAD\",1", true);// Отправляем запрос чтения непрочитанных сообщений
    if (_response.indexOf("+CMGL: ") > -1) {                    // Если есть хоть одно, получаем его индекс
      int msgIndex = _response.substring(_response.indexOf("+CMGL: ") + 7, _response.indexOf("\"REC UNREAD\"", _response.indexOf("+CMGL: ")) - 1).toInt();
      char i = 0;                                               // Объявляем счетчик попыток
      do {
        i++;                                                    // Увеличиваем счетчик
        _response = sendATCommand("AT+CMGR=" + (String)msgIndex + ",1", true);  // Пробуем получить текст SMS по индексу
        _response.trim();                                       // Убираем пробелы в начале/конце
        if (_response.endsWith("OK")) {                         // Если ответ заканчивается на "ОК"
          if (!flgHasMsg) flgHasMsg = true;                           // Ставим флаг наличия сообщений для удаления
            sendATCommand("AT+CMGR=" + (String)msgIndex, true);   // Делаем сообщение прочитанным
            sendATCommand("\n", true);                            // Перестраховка - вывод новой строки
            phone_sender = parseSMS(_response);                       // Отправляем текст сообщения на обработку
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
      if (flgHasMsg) {
        sendATCommand("AT+CMGDA=\"DEL READ\"", true);           // Удаляем все прочитанные сообщения
        flgHasMsg = false;
      }
      break;
    }

  } while (1);
  SIM800.end();
}
//-----------------------------------------------------------------------------------

// *****************************Парсим SMS*******************************
String parseSMS(String incoming_sms) {                                   
  String _sms_header = "";
  String _sms_body = "";
  //String msgphone   = "";
  // Serial.println("Parse!");
  incoming_sms = incoming_sms.substring(incoming_sms.indexOf("+CMGR: "));
  _sms_header = incoming_sms.substring(0, incoming_sms.indexOf("\r"));            // Выдергиваем телефон

  _sms_body = incoming_sms.substring(_sms_header.length() + 2);
  _sms_body = _sms_body.substring(0, _sms_body.lastIndexOf("OK"));  // Выдергиваем текст SMS
  _sms_body.trim();

  int _first_index = _sms_header.indexOf("\",\"") + 3;
  int _second_index = _sms_header.indexOf("\",\"", _first_index);
  phone_sender = _sms_header.substring(_first_index, _second_index);

  // Serial.println("Phone: " + phone_sender);                       // Выводим номер телефона
  // Serial.println("Message: " + _sms_body);                      // Выводим текст SMS

  if (phone_sender.length() > 6 && ALLOW_PHONES.indexOf(phone_sender) > -1) { // Если телефон в белом списке, то...
    //Serial.println("SMSSelectIN!");
    SMSSelect(_sms_body);                           
  }
  else {
    //Serial.println("Unknown phonenumber");
    }
  return phone_sender;
}
//-----------------------------------------------------------------------------------

//****************************отправка SMS**************************************
void sendSMS(String phone, String message)
{
  // Serial.println("SendSMS>>>>>"); 
  SIM800.begin(9600);
  SIM800.listen();
  delay(100);
  // Serial.println("sendSMS!   " + phone + "    " + message);
  sendATCommand("AT+CMGS=\"" + phone + "\"", true);             // Переходим в режим ввода текстового сообщения
  sendATCommand(message + "\r\n" + (String)((char)26), true);   // После текста отправляем перенос строки и Ctrl+Z
  SIM800.end();
}
//-----------------------------------------------------------------------------------



//*************************Функция обаработки запроса в СМС***************************
void SMSSelect(String command){
  //Serial.println("SMSSelect!"); 
 
  //+++++++++Перезагрузка+++++++++++++
  if (command == "0" || command == "r" || command == "R")
  {
    Serial.println("Reboot");
    text_sms = ("Reboot!");
    sendSMS(phone_sender, text_sms);
    delay(15000);
    digitalWrite(PIN_DS18B20, LOW);
    digitalWrite(DHT_PIN, LOW);
    resetFunc();
  }

  //+++++++++Полив+++++++++++++
  if (command == "1" || command == "p1" || command == "P1")
  {
    text_sms = "Запуск полива_1";
    sendSMS(phone_sender, text_sms);
    // Serial.println("Запуск полива_1");
    SetupWatering(1, WATERING_TIME);
  }
  if (command == "2" || command == "p2" || command == "P2")
  {
    text_sms = "Запуск полива_2";
    sendSMS(phone_sender, text_sms);
    // Serial.println("Запуск полива_2");
    SetupWatering(2, WATERING_TIME);
  }
  //+++++++++Проветривание+++++++++++++
  if (command == "O" || command == "o")
  {
    Serial.println("Открытие окна");
  }
  //+++++++++Температура внутри+++++++++++++
  if (command == "Ti" || command == "ti" || command == "TI")
  {
    text_sms = ("Temp: " + String(temp_in) + " Humidity: " + String(humidity_in));
    // Serial.print("Температура и влажность внутри");
    // Serial.println(text_sms);
    sendSMS(phone_sender, text_sms);
  }
  //+++++++++Температура снаружи++++++++++++
  if (sms == "To" || sms == "to" || sms == "TO")
  {
    text_sms = ("Temp out: " + String(temp_out));
    // Serial.print("Температура снаружи: "); Serial.println(temp_out);
    // Serial.println(text_sms);
    sendSMS(phone_sender, text_sms);
  }
  
}
//-----------------------------------------------------------------------------------

//*************************Инициализация SIM800***************************
void SIMinit()
{
  // Serial.println("SIMInit>>>>>");
  sendATCommand("AT", true);                                  // Отправили AT для настройки скорости обмена данными
  sendATCommand("AT+CMGDA=\"DEL ALL\"", true);               // Удаляем все SMS, чтобы не забивать память
  sendATCommand("AT+CLIP=1", true);             // Включаем АОН
  sendATCommand("AT+DDET=1", true);             // Включаем DTMF
  sendATCommand("AT+CMGF=1;&W", true);                        // Включаем текстовый режима SMS (Text mode) и сразу сохраняем значение (AT&W)!
}
//-----------------------------------------------------------------------------------


//*************************Включение полива***************************
void SetupWatering(int num_valve, int timer){
	int _start = min_now;
	// Serial.print("Num pin valve: ");Serial.println(PINS_VALVE[num_valve]);
	digitalWrite(PINS_VALVE[num_valve], HIGH);
	while((min_now - _start) < timer){
		delay(60000);
		ReadTime();
	}
	digitalWrite(PINS_VALVE[num_valve], LOW);
}

void setup() {
  //-------------------Выходы сброса модулей---------------------
  pinMode(PINS_VALVE[1], OUTPUT);                   //PIN реле 1>клапана1
  pinMode(PINS_VALVE[2], OUTPUT);                   //PIN реле 2>клапана2
  pinMode(PINS_VALVE[3], OUTPUT);                   //PIN реле 3
  pinMode(PINS_VALVE[4], OUTPUT);                   //PIN реле 4
  
  delay(50);

  Serial.begin(9600);
  delay(200);
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
    GetTempOUT();
    GetTempIN();
    CheckSMS();
 }
 if (hour_now == 20 && min_now == 00){
    SetupWatering(1, WATERING_TIME);
 }
 if (hour_now == 20 && min_now == 00){
    SetupWatering(1, WATERING_TIME);
 }

  
}
