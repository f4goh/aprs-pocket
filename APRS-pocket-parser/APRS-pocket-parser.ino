/*
  Génère un signal APRS en FSK 1200 baud sur la sortie DAC de l'ATTINY1614
  en fonction du GPS
  https://github.com/wb2osz/aprsspec
  Configuation via une page web  
*/
#include <Arduino.h>
#include <EEPROM.h>
#include "Position.h"
#include "Ax25.h"
#include "Smart.h"
#include "ParserNMEA.h"


#define BITRATE 1200
#define GPSBAUD 9600
#define LED 1

typedef struct {
  char callsign[10];
  char comment[31];
  char symbol;
  uint8_t minute;
  bool smart;
  bool compressed;
  bool altitude;
  MobilityType mobility;
} configuration;


ParserNMEA gps;

configuration *config;

Fsk *leFsk;
Ax25 *ax25;
Position *pos;
Smart *smartBeacon;

byte second_prec = 0;
bool newData = false;

int main() {
  Serial.begin(GPSBAUD);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, 0);  // Horloge principale à 20 MHz

  config = new configuration;
  EEPROM.get(0, *config);
  afficheConfig();

  /*
    strcpy(config->callsign,"F4GOH-2");
    strcpy(config->comment,"Tiny");
    config->symbol='>';
    config->minute=10;
    config->smart=false;
    config->compressed=false;
    config->altitude=false;
    config->mobility=RUNNER;
    EEPROM.put(0, *config);
  */

  pos = new Position(47.890242, 0.276770, config->comment, '/', config->symbol);

  leFsk = new Fsk(1200, 1000);
  ax25 = new Ax25(*leFsk);

  leFsk->begin();
  char dstCallsign[] = "F4GOH  ";
  char path1[]       = "WIDE1-1";
  char path2[]       = "WIDE2-2";
  ax25->begin(BITRATE, config->callsign, dstCallsign, path1, path2);


  smartBeacon = new Smart();  


  while (1) {
    syncGps();
    //txing();
    //leFsk->delay(5000);
  }

}

void syncGps() {
  // For one second we parse GPS data and report some key values
  newData = false;
  while (Serial.available()) {
    char c = Serial.read();
    //Serial.write(c); // uncomment this line if you want to see the GPS data flowing
    if (c == '#') {
      saveConfig();
    }
    if (gps.encode(c)) // Did a new valid sentence come in?
      newData = true;
  }
  
  if (newData) {
    newData = false;
    if (gps.isTimeValid() && gps.getSecond() != second_prec) {
      second_prec = gps.getSecond();
      Serial.println(gps.getTime());

      if (gps.isCoordValid()) {
        float lat = gps.getLatDec();
        float lon = gps.getLongDec();
        pos->setLatitude(lat);
        pos->setLongitude(lon);
        if (gps.isAltValid()) pos->setAltitude(gps.getAltitudeMeters());
        if (config->smart) {                     //attention pour test smart enlever le ! après le test
          int course = gps.getCourse();
          int speed = gps.getSpeed();
          smartBeacon->setMobilityType(config->mobility);  //a deporter dans la config
          smartBeacon->setMillis(leFsk->millis());
          if (gps.isSpeedValid()) smartBeacon->setSpeed(speed);
          if (gps.isCourseValid()) smartBeacon->setCourse(course);
          if (smartBeacon->isTxing()) {
            txing();
          }
        }
        else {
          //if ((gps.getSecond() % config->minute == 0) && (gps.getSecond()==0)) {
          if (gps.getSecond() % config->minute == 0) { //provisoire pour aller plus vite  5 secondes trop court
            //Serial.println(lat,6);
            //Serial.println(lon,6);
            txing();
          }
        }
      }
    }
  }

}


void txing() {
  //Serial.println(gps->get_latitude());
  //Serial.println(gps->get_longitude());
  //Serial.println(gps->f_speed_kmph());
  //Serial.println(gps->course());
  const char* pdu;
  digitalWrite(LED, HIGH);   // turn the LED on ptt on
  pdu = pos->getPduAprs(config->compressed, config->altitude && gps.isAltValid());
  Serial.println(pdu);
  Serial.flush();
  ax25->txMessage(pdu);
  digitalWrite(LED, LOW);    // turn the LED off ptt off
}




void saveConfig() {
  Serial.println("config saved");
  //Serial.println(sizeof(configuration));
  unsigned long startTime = leFsk->millis();
  while (Serial.available() < sizeof(configuration)) {
    if (leFsk->millis() - startTime > 2000) {
      Serial.println(F("⏱Serial Timeout"));
      return;
    }
  }

  size_t n = Serial.readBytes((char*)config, sizeof(configuration));

  if (n == sizeof(configuration)) {
    EEPROM.put(0, *config); //sauve
    //afficheConfig();
    digitalWrite(LED, HIGH); //led PTT on
    leFsk->delay(1000);
    _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm); // Déclenche un reset logiciel pour prise en compte de la nouvelle config
  }

}


void afficheConfig()
{
  Serial.println(F("Configuration :"));
  Serial.print(F("Callsign: ")); Serial.println(config->callsign);
  Serial.print(F("Symbol: ")); Serial.println(config->symbol);
  Serial.print(F("Comment: ")); Serial.println(config->comment);
  Serial.print(F("Minute: ")); Serial.println(config->minute);
  Serial.print(F("SmartBeaconing: ")); Serial.println(config->smart ? "Yes" : "No");
  Serial.print(F("Compressed: ")); Serial.println(config->compressed ? "Yes" : "No");
  Serial.print(F("Altitude: ")); Serial.println(config->altitude ? "Yes" : "No");
  Serial.print(F("Mobility: "));
  switch (config->mobility) {
    case 0 :  Serial.println(F("RUNNER"));
      break;
    case 1 :  Serial.println(F("BIKE"));
      break;
    case 2 :  Serial.println(F("CAR"));
      break;
  }
}


/*
  //tempo sans irq
  void delay_s(byte sec) {
  for (byte s = 0; s < sec; s++) {
    for (uint32_t n = 0; n < 2800000UL; n++) {
      __asm__ __volatile__("nop");
    }
  }
  }
*/
