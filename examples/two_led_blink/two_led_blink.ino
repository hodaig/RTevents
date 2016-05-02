
#include <MsTimer2.h>
#include <RTevents.h>

#define LED_PIN1 13
#define LED_PIN2 2

void flash1() {
  static boolean output = HIGH;
  
  digitalWrite(LED_PIN1, output);
  output = !output;
  
}

void flash2() {
  static boolean output = HIGH;
  
  digitalWrite(LED_PIN2, output);
  output = !output;
}

void setup(){
    pinMode(LED_PIN1, OUTPUT);
    pinMode(LED_PIN2, OUTPUT);
    
    RTevents::begin();

    RTevents::addTask(&flash1, 0, 500);
    RTevents::addTask(&flash2, 0, 50);
    
}

void loop(){
  // do some long task
  delay(1000);
}

