#ifndef MOJE_H
#define MOJE_H

#include <Arduino.h>
#include <OneWire.h>

#ifndef OneWire_h
#error OneWire chybi
#endif

#define AVG_LEN  8

//#define SENSBUF_LEN  AVG_LEN*7+1  // 4*6 + 2
#define SENSBUF_LEN  64  // 4*6 + 2

// Rele
class Rele{
  const int rele_pin;
  boolean reverse;  
public:
  Rele(int pin, boolean reverse = false);
  void on();
  void off();
  boolean state();
  void setReverse(boolean v){reverse = v;}
  boolean isReverse(){return reverse;}
};

// Dallas Temperature DS18x20
class OneWire;
class Teplomer : public OneWire
{
  byte addr[8];
  byte data[12];
  boolean tempValid;
  float celsius;
  
  boolean isType_s(){return addr[0]==0x10;}
  
public:
  Teplomer(int pin):OneWire(pin),tempValid(false){}
  boolean DSsearch(){return search(addr);}
  // reset_search();
  // delay(250);
  boolean isAddrValidCRC(){return OneWire::crc8(addr, 7) == addr[7];}
  String componentName();
  void startConversion();
  void readData();
  void computeTemperature();
  float getTemperature(){return celsius;}
};

class AnalogIN{
  private:
  unsigned int *sensbuf;
  unsigned long cron_next;
  unsigned long cron_last;
  unsigned char cron_runs;
  uint8_t  index;
  
  void _posun_index();
  
  public:
  AnalogIN(unsigned int *_sensbuf); // Constructor
  void read5V();
  void initDiv();
  unsigned int get(int i);
  void cron();
};

#endif
