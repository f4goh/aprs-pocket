/*
   https://github.com/panaaj/nmeasimulator/releases
*/

#ifndef SMART_H
#define SMART_H
#include <math.h>
#include <Arduino.h>



enum MobilityType {
  RUNNER = 0,
  BIKE = 1,
  CAR = 2
};

struct SmartBeaconValues{
  int highSpeed;       // Vitesse élevée (en km/h) : au-dessus de cette valeur, on transmet à intervalle minInterval
  int lowSpeed;        // Vitesse basse (en km/h) : en dessous de cette valeur, on transmet à intervalle slowInterval
  int slowInterval;    // Intervalle de transmission (en secondes) en mode lent (vitesse < lowSpeed)
  int maxInterval;     // Intervalle maximum autorisé (en secondes) entre deux trames, quelle que soit la vitesse
  int minInterval;     // Intervalle minimum (en secondes) entre deux trames en mode rapide (vitesse > highSpeed)
  int minAngle;        // Angle minimum (en degrés) de changement de cap pour déclencher une trame (utile pour détecter les virages)
};

class Smart {
  public:
    Smart();

    void setCourse(int degrees);         // 0–359
    void setSpeed(int kmh);           // km/h
    void setLatitude(float lat);        // degrés décimaux
    void setLongitude(float lon);       // degrés décimaux
    void setMillis(unsigned long ms);   // millis Arduino
    void setMobilityType(MobilityType type); // Choix du profil
    void incSecond();
    bool isTxing();                     // retourne true si émission nécessaire

  private:
    // Valeurs actuelles
    int course;
    int speed;
    float latitude;
    float longitude;
    unsigned long currentMillis;


    // Valeurs précédentes
    int prevCourse;
    float prevLatitude;
    float prevLongitude;
    unsigned long lastMillis;
    unsigned int timeElapsed;
    int deltaCourse;

    // Paramètres dynamiques
    SmartBeaconValues currentSettings;

    // Méthodes internes
    float calculateDistance();
    bool shouldTransmit();
    void logStatus(const char* reason);
};

#endif

/*
  https://github.com/billygr/arduino-aprs-tracker/blob/master/arduino-aprs-tracker/arduino-aprs-tracker.ino
  https://github.com/richonguzman/LoRa_APRS_Tracker/blob/main/src/smartbeacon_utils.cpp
  http://n3ujj.com/manuals/SmartBeaconing.pdf

   Algorithme SmartBeaconing APRS
  ==============================

  Introduction
  ------------
  Le SmartBeaconing est une méthode intelligente de gestion des transmissions dans le système APRS (Automatic Packet Reporting System). Elle ajuste dynamiquement la fréquence d’émission des balises selon la vitesse de déplacement et les changements de direction, afin d’optimiser la précision tout en réduisant la charge réseau.

  Paramètres clés
  ---------------
  Paramètre       : Description
  --------------- : ------------------------------------------------------------
  Slow Rate       : Intervalle d’émission à faible vitesse ou à l’arrêt
  Fast Rate       : Intervalle d’émission à vitesse élevée
  Turn Angle      : Angle minimum de changement de direction pour déclencher une balise
  Min TX Time     : Temps minimum entre deux transmissions pour éviter la saturation

  Profils de mobilité
  -------------------
  Profil   | Vitesse basse | Vitesse haute | Slow Rate | Fast Rate | Turn Angle | Min TX Time
  -------- | ------------- | ------------- | ----------| ----------| -----------| ------------
  Voiture  | 10–15 km/h    | 50–70 km/h    | 30 min    | 3 min     | 30°        | 15–20 s
  Vélo     | 5–8 km/h      | 20–25 km/h    | 10 min    | 1–2 min   | 25–30°     | 15–20 s
  Piéton   | 2–5 km/h      | 7–10 km/h     | 10–15 min | 20–30 s   | 20–30°     | 10–15 s

  Logique de l’algorithme (pseudo-code)
  -------------------------------------
  Si vitesse < seuil_basse:
    intervalle = SlowRate
  Sinon si vitesse > seuil_haute:
    intervalle = FastRate
  Sinon:
    intervalle = valeur_intermédiaire

  Si angle_de_virage > TurnAngle ET temps_depuis_dernière_balise > MinTXTime:
    émettre_balise

  Si temps_depuis_dernière_balise > intervalle:
    émettre_balise

  Source
  ------
  https://franceaprs.net/doku.php?id=aide:smartbeaconing
*/
