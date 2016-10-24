// EtherShield webserver demo
#include "EtherShield.h"
#include <stdio.h> // sprintf
#include <Time.h>  // day, hour, minute, ... // useless
#include <OneWire.h>
#include "moje.h"

/*
http://www.instructables.com/id/Arduino-TempHumidity-with-LCD-and-Web-Interface/?ALLSTEPS
VCC to Arduino Pin 3.3V
GND to Arduino Pin GND
CS to Arduino Digital Pin 10
SI to Arduino Digital Pin 11
SO to Arduino Digital Pin 12
SCK to Arduino Digital Pin 13
*/

#define DS1820  4
#define RELE0   6
#define RELE1   7

// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = {
  0x54,0x55,0x58,0x10,0x00,0x25}; 
  
static uint8_t myip[4] = {
  192,168,11,99};

#define MYWWWPORT 80
#define BUFFER_SIZE 550
#define ES_MAX_LAST_TIMEms  200000

// Relays and tempSens
Rele rele0(RELE0);
Rele rele1(RELE1);
Teplomer tempSen(DS1820);

static uint8_t buf[BUFFER_SIZE+1];
static char myBuff[11];

// The ethernet shield
EtherShield es=EtherShield();
void es_init(){
  // Initialise SPI interface
  es.ES_enc28j60SpiInit();

  // initialize enc28j60
  es.ES_enc28j60Init(mymac);

  // init the ethernet/ip layer:
  es.ES_init_ip_arp_udp_tcp(mymac,myip, MYWWWPORT);
}

uint16_t http200ok(void)
{
  return(es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}

uint16_t prepareTemperature(uint8_t *buf, uint16_t plen){
  sprintf(myBuff,"%5.2dC",tempSen.getTemperature());
  plen=es.ES_fill_tcp_data(buf,plen,myBuff);
}
// prepare the webpage by writing the data to the tcp send buffer
uint16_t print_webpage(uint8_t *buf) // vlidna tvar 
{
  uint16_t plen;
  plen=http200ok();
  // Header
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<html><head><title>Kotelnik V1.3</title></head><body><table>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<tr><th>RELE0</th><th>RELE1</th><th>Dallas</th></tr>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<tr><td><a href=/rele0/on>ON</a></td><td><a href=/rele1/on>ON</a></td><td></td></tr>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<tr><td><a href=/rele0/off>OFF</a></td><td><a href=/rele1/off>OFF</a></td><td></td></tr>"));
  // RELEs, Dallas status...
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<tr><td>"));
  if(rele0.state())
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("isOn"));
  else
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("isOff"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</td><td>"));
  if(rele1.state())
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("isOn"));
  else
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("isOff"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</td><td>"));
  plen=prepareTemperature(buf,plen);
  // Footer
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</td></tr></table><br><hr>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("V1.3 <a href=\"https://github.com/Tommekster/EtherDallasTeplomer\">github.com/Tommekster/EtherDallasTeplomer</a>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</body></html>"));

  return(plen);
}
uint16_t print_toindex(uint8_t *buf) // Redirecs 
{
  uint16_t plen;
  plen=http200ok();
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<html><head><title>Kotelnik Done</title> <meta http-equiv=\"refresh\" content=\"1;url=/\"></head><body>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</body></html>"));

  return(plen);
}
uint16_t print_sens(uint8_t *buf) 
{
  uint16_t plen;
  plen=http200ok();
  plen=prepareTemperature(buf,plen);
  return(plen);
}


void setup(){
  es_init();
  tempSen.DSsearch(); // I hope it finds at least 1st sensor
  Serial.begin(9600);
}

void loop(){
  uint16_t plen, dat_p;
  unsigned long timer1;
  int state = 0;
  unsigned long last_tcp_time;

  timer1 = millis(); // reset "timer"
  last_tcp_time = millis();
  while(1) {
    
    // read packet, handle ping and wait for a tcp packet:
    dat_p=es.ES_packetloop_icmp_tcp(buf,es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf));

    /* dat_p will be unequal to zero if there is a valid 
     * http get */
    if(dat_p==0){
      // no http request; It has time for own work, anyone wants nothing
      // Measure temperature in to steps (1. prepare, 2.read)
      switch(state){
        case 0: // begin temperature measuring
          // Start measure temperature
          tempSen.startConversion();
          Serial.println("Spoustim mereni");
          timer1 = millis(); // reset "timer"
          state++;
          break;
        case 1: // wait: // maybe 750ms is enough, maybe not
          if((millis() - timer1) > 1000) state++;
          break;
        case 2:
          tempSen.readData();
          Serial.println("Ctu mereni");
          state++;
          break;
        default:
          state = 0;
      }
      
      if((millis()-last_tcp_time) > ES_MAX_LAST_TIMEms){
        // Co se stane, kdyz nebude dlouho pod dozorem?
      }
      
      continue;
    }
    Serial.println("Packet");
    last_tcp_time=millis();
    // tcp port 80 begin
    if (strncmp("GET ",(char *)&(buf[dat_p]),4)!=0){
      // head, post and other methods:
      dat_p=http200ok();
      dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("<h1>200 OK</h1>"));
      goto SENDTCP;
    }
    // just one web page in the "root directory" of the web server
    if (strncmp("/ ", (char *)&(buf[dat_p+4]), 2) == 0){
      dat_p=print_webpage(buf);
      goto SENDTCP;
    }
    // just temperature
    else if (strncmp("/temp ", (char *)&(buf[dat_p+4]), 6) == 0){
      dat_p=print_sens(buf);
      goto SENDTCP;
    }
    // control RELAYS
    else if (strncmp("/rele", (char *)&(buf[dat_p+4]), 5)==0){
      // Whitch Relay is it going to control
      Rele *relay;
      switch(buf[dat_p+4+5]){ // relay number
        case '0':
          relay = &rele0;
          break;
        case '1':
          relay = &rele1;
          break;
        default:
          goto BADREQUEST;
      }
      // What is it going to do? (on/off)
      if(strncmp("/on ", (char *)&(buf[dat_p+4+6]), 4)==0) relay->on();
      else if(strncmp("/off ", (char *)&(buf[dat_p+4+6]), 5)==0) relay->off();
      else if(strncmp("/toggle ", (char *)&(buf[dat_p+4+6]), 8)==0) relay->toggle();
      else goto BADREQUEST;
      dat_p=print_toindex(buf);
      goto SENDTCP;
    }
    else{
      dat_p=es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>"));
      goto SENDTCP;
    }
BADREQUEST:
    dat_p=es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 400 Bad Request\r\nContent-Type: text/html\r\n\r\n<h1>400 Bad Request</h1>"));
SENDTCP:
    es.ES_www_server_reply(buf,dat_p); // send web page data
    // tcp port 80 end

  }

}
