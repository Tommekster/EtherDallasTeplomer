
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
}

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
