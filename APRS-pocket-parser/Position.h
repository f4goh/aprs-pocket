/*
   File:   Position.h
   Author: philippe

   Created on 7 août 2021, 08:00
*/

#ifndef POSITION_H
#define POSITION_H

#include <stdio.h>
#include <math.h>
#include <Arduino.h>

class Position {
  public:

    Position(const float _latitude,
             const float _longitude,
             char * _comment,
             const char _symboleTable = '/',
             const char _symbole = '>'
            );

    Position(const Position& orig);
    virtual ~Position();

    const char* getPduAprs(bool compressed = false,bool altEnable = false);
    void setLatitude(const float _latitude);
    void setLongitude(const float _longitude);
    void setComment(char * _comment);
    void setAltitude(const float _alt);
    void setSymbol(char _symbole);


  protected:
    void latitude_to_str();
    void longitude_to_str();
    void convBase91(int32_t x, char* base91);
    char    slat[9];
    char    slong[10];
    float   latitude;
    float   longitude;
    char    symboleTable;
    char    symbole;
    char    pdu[100];
    char * comment;


  private:

    uint32_t altitude;
    void latitude_to_comp_str();
    void longitude_to_comp_str();
    void trimTrailingSpaces(char *str);
};

#endif /* POSITION_H */
