#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <DallasTemperature.h>

// TODO выставить корректый PIN подключения реле
#define PIN_VALVE_RELAY = 2;
// Номер пина Arduino с подключенным датчиком
#define PIN_DS18B20 8;

int hour = 0, min = 0, sec = 0;

// Создаем объект OneWire
OneWire oneWire(PIN_DS18B20);
// Создаем объект DallasTemperature для работы с сенсорами, передавая ему ссылку на объект для работы с 1-Wire.
DallasTemperature dallasSensors(&amp;oneWire);
// Специальный объект для хранения адреса устройства
DeviceAddress sensorAddress;

//Значение температуры на DS1820
float temp_out_DS1820;

//Таймеры
uin32_t timer_1 = 0;

//__________________________Функция получения времени от Tiny RTC_____________________________________
void ReadTime(){
	 tmElements_t tm;
	 if (RTC.read(tm)) {
		hour = tm.Hour;
		min = tm.Minute;
		sec = tm.Second;
	 } else {
		if (RTC.chipPresent()) {
		  Serial.println("The DS1307 is stopped.  Please run the SetTime");
		  Serial.println();
		} else {
		  Serial.println("DS1307 read error!  Please check the circuitry.");
		  Serial.println();
		}
		Serial.print(hour);
		Serial.print(".");
		Serial.print(min);
		Serial.print(".");
		Serial.print(sec);
}
//___________________________________________________________________________________________________

//_____________________________Добавление нуля ко времени__________________________________________
void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}
//___________________________________________________________________________________________________


// ____________________Вспомогательная функция для отображения адреса датчика ds18b20__________________
void printAddress(DeviceAddress deviceAddress){
	for (uint8_t i = 0; i < 8; i++)
	{
		if (deviceAddress[i] < 16) Serial.print("0");
		Serial.print(deviceAddress[i], HEX);
	}
}
//____________________________________________________________________________________________________

//______________________Получение температуры на улице________________________________
void GetTempDS1820 () {
	temp_out_DS1820 = dallasSensors.getTempC(deviceAddress);
	Serial.print("Temp C: ");
	Serial.println(temp_out_DS1820);
}
//___________________________________________________________________________________

void setup() {
	Serial.begin(9600);
	while (!Serial) ; // wait for serial
	delay(200);
 
}

void loop() {

	if ((millis - timer_1)>1000){
		timer_1 = millis;
		GetTempDS1820();
		ReadTime();
	}

  
}



