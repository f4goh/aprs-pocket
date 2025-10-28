#include "ParserNMEA.h"

// Constructor
ParserNMEA::ParserNMEA() {
  index = 0;
  newData = false;
  timeValid = false;
  coordValid = false;
  speedValid = false;
  courseValid = false;
  altValid = false;
  latStr[0] = '\0';
  lonStr[0] = '\0';
  speedKnots = 0.0;
  courseDeg = 0.0;
  hour = minute = second = 0;
}

// Main parser
bool ParserNMEA::encode(char c) {
  if (c == '$') {
    index = 0;
    buffer[index++] = c;
    return false;
  }

  if (index < sizeof(buffer) - 1) {
    buffer[index++] = c;
    buffer[index] = '\0';
  }

  if (c == '\n' || c == '\r') {
    if (!verifyChecksum(buffer)) return false;

    if (strncmp(buffer + 3, "RMC", 3) == 0) {
      parseGPRMC(buffer);
      newData = true;
    } else if (strncmp(buffer + 3, "GGA", 3) == 0) {
      parseGPGGA(buffer);
      newData = true;
    }

    index = 0;
  }

  return newData;
}

// Checksum verification
bool ParserNMEA::verifyChecksum(const char* sentence) {
  if (sentence[0] != '$') return false;
  const char* asterisk = strchr(sentence, '*');
  if (!asterisk) return false;

  uint8_t checksum = 0;
  for (const char* p = sentence + 1; p < asterisk; p++) {
    checksum ^= *p;
  }

  uint8_t received = strtol(asterisk + 1, nullptr, 16);
  return checksum == received;
}

// Parse RMC sentence
void ParserNMEA::parseGPRMC(char* sentence) {
  char* tokens[12] = { nullptr };
  uint8_t t = 0;
  tokens[t++] = strtok(sentence, ",");

  while (t < 12 && (tokens[t] = strtok(nullptr, ","))) t++;

  if (tokens[3] && tokens[4] && tokens[5] && tokens[6]) {
    snprintf(latStr, sizeof(latStr), "%s%c", tokens[3], tokens[4][0]);
    snprintf(lonStr, sizeof(lonStr), "%s%c", tokens[5], tokens[6][0]);
    //Serial.println(latStr);
    //Serial.println(lonStr);
    coordValid = true;
  } else {
    coordValid = false;
  }

  if (tokens[7]) {
    speedKnots = atof(tokens[7]);
    speedValid = true;
  } else {
    speedKnots = 0.0;
    speedValid = false;
  }

  if (tokens[8]) {
    courseDeg = atof(tokens[8]);
    if (courseDeg >= 0.0 && courseDeg < 360.0) {
      courseValid = true;
    } else {
      courseValid = false;
    }
  } else {
    courseDeg = 0.0;
    courseValid = false;
  }
}


// Parse GGA sentence
void ParserNMEA::parseGPGGA(char* sentence) {
  char* tokens[15] = { nullptr };
  uint8_t t = 0;
  tokens[t++] = strtok(sentence, ",");

  while (t < 15 && (tokens[t] = strtok(nullptr, ","))) t++;

  // Extraction de l'heure
  if (tokens[1] && strlen(tokens[1]) >= 6) {
    hour = (tokens[1][0] - '0') * 10 + (tokens[1][1] - '0');
    minute = (tokens[1][2] - '0') * 10 + (tokens[1][3] - '0');
    second = (tokens[1][4] - '0') * 10 + (tokens[1][5] - '0');
    timeValid = true;
  } else {
    timeValid = false;
  }

  // Extraction de l'altitude (champ 9)
  if (tokens[9]) {
    float alt = atof(tokens[9]);
    if (alt >= 0.0f) {
      altitudeMeters = alt;
      altValid = true;
    } else {
      altitudeMeters = 0.0;
      altValid = false;
    }
  } else {
    altitudeMeters = 0.0;
    altValid = false;
  }
}


float ParserNMEA::getLatDec() {
  if (!coordValid || strlen(latStr) < 4) return 0.0;

  // Extraire direction
  char dir = latStr[strlen(latStr) - 1];
  if (dir != 'N' && dir != 'S') return 0.0;

  // Extraire degrés (2 premiers chiffres)
  char degStr[3] = { latStr[0], latStr[1], '\0' };
  int deg = atoi(degStr);

  // Extraire minutes (le reste sauf direction)
  char minStr[8];
  strncpy(minStr, &latStr[2], strlen(latStr) - 3);
  minStr[strlen(latStr) - 3] = '\0';
  float min = atof(minStr);

  float dec = deg + (min / 60.0);
  if (dir == 'S') dec = -dec;

  return dec;
}

float ParserNMEA::getLongDec() {
  if (!coordValid || strlen(lonStr) < 5) return 0.0;

  // Extraire direction
  char dir = lonStr[strlen(lonStr) - 1];
  if (dir != 'E' && dir != 'W') return 0.0;

  // Extraire degrés (3 premiers chiffres)
  char degStr[4] = { lonStr[0], lonStr[1], lonStr[2], '\0' };
  int deg = atoi(degStr);

  // Extraire minutes (le reste sauf direction)
  char minStr[8];
  strncpy(minStr, &lonStr[3], strlen(lonStr) - 4);
  minStr[strlen(lonStr) - 4] = '\0';
  float min = atof(minStr);

  float dec = deg + (min / 60.0);
  if (dir == 'W') dec = -dec;

  return dec;
}


const char* ParserNMEA::getLat() {
  return latStr;
}

const char* ParserNMEA::getLong() {
  return lonStr;
}

int ParserNMEA::getSpeed() const {
  return (int)(speedKnots * 1.852 + 0.5); // knots to km/h arrondi
}

int ParserNMEA::getCourse() const {
  return (int)(courseDeg + 0.5); // arrondi degré
}

bool ParserNMEA::isTimeValid() const {
  return timeValid;
}

bool ParserNMEA::isCoordValid() const {
  return coordValid;
}

bool ParserNMEA::isSpeedValid() const {
  return speedValid;
}

bool ParserNMEA::isCourseValid() const {
  return courseValid;
}

void ParserNMEA::clearNewData() {
  newData = false;
}

bool ParserNMEA::isAltValid() const {
  return altValid;
}


// Retourne l'altitude en mètres (entier)
float ParserNMEA::getAltitudeMeters() const {
  return altitudeMeters;
}

byte ParserNMEA::getSecond() {
 return second;
}
byte ParserNMEA::getMinute() {
return minute;
}
char * ParserNMEA::getTime() const {
  static char timeStr[9]; // "HH:MM:SS" + null terminator
  snprintf(timeStr, sizeof(timeStr), "%02u:%02u:%02u", hour, minute, second);
  return timeStr;
}
