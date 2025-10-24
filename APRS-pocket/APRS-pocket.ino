/*
Génère un signal APRS en FSK 1200 baud sur la sortie DAC de l'ATTINY1614
en fonction du GPS  
*/

#include "TinyGPSMinus.h"
#include <EEPROM.h>
#include "Position.h"
#include "Ax25.h"
#include "Smart.h"

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
} configuration;


TinyGPSMinus *gps;

configuration *config;

Fsk *leFsk;
Ax25 *ax25;
Position *pos;
Smart *smartBeacon;

byte second_prec = 0;

int main() {
  Serial.begin(GPSBAUD);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED, OUTPUT);
  digitalWrite(LED,LOW);
  
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

  gps = new TinyGPSMinus(*leFsk);

  smartBeacon = new Smart();
  bool flag = smartBeacon->isTxing();
  //testSmart();


  //pos->setLatitudeStr("4753.42N");  //pour test fixe
  //pos->setLongitudeStr("00016.28E");
 
  
  while (1) {
    syncGps();
    //txing();
    //leFsk->delay(5000);
  }

}

void syncGps() {
  bool newData = false;
  char time[9];
  // For one second we parse GPS data and report some key values

  while (Serial.available()) {
    char c = Serial.read();
    // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
    if (c == '#') {
      saveConfig();
    }
    if (gps->encode(c)) // Did a new valid sentence come in?
      newData = true;
  }

  if (newData) {
    int year;
    byte month;
    byte day;
    byte hour;
    byte minute;
    byte second;
    byte hundredths;
    unsigned long age;
    gps->crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
    if (second != second_prec) {
      sprintf(time, "%02d:%02d:%02d", hour, minute, second);  //doutes sur le sprintf
      Serial.println(time);
      Serial.flush();
      second_prec = second;

      gps->get_pos_age(&age);
      if (age != TinyGPSMinus::GPS_INVALID_AGE) {
        float lat=gps->get_latitudeDeg();
        float lon=gps->get_longitudeDeg();
        pos->setLatitude(lat);
        pos->setLongitude(lon);
        pos->setLatitudeStr(gps->get_latitude());
        pos->setLongitudeStr(gps->get_longitude());
        if (!config->smart) {                     //attention pour test smart enlever le ! après le test
          int course=(int) gps->f_course();
          int speed=(int) gps->f_speed_kmph();
          smartBeacon->setMobilityType(RUNNER);
          smartBeacon->setMillis(leFsk->millis());
          smartBeacon->setSpeed(speed);
          smartBeacon->setCourse(course);
          smartBeacon->setLongitude(lon);
          smartBeacon->setLatitude(lat);                    
          if (smartBeacon->isTxing()) {
            txing();
          }
        }
        else {
          //if ((minute % config->minute == 0) && (second==0)) {
          if (second % config->minute == 0) { //provisoire pour aller plus vite
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

  digitalWrite(LED, HIGH);   // turn the LED on ptt on
  Serial.println(pos->getPduAprs(config->compressed));
  Serial.flush();
  ax25->txMessage(pos->getPduAprs(config->compressed));

  digitalWrite(LED, LOW);    // turn the LED off ptt off
}




void saveConfig() {
  Serial.println("config rec");
  unsigned long startTime = leFsk->millis();
  while (Serial.available() < sizeof(configuration)) {
    if (leFsk->millis() - startTime > 2000) {
      Serial.println("⏱️ Timeout : données incomplètes reçues.");
      return;
    }
  }

  size_t n = Serial.readBytes((char*)config, sizeof(configuration));

  if (n == sizeof(configuration)) {
    EEPROM.put(0, *config); //sauve
    Serial.write('#'); //ack non traité dans le web JS
    afficheConfig();
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
}

/*
void testSmart() {
  Serial.println("\n========== DÉBUT DES TESTS SMARTBEACON ==========\n");

  struct TestCase {
    const char* titre;
    MobilityType type;
    unsigned long millis;
    float speed;
    int course;
    float lat;
    float lon;
  };

  TestCase tests[] = {
    {"Test 1 : RUNNER, vitesse lente, statique", RUNNER, 0, 2.0, 90, 47.890000, 0.270000},
    {"Test 2 : Changement de cap > seuil", RUNNER, 10000, 2.0, 200, 47.890000, 0.270000},
    {"Test 3 : Déplacement > minTxDist", RUNNER, 20000, 2.0, 200, 47.891000, 0.271500},
    {"Test 4 : CAR, vitesse > fastSpeed", CAR, 30000, 85.0, 270, 47.892000, 0.273000},
    {"Test 5 : Délai > fastRate", CAR, 120000, 85.0, 270, 47.892500, 0.273500},
    {"Test 6 : BIKE, vitesse intermédiaire", BIKE, 180000, 25.0, 270, 47.893000, 0.274000}
  };

  for (int i = 0; i < sizeof(tests) / sizeof(TestCase); i++) {
    Serial.println("--------------------------------------------------");
    Serial.println(tests[i].titre);
    smartBeacon->setMobilityType(tests[i].type);
    smartBeacon->setMillis(tests[i].millis);
    smartBeacon->setSpeed(tests[i].speed);
    smartBeacon->setCourse(tests[i].course);
    smartBeacon->setLatitude(tests[i].lat);
    smartBeacon->setLongitude(tests[i].lon);

    Serial.print("🕒 Temps : ");
    Serial.print(tests[i].millis / 1000);
    Serial.println("s");

    Serial.print("🚴 Vitesse : ");
    Serial.print(tests[i].speed);
    Serial.println(" km/h");

    Serial.print("🧭 Cap : ");
    Serial.print(tests[i].course);
    Serial.println("°");

    Serial.print("📍 Position : ");
    Serial.print(tests[i].lat, 6);
    Serial.print(", ");
    Serial.println(tests[i].lon, 6);

    bool tx = smartBeacon->isTxing();
    Serial.print("📡 Résultat : ");
    Serial.println(tx ? "✅ Émission déclenchée" : "❌ Pas d'émission");
    Serial.println("--------------------------------------------------\n");
    delay(500); // Pour lisibilité dans le moniteur série
  }

  Serial.println("========== FIN DES TESTS SMARTBEACON ==========\n");
}
*/


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
