#include <Arduino.h>
#include <OneWire.h>
#include "moje.h"

// Rele
Rele::Rele(int pin, boolean _reverse):rele_pin(pin){
  setReverse(_reverse);
  pinMode(rele_pin, OUTPUT);
  off();
}
void Rele::on(){
  if(reverse) digitalWrite(rele_pin,LOW);
  else digitalWrite(rele_pin,HIGH);
}
void Rele::off(){
  if(reverse) digitalWrite(rele_pin,HIGH);
  else digitalWrite(rele_pin,LOW);
}
void Rele::toggle(){
  if(state()) off();
  else on();
}
boolean Rele::state(){return digitalRead(rele_pin);}

// Dallas Temperature DS18x20
String Teplomer::componentName(){
  switch (addr[0]){
    case 0x10:
      return "DS18S20";
    case 0x28:
      return "DS18B20";
    case 0x22:
      return "DS1822";
    default:
      return "Unknown";
  }
}
void Teplomer::startConversion(){
  reset();
  select(addr);
  write(0x44,1); // start conversion, with parasite power on at the end
  depower();
}
void Teplomer::readData(){
  reset();
  select(addr);
  write(0xBE);  // read scratchpad
  tempValid = false;
  for(int i  = 0; i < 9; i++){
    data[i] = read();
  }
}
void Teplomer::computeTemperature(){
  int16_t raw = (data[1] << 8) | data[0];
  if(isType_s()){
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  //fahrenheit = celsius * 1.8 + 32.0;
  tempValid = true;
}

//#define MY_CRON_INTERVALms  90000/AVG_LEN
#define MY_CRON_INTERVALms  9000/AVG_LEN

