#include <Arduino.h>
#include "wdt.h"

void wdt_reset(){
  static boolean stat=false;
  
  if(stat)
    digitalWrite(WDOG,HIGH);
  else 
    digitalWrite(WDOG,LOW);
    
  stat=!stat;
}
void wdt_enable(){
  pinMode(WDOG, OUTPUT);
  wdt_reset();
}
