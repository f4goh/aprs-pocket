#include "Smart.h"

SmartBeaconValues smartBeaconSettings[3] = {
  // 🏃 RUNNER
  {
    10,   // highSpeed : au-delà de 10 km/h → intervalle minimum
    3,    // lowSpeed  : en dessous de 3 km/h → intervalle lent
    60,  // slowInterval : 60 secondes entre trames à basse vitesse
    30,  // maxInterval  : 30 secondes max entre trames
    5,   // minInterval  : 5 secondes à haute vitesse
    25    // minAngle     : virage significatif à partir de 25°
  },

  // 🚴 BIKE
  {
    20,   // highSpeed : au-delà de 10 km/h → intervalle minimum
    8,    // lowSpeed  : en dessous de 3 km/h → intervalle lent
    60,  // slowInterval : 60 secondes entre trames à basse vitesse
    30,  // maxInterval  : 30 secondes max entre trames
    5,   // minInterval  : 5 secondes à haute vitesse
    25    // minAngle     : virage significatif à partir de 25°
  },

  // 🚗 CAR
  {
    60,   // highSpeed : au-delà de 10 km/h → intervalle minimum
    10,    // lowSpeed  : en dessous de 3 km/h → intervalle lent
    60,  // slowInterval : 60 secondes entre trames à basse vitesse
    30,  // maxInterval  : 30 secondes max entre trames
    5,   // minInterval  : 5 secondes à haute vitesse
    25    // minAngle     : virage significatif à partir de 25°
  }
};

Smart::Smart() {
  course = 0;
  prevCourse = 0;
  latitude = 0.0;
  prevLatitude = 0.0;
  longitude = 0;
  prevLongitude = 0.0;
  lastMillis = 0;
  timeElapsed = 0;
  setMobilityType(RUNNER); // Par défaut
}

void Smart::setCourse(int degrees) {
  deltaCourse = course - degrees;
  if (deltaCourse < 0) deltaCourse *= -1;
  prevCourse = course;
  course = degrees;
  //Serial.print(deltaCourse);
}

void Smart::setSpeed(int kmh) {
  speed = kmh;
}

void Smart::setLatitude(float lat) {
  latitude = lat;
}

void Smart::setLongitude(float lon) {
  longitude = lon;
}

void Smart::setMillis(unsigned long ms) {
  lastMillis = currentMillis;
  currentMillis = ms;
  timeElapsed += (currentMillis - lastMillis) / 1000;
}

void Smart::incSecond() {
  timeElapsed ++;
}

void Smart::setMobilityType(MobilityType type) {
  currentSettings = smartBeaconSettings[type];
}

bool Smart::isTxing() {
  if (shouldTransmit()) {
    timeElapsed = 0;
    prevLatitude = latitude;
    prevLongitude = longitude;
    return true;
  }
  return false;
}



bool Smart::shouldTransmit() {

  float deltaDistance = calculateDistance(); // en km

  //Serial.print("/");
  //Serial.println(deltaDistance);

  //virage significatif
  if (deltaCourse >= currentSettings.minAngle &&  deltaDistance > 0.02) { // 20m
    logStatus("TURN");
    return true;
  }

  //vitesse lente
  if (speed <= currentSettings.lowSpeed && timeElapsed >= currentSettings.slowInterval) {
    logStatus("SLOW");
    return true;
  }
  else {
    //vitesse rapide
    if (speed >= currentSettings.highSpeed && timeElapsed >= currentSettings.minInterval) {
      logStatus("FAST");
      return true;
    }
    else {
      if (timeElapsed >= currentSettings.maxInterval) {
        logStatus("MAX");
        return true;
      }
    }
  }

  logStatus("NO TX");
  return false;
}


float Smart::calculateDistance() {

  const float R = 6371.0; // Rayon terrestre en km
  float dLat = (latitude - prevLatitude) * M_PI / 180.0;
  float dLon = (longitude - prevLongitude) * M_PI / 180.0;

  float a = sin(dLat / 2) * sin(dLat / 2) +
            cos(prevLatitude * M_PI / 180.0) * cos(latitude * M_PI / 180.0) *
            sin(dLon / 2) * sin(dLon / 2);

  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}


void Smart::logStatus(const char* reason) {
  Serial.println(reason);
  /*
    Serial.print(F(" "));
    Serial.print(timeElapsed);
    Serial.print(F("s | Speed: "));
    Serial.print(speed);
    Serial.print(F(" km/h | Course: "));
    Serial.println(course);
  
  Serial.print(F("° | Lat: "));
  Serial.print(latitude, 6);
  Serial.print(F(" | Lon: "));
  Serial.println(longitude, 6);
  Serial.flush();
  */
}
