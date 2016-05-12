
#include <RTevents.h>

#define LED_PIN1 9
#define LED_PIN2 13

uint8_t t1;
uint8_t t2;

volatile uint32_t c1=0;
volatile uint32_t c2=0;

uint32_t t1_period = 10*1000L;
uint32_t t2_period = 10L;

uint32_t start;

void flash1() {
  delayMicroseconds(10);
  c1++;
}

void flash2() {
  delayMicroseconds(10);
  c2++;
}

void setup(){
    Serial.begin(9600);
    Serial.print("RTevent task size: ");
    Serial.println(sizeof(RTtask_t));
    Serial.println("starting test");
    
    pinMode(LED_PIN1, OUTPUT);
    pinMode(LED_PIN2, OUTPUT);
    delay(1);
    RTevents::begin();
    
    t1 = RTevents::addTask(&flash1, 0, t1_period);
    t2 = RTevents::addTask(&flash2, 0, t2_period);
    
    start = micros();
}

void loop(){
  
  //delay(1);
  
  uint32_t delta = (micros() - start);
  
  if (delta > 1*1000L*1000L){
      RTevents::removeTask(t1);
      RTevents::removeTask(t2);
      Serial.print("done after ");
      Serial.print(delta);
      Serial.println(" micros");
      
      Serial.print("c1=");
      Serial.print(c1);
      Serial.print(", avg period ");
      Serial.print(delta / c1);
      Serial.print(", expected ");
      Serial.println(t1_period);
      
      Serial.print("c2= ");
      Serial.print(c2);
      Serial.print(", avg period ");
      Serial.print(delta / c2);
      Serial.print(", expected ");
      Serial.println(t2_period);
      
      for(;;){};
      
  }
}

