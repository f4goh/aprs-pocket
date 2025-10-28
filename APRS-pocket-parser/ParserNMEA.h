

#ifndef PARSER_NMEA_H
#define PARSER_NMEA_H

#include <Arduino.h>

class ParserNMEA {
  public:
    ParserNMEA();

    bool encode(char c);
    void clearNewData();
    bool isNewData() const;

    const char* getLat();
    const char* getLong();
    int getSpeed() const;   // km/h arrondi
    int getCourse() const;  // degrés arrondi
    float getAltitudeMeters() const;


    bool isTimeValid() const;
    bool isCoordValid() const;
    bool isSpeedValid() const;
    bool isCourseValid() const;
    bool isAltValid() const;

    float getLatDec();
    float getLongDec();
    byte getSecond();
    byte getMinute();
    char * getTime() const;


  private:
    char buffer[100];
    uint8_t index;
    bool newData;

    bool timeValid;
    bool coordValid;
    bool speedValid;
    bool courseValid;
    bool altValid;
    byte hour, minute, second;

    char latStr[15];
    char lonStr[15];
    float altitudeMeters;
    float speedKnots;
    float courseDeg;

    void parseGPRMC(char* sentence);
    void parseGPGGA(char* sentence);
    bool verifyChecksum(const char* sentence);
};

#endif
