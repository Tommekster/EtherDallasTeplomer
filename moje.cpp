#include <Arduino.h>
#include <OneWire.h>
#include "moje.h"

// Rele
Rele::Rele(int pin, boolean _reverse = false):rele_pin(pin){
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

void parseLong(char *str, unsigned long &num){
  unsigned int index=0;
  
  num=0;
  while(str[index]<='9' && str[index]>='0'){
    num*=10;
    num+=str[index]-'0';
    
    index++;
  }
}

void readSens(int* sens){
  analogReference(DEFAULT);
  sens[0] = analogRead(A0);
  sens[1] = analogRead(A1);
  sens[2] = analogRead(A2);
  sens[3] = analogRead(A3);
  sens[4] = analogRead(A4);
  sens[5] = analogRead(A5);
  sens[6] = analogRead(A6);
  
  analogReference(INTERNAL);
  delay(100);
  sens[7] = analogRead(A6);
  analogReference(DEFAULT);
}

AnalogIN::AnalogIN(unsigned int *_sensbuf){
  // nastavi ukazatel
  sensbuf=_sensbuf;
  // pripravi cron
  cron_next=0;
  cron_last=0;
  cron_runs=0;
  
  // vymaze obsah
  for(int i=0; i<SENSBUF_LEN; i++){
    sensbuf[i]=0;
  }
}
void AnalogIN::_posun_index(){
  index++;
  index%=AVG_LEN;
}
void AnalogIN::read5V(){
  sensbuf[0*AVG_LEN+index] = analogRead(A0);
  sensbuf[1*AVG_LEN+index] = analogRead(A1);
  sensbuf[2*AVG_LEN+index] = analogRead(A2);
  sensbuf[3*AVG_LEN+index] = analogRead(A3);
  sensbuf[4*AVG_LEN+index] = analogRead(A4);
  sensbuf[5*AVG_LEN+index] = analogRead(A5);
  sensbuf[7*AVG_LEN] = analogRead(A6);    // vol
  
  analogReference(INTERNAL);
  while(analogRead(A6)>600) delay(100);
  //analogRead(A6);    // vol
  //analogRead(A6);    // vol
  delay(100);
  sensbuf[6*AVG_LEN+index] = analogRead(A6);    // vol
  
  analogReference(DEFAULT);
  analogRead(A6);    // vol
  
  _posun_index();
}
unsigned int AnalogIN::get(int i){
  
  if(i<6 || i==7){
    unsigned int sum;
    
    if(i==7) i=6;
    Serial.println(i);
    sum=0;
    for(int j=0; j<AVG_LEN; j++) sum+=sensbuf[i*AVG_LEN+j];
    
    return sum/AVG_LEN;
  }else{
    return sensbuf[7*AVG_LEN];
  }
}
void AnalogIN::initDiv(){
  
  analogReference(DEFAULT);
  delay(500);
  sensbuf[24] = analogRead(A6);    // vol
  delay(500);
  sensbuf[24] = analogRead(A6);    // vol
  
  analogReference(INTERNAL);
  delay(500);
  sensbuf[25] = analogRead(A6);    // vol
  delay(500);
  sensbuf[25] = analogRead(A6);    // vol
  
  analogReference(DEFAULT);
  analogRead(A6);    // vol
}
void AnalogIN::cron(){
  unsigned long _now=millis();
  
  if(_now > cron_next || _now < cron_last){
    read5V();
    //if((cron_runs & 1)==0) initDiv();
    
    cron_runs++;
    cron_last=_now;
    cron_next=_now+MY_CRON_INTERVALms;
  }
}

