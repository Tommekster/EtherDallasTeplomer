#include <Arduino.h>
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

