#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

// TODO 
#define PIN_VALVE_RELAY = 2;

int hour = 0, min = 0, sec = 0;

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

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // wait for serial
  delay(200);
 
}

void loop() {
  
}



