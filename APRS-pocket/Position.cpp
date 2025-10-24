/*
   File:   Position.cpp
   Author: philippe & Anthony

   Created on 7 août 2021, 08:00
*/

#include "Position.h"

/**

   @param _latitude
   @param _longitude      double longitude
   @param _comment        String le commentaire
   @param _symboleTable   char pour désigner la table des symboles
   @param _symbole        char pour désigner un symbole dans la table
*/
Position::Position(const float _latitude, const float _longitude, char * _comment, const char _symboleTable, const char _symbole) :
  latitude(_latitude),
  longitude(_longitude),
  symboleTable(_symboleTable),
  symbole(_symbole),
  alt(0)
  {
  comment=_comment;
  trimTrailingSpaces(comment);  
}

Position::Position(const Position& orig) {
}

Position::~Position() {
}

void Position::setLatitude(const float _latitude) {
  latitude = _latitude;
}

void Position::setLongitude(const float _longitude) {
  longitude = _longitude;
}

void Position::setComment(char  *_comment) {
  comment = _comment;
  trimTrailingSpaces(comment);
}

void Position::setSymbol(char _symbole) {
  symbole = _symbole;
}


void Position::setAltitude(const float _alt) {
  alt = (int)(3.2809 * _alt);
}
/**
   @brief Fabrique le PDU APRS position
          si compressed est true la position est compressée (base91)
   @param bool compressed indique si la position doit être compressée
   @return char* Le pdu APRS position
*/
char* Position::getPduAprs(bool compressed) {
  //char com[44];
  //char scom[34];
  //char salt[10];

  //provisoire
/*
  if (alt != 0) {
    snprintf(salt, sizeof (salt), "/A=%06d", alt);
    //comment.toCharArray(scom, 33);
    snprintf(com, sizeof(com), "%s %s", salt, scom);
  } else {
    strcpy(com,comment);    
  }
*/

 
  if (compressed) {

    latitude_to_comp_str();
    longitude_to_comp_str();

    snprintf(pdu, sizeof (pdu), "!%c%s%s%c  T%s", symboleTable, slat, slong, symbole, comment);
  }
  else {
    snprintf(pdu, sizeof (pdu), "!%s%c%s%c%s", slat, symboleTable, slong, symbole, comment);
  }
  return pdu;
}

void Position::setLatitudeStr(char * _latitude){
  strcpy(slat,_latitude);
}
void Position::setLongitudeStr(char * _longitude){
  strcpy(slong,_longitude);
}



void Position::latitude_to_comp_str() {

  int32_t y;
  y = (int32_t) round(380926. * (90. - latitude));
  convBase91(y, slat);
}

void Position::longitude_to_comp_str() {

  int32_t x;
  x = (int32_t) round(190463. * (180. + longitude));
  convBase91(x, slong);
}

void Position::convBase91(int32_t x, char* base91) {

  //int c[4];

  for (int i = 3; i >= 0; i--) {
    base91[i] = (x % 91) + 33;
    x /= 91;
  }
  base91[4] = '\0';
}

void Position::trimTrailingSpaces(char *str) {
  int len = strlen(str);
  while (len > 0 && str[len - 1] == ' ') {
    str[--len] = '\0';
  }
}
