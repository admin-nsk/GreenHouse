#include <Wire.h>					//Бибилотека для работы с Tiny RTC
#include <TimeLib.h>				//Бибилотека для работы с Tiny RTC
#include <DS1307RTC.h>				//Бибилотека для работы с Tiny RTC
#include <DallasTemperature.h>		//Бибилотека для работы с DS1820
#include <dhtnew.h>		//Бибилотека для работы с DHT11
#include <SoftwareSerial.h>         // Библиотека програмной реализации обмена по UART-протоколу


// TODO выставить корректый PIN подключения реле
#define PIN_VALVE_RELAY 2
// Номер пина Arduino с подключенным датчиком
#define PIN_DS18B20 8
// Номер пина Arduino с DHT11 
#define PIN_DHT11 4

// RX, TX GSM
SoftwareSerial SIM800(3, 2);                               

//****Переменные для работы с Tiny RTC
	int hour_now = 0, min_now = 0, sec_now = 0;
//*****

//******Переменные для работы с DS1820 
	// Создаем объект OneWire
	OneWire oneWire(PIN_DS18B20);
	// Создаем объект DallasTemperature для работы с сенсорами, передавая ему ссылку на объект для работы с 1-Wire.
	DallasTemperature dallasSensors(&oneWire);
	// Специальный объект для хранения адреса устройства
	DeviceAddress sensorAddress;
	//Значение температуры на DS1820
	float temp_out_DS1820;
//*****

//*************Настройка DHT11***************
	DHTNEW dhtSensor(PIN_DHT11);
	float temp_IN = 0;
	float humidity_IN = 0;


//Таймеры
	uint32_t timer_1 = 0, timer_2 = 0;

//***Переменнные для работы с SIM800
	String _response = "";    // Переменная для хранения ответа модуля
	String phones = "+79137857684, +79612183656, +79133731599, +79133886955";   // Белый список телефонов
	String msgphone;                        // Переменная для хранения номера отправителя
	String msgbody;                         // Переменная для хранения текста СМС
	String outSMS;
	bool hasmsg = false;                                              // Флаг наличия сообщений к удалению
//****

void(* resetFunc) (void) = 0;      //Функция перезагрузки

//__________________________Функция получения времени от Tiny RTC_____________________________________
void ReadTime(){
	 tmElements_t tm;
	 if (RTC.read(tm)) {
		hour_now = tm.Hour;
		min_now = tm.Minute;
		sec_now = tm.Second;
	 } else {
		if (RTC.chipPresent()) {
		  Serial.println("The DS1307 is stopped.  Please run the SetTime");
		  Serial.println();
		} else {
		  Serial.println("DS1307 read error!  Please check the circuitry.");
		  Serial.println();
		}
		Serial.print(hour_now);
		Serial.print(".");
		Serial.print(min_now);
		Serial.print(".");
		Serial.print(sec_now);
	}
}
//___________________________________________________________________________________________________

//_____________________________Добавление нуля ко времени__________________________________________
void Print2digits (int number) {
	if (number >= 0 && number < 10) {
		Serial.write('0');
	}
	Serial.print(number);
}
//___________________________________________________________________________________________________


// ____________________Вспомогательная функция для отображения адреса датчика ds18b20__________________
void PrintAdress (DeviceAddress deviceAddress){
	for (uint8_t i = 0; i < 8; i++)
	{
		if (deviceAddress[i] < 16) Serial.print("0");
		Serial.print(deviceAddress[i], HEX);
	}
}
//____________________________________________________________________________________________________

//______________________Получение температуры на улице________________________________
void GetTempDS1820(DeviceAddress deviceAddress) {
	temp_out_DS1820 = dallasSensors.getTempC(deviceAddress);
	Serial.print("Temp C: ");
	Serial.println(temp_out_DS1820);
}
//___________________________________________________________________________________

//*************************Получение температуры и влажности с DHT11***************************
void GetTempIN()
{
  temp_IN = dhtSensor.getTemperature();
  humidity_IN = dhtSensor.getHumidity();
 }
//-----------------------------------------------------------------------------------

//***************************Отправка команды модему******************************
String sendATCommand(String cmd, bool waiting) {
  SIM800.begin(9600);
  SIM800.listen();
  delay(50);
  String _resp = "";                                              // Переменная для хранения результата
  Serial.println("SendATCommand>>>>>");
  Serial.println(cmd);                                            // Дублируем команду в монитор порта
  SIM800.println(cmd);                                            // Отправляем команду модулю SIM
  if (waiting) {                                                  // Если необходимо дождаться ответа...
    _resp = waitResponse();                                       // ... ждем, когда будет передан ответ
    // Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать
    if (_resp.startsWith(cmd)) {                                  // Убираем из ответа дублирующуюся команду
      _resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
    }
    Serial.println(_resp);                                        // Дублируем ответ в монитор порта
  }
  SIM800.end();
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
  SIM800.begin(9600);
  SIM800.listen();
  Serial.println("CheckSMS>>>>>>>");
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
  SIM800.end();
}
//-----------------------------------------------------------------------------------

// *****************************Парсим SMS*******************************
String parseSMS(String msg) {                                   
  String msgheader  = "";
  String msgbody    = "";
  //String msgphone   = "";
  Serial.println("Parse!");
  msg = msg.substring(msg.indexOf("+CMGR: "));
  msgheader = msg.substring(0, msg.indexOf("\r"));            // Выдергиваем телефон

  msgbody = msg.substring(msgheader.length() + 2);
  msgbody = msgbody.substring(0, msgbody.lastIndexOf("OK"));  // Выдергиваем текст SMS
  msgbody.trim();

  int firstIndex = msgheader.indexOf("\",\"") + 3;
  int secondIndex = msgheader.indexOf("\",\"", firstIndex);
  msgphone = msgheader.substring(firstIndex, secondIndex);

  Serial.println("Phone: " + msgphone);                       // Выводим номер телефона
  Serial.println("Message: " + msgbody);                      // Выводим текст SMS

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
  Serial.println("SendSMS>>>>>");	
  SIM800.begin(9600);
  SIM800.listen();
  delay(100);
  Serial.println("sendSMS!   " + phone+"    " + message);
  sendATCommand("AT+CMGS=\"" + phone + "\"", true);             // Переходим в режим ввода текстового сообщения
  sendATCommand(message + "\r\n" + (String)((char)26), true);   // После текста отправляем перенос строки и Ctrl+Z
  SIM800.end();
}
//-----------------------------------------------------------------------------------



//*************************Функция обаработки запроса в СМС***************************
void SMSSelect(String sms){
  //Serial.println("SMSSelect!"); 
 
	//+++++++++Перезагрузка+++++++++++++
	if (sms == "0" || sms == "r" || sms == "R")
	{
		Serial.println("Reboot");
		String sSMS5 = ("Reboot!");
		sendSMS(msgphone, sSMS5);
		delay(15000);
		digitalWrite(6, LOW);
		digitalWrite(7, LOW);
		resetFunc();
	}

	//+++++++++Полив+++++++++++++
	if (sms == "1" || sms == "p" || sms == "P")
	{
		outSMS = "Запуск полива";
		sendSMS(msgphone, outSMS);
		Serial.println("Запуск полива");
	}
	//+++++++++Проветривание+++++++++++++
	if (sms == "O" || sms == "o")
	{
		Serial.println("Открытие окна");

	}
	//+++++++++Температура внутри+++++++++++++
	if (sms == "Ti" || sms == "ti" || sms == "TI")
	{
		Serial.println("Температура и влажность внутри");
	}
	//+++++++++Температура снаружи++++++++++++
	if (sms == "To" || sms == "to" || sms == "TO")
	{
		Serial.print("Температура снаружи: "); Serial.println(temp_out_DS1820);
	}
  
}
//-----------------------------------------------------------------------------------

//*************************Инициализация SIM800***************************
void SIMinit()
{
  Serial.println("SIMInit>>>>>");
  sendATCommand("AT", true);                                  // Отправили AT для настройки скорости обмена данными
  sendATCommand("AT+CMGDA=\"DEL ALL\"", true);               // Удаляем все SMS, чтобы не забивать память
  _response = sendATCommand("AT+CLIP=1", true);             // Включаем АОН
  _response = sendATCommand("AT+DDET=1", true);             // Включаем DTMF
  sendATCommand("AT+CMGF=1;&W", true);                        // Включаем текстовый режима SMS (Text mode) и сразу сохраняем значение (AT&W)!
}
//-----------------------------------------------------------------------------------


void setup() {
	//-------------------Выходы сброса модулей---------------------
  pinMode(6, OUTPUT);                   //PIN клапана
  pinMode(7, OUTPUT);                   //Сброс SIM800
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
  delay(50);

  dhtSensor.setType(11);

	Serial.begin(9600);
	while (!Serial) ; // wait for serial
	delay(200);
	SIMinit();
}

void loop() {

	if ((millis() - timer_1)>1000){
		timer_1 = millis();
		GetTempDS1820(sensorAddress);
		ReadTime();
	}
	if ((millis() - timer_2)>60000){
			timer_2 = millis();
			CheckSMS();
	}

  
}
