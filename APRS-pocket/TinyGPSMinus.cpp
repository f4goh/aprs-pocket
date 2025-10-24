/*
TinyGPS - a small GPS library for Arduino providing basic NMEA parsing
Based on work by and "distance_to" and "course_to" courtesy of Maarten Lamers.
Suggestion to add satellites(), course_to(), and cardinal(), by Matt Monson.
Precision improvements suggested by Wayne Holder.
Copyright (C) 2008-2013 Mikal Hart
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "TinyGPSMinus.h"

// NMEA Sentence identifiers:
#define _GGA_TERM   "GGA"
#define _GLL_TERM   "GLL"
#define _RMC_TERM   "RMC"
#define _VTG_TERM   "VTG"

TinyGPSMinus::TinyGPSMinus(Fsk &f)
  : fsk(f) 
  , _time(GPS_INVALID_TIME)
  ,  _date(GPS_INVALID_DATE)
  ,  _latitude(_GPS_INVALID_LATITUDE)
  ,  _longitude(_GPS_INVALID_LONGITUDE)
  ,  _altitude(GPS_INVALID_ALTITUDE)
  ,  _speed(GPS_INVALID_SPEED)
  ,  _course(GPS_INVALID_ANGLE)
  ,  _hdop(GPS_INVALID_HDOP)
  ,  _ambiguity(0)
  ,  _last_time_fix(GPS_INVALID_FIX_TIME)
  ,  _last_position_fix(GPS_INVALID_FIX_TIME)
  ,  _parity(0)
  ,  _is_checksum_term(false)
  ,  _sentence_type(_OTHER_SENTENCE)
  ,  _term_number(0)
  ,  _term_offset(0)
  ,  _data_good(false)
  #ifndef _GPS_NO_STATS
    ,  _encoded_characters(0)
    ,  _good_sentences(0)
    ,  _failed_checksum(0)
  #endif
{
  _term[0] = '\0';
}

//
// public methods
//

bool TinyGPSMinus::encode(char c) {
  bool valid_sentence = false;

  #ifndef _GPS_NO_STATS
    ++_encoded_characters;
  #endif
  switch(c) {
    case ',': // term terminators
      _parity ^= c;
    case '\r':
    case '\n':
    case '*':
      if (_term_offset < sizeof(_term)) {
        _term[_term_offset] = 0;
        valid_sentence = term_complete();
      }
      ++_term_number;
      _term_offset = 0;
      _is_checksum_term = c == '*';
      return valid_sentence;

    case '$': // sentence begin
      _term_number = _term_offset = 0;
      _parity = 0;
      _sentence_type = _OTHER_SENTENCE;
      _is_checksum_term = false;
      _data_good = false;
      return valid_sentence;
  }

  // ordinary characters
  if (_term_offset < sizeof(_term) - 1)
    _term[_term_offset++] = c;
  if (!_is_checksum_term)
    _parity ^= c;

  return valid_sentence;
}

#ifndef _GPS_NO_STATS
  void TinyGPSMinus::stats(unsigned long *chars, unsigned short *sentences, unsigned short *failed_cs) {
    if (chars) *chars = _encoded_characters;
    if (sentences) *sentences = _good_sentences;
    if (failed_cs) *failed_cs = _failed_checksum;
  }
#endif

//
// internal utilities
//
int TinyGPSMinus::from_hex(char a) {
  if (a >= 'A' && a <= 'F')
    return a - 'A' + 10;
  else if (a >= 'a' && a <= 'f')
    return a - 'a' + 10;
  else
    return a - '0';
}

unsigned long TinyGPSMinus::parse_decimal() {
  char *p = _term;
  bool isneg = *p == '-';
  if (isneg) ++p;
  unsigned long ret = 100UL * gpsatol(p);
  while (gpsisdigit(*p)) ++p;
  if (*p == '.') {
    if (gpsisdigit(p[1])) {
      ret += 10 * (p[1] - '0');
      if (gpsisdigit(p[2]))
        ret += p[2] - '0';
    }
  }
  return isneg ? -ret : ret;
}

// Parse string in form ddmm.hhhh and convert to char array in form ddmm.hhN
void TinyGPSMinus::parse_latitude() {
  // first 6 are unchanged
  for (int i = 0; i < 6; i++) {
    _latitude[i] = _term[i];
  }

  // round to 2 decimal places
  if (_term[7] - '0' >= 5) {
    _latitude[6] = _term[6] + 1;
  } else {
    _latitude[6] = _term[6];
  }

  // add N/S (set to N for now, change to S when parsing later)
  _latitude[7] = 'N';

  // null terminate
  _latitude[8] = '\0';
}

// Parse string in form dddmm.hhhh and convert to char array in form dddmm.hhW
void TinyGPSMinus::parse_longitude() {
  // first 7 are unchanged
  for (int i = 0; i < 7; i++) {
    _longitude[i] = _term[i];
  }

  // round to 2 decimal places
  if (_term[8] - '0' >= 5) {
    _longitude[7] = _term[7] + 1;
  } else {
    _longitude[7] = _term[7];
  }

  // add E/W (set to E for now, change to W when parsing later)
  _longitude[8] = 'E';

  // null terminate
  _longitude[9] = '\0';
}

void TinyGPSMinus::update_ambiguity() {
  _ambiguity = 5.0 * f_hdop(); // gives very rough horizontal ambiguity in meters
  
  // https://www.usna.edu/Users/oceano/pguth/md_help/html/approx_equivalents.htm
  
  if (_ambiguity > 1110000.0) {
    // remove any digits after latitude X_˚ (step size < 1,110km)
    // ie 4217.65N -> 4___.__N
    _latitude[1] = ' ';
    _latitude[2] = ' ';
    _latitude[3] = ' ';
    // 4 is the decimal point
    _latitude[5] = ' ';
    _latitude[6] = ' ';
    return;
  }
  if (_ambiguity > 111000.0) {
    // remove any digits after latitude X˚ (step size < 111km)
    // ie 4217.65N -> 42__.__N
    _latitude[2] = ' ';
    _latitude[3] = ' ';
    // 4 is the decimal point
    _latitude[5] = ' ';
    _latitude[6] = ' ';
    return;
  }
  if (_ambiguity > 18500.0) {
    // remove any digits after latitude X_' (step size < 18.5km)
    // ie 4217.65N -> 421_.__N
    _latitude[3] = ' ';
    // 4 is the decimal point
    _latitude[5] = ' ';
    _latitude[6] = ' ';
    return;
  }
  if (_ambiguity > 1850.0) {
    // remove any digits after latitude X' (step size < 1.85km)
    // ie 4217.65N -> 4217.__N
    // 4 is the decimal point
    _latitude[5] = ' ';
    _latitude[6] = ' ';
    return;
  }
  if (_ambiguity > 185.0) {
    // remove any digits after latitude 0.X' (step size < 185m)
    // ie 4217.65N -> 4217.6_N
    _latitude[6] = ' ';
    return;
  }
}

#define COMBINE(sentence_type, term_number) (((unsigned)(sentence_type) << 5) | term_number)

// Processes a just-completed term
// Returns true if new sentence has just passed checksum test and is validated
bool TinyGPSMinus::term_complete() {
  // if sentence is over, confirm it is good and return
  if (_is_checksum_term) {
    byte checksum = 16 * from_hex(_term[0]) + from_hex(_term[1]);
    if (checksum == _parity) {
      #ifndef _GPS_NO_STATS
        ++_good_sentences;
      #endif
      return true;
    } else {
      #ifndef _GPS_NO_STATS
        ++_failed_checksum;
      #endif
      return false;
    }
  }

  // the first term determines the sentence type
  if (_term_number == 0) {
    if (!gpsstrcmp(_term, _GGA_TERM))
      _sentence_type = _GGA_SENTENCE;
    else if (!gpsstrcmp(_term, _GLL_TERM))
      _sentence_type = _GLL_SENTENCE;
    else if (!gpsstrcmp(_term, _RMC_TERM))
      _sentence_type = _RMC_SENTENCE;
    else if (!gpsstrcmp(_term, _VTG_TERM))
      _sentence_type = _VTG_SENTENCE;
    else
      _sentence_type = _OTHER_SENTENCE;
    return false;
  }

  // parse a term based on the sentence type
  if (_sentence_type != _OTHER_SENTENCE && _term[0]) {
    switch(COMBINE(_sentence_type, _term_number)) {
      case COMBINE(_GGA_SENTENCE, 1): // Time
      case COMBINE(_GLL_SENTENCE, 5):
      case COMBINE(_RMC_SENTENCE, 1):
        _time = parse_decimal();
        _last_time_fix = fsk.millis();
        break;
      case COMBINE(_GLL_SENTENCE, 6): // Validity
      case COMBINE(_RMC_SENTENCE, 2):
        _data_good = _term[0] == 'A';
        break;
      case COMBINE(_GGA_SENTENCE, 2): // Latitude
      case COMBINE(_GLL_SENTENCE, 1):
      case COMBINE(_RMC_SENTENCE, 3):
        parse_latitude();
        _last_position_fix = fsk.millis();
        break;
      case COMBINE(_GGA_SENTENCE, 3): // N/S
      case COMBINE(_GLL_SENTENCE, 2):
      case COMBINE(_RMC_SENTENCE, 4):
        if (_term[0] == 'S')
          _latitude[7] = 'S';
        break;
      case COMBINE(_GGA_SENTENCE, 4): // Longitude
      case COMBINE(_GLL_SENTENCE, 3):
      case COMBINE(_RMC_SENTENCE, 5):
        parse_longitude();
        break;
      case COMBINE(_GGA_SENTENCE, 5): // E/W
      case COMBINE(_GLL_SENTENCE, 4):
      case COMBINE(_RMC_SENTENCE, 6):
        if (_term[0] == 'W')
          _longitude[8] = 'W';
        break;
      case COMBINE(_RMC_SENTENCE, 7): // Speed, knots
      case COMBINE(_VTG_SENTENCE, 5):
        _speed = parse_decimal();
        break;
      case COMBINE(_RMC_SENTENCE, 8): // True course, degrees
      case COMBINE(_VTG_SENTENCE, 1):
        _course = parse_decimal();
        break;
      case COMBINE(_RMC_SENTENCE, 9): // Date
        _date = gpsatol(_term);
        break;
      case COMBINE(_GGA_SENTENCE, 6): // Fix data
        _data_good = _term[0] > '0';
        break;
      case COMBINE(_GGA_SENTENCE, 9): // Altitude
        _altitude = parse_decimal();
        break;
      case COMBINE(_GGA_SENTENCE, 8): // HDOP
        _hdop = parse_decimal();
        update_ambiguity();
        break;
    }
  }
  return false;
}

long TinyGPSMinus::gpsatol(const char *str) {
  long ret = 0;
  while (gpsisdigit(*str))
    ret = 10 * ret + *str++ - '0';
  return ret;
}

int TinyGPSMinus::gpsstrcmp(const char *str1, const char *str2) {
  // throw out first two talker ID characters
  str1 += 2;
  while (*str1 && *str1 == *str2)
    ++str1, ++str2;
  return *str1;
}


char * TinyGPSMinus::get_latitude() {
  return _latitude;
}

char * TinyGPSMinus::get_longitude() {
  return _longitude;
}

float  TinyGPSMinus::get_latitudeDeg(){
  char degStr[3];
  char minStr[6];
  strncpy(degStr,_latitude,2);
  strncpy(minStr,_latitude+2,5);
  degStr[2]='\0';
  degStr[5]='\0';
  //Serial.println(degStr);
  //Serial.println(minStr);
  int deg=atoi(degStr);
  float min=atof(minStr);
  //Serial.println(deg);
  //Serial.println(min);
  float coord=deg+(min/60.0);
  if (_latitude[7]=='S'){
    coord*=-1;
 }
  return coord;
}

float  TinyGPSMinus::get_longitudeDeg(){
  char degStr[4];
  char minStr[6];
  strncpy(degStr,_longitude,3);
  strncpy(minStr,_longitude+3,5);
  degStr[3]='\0';
  degStr[5]='\0';
  //Serial.println(degStr);
  //Serial.println(minStr);
  int deg=atoi(degStr);
  float min=atof(minStr);
  //Serial.println(deg);
  //Serial.println(min);
  float coord=deg+(min/60.0);
  if (_longitude[8]=='W'){
    coord*=-1;
 }
  return coord;
}



void TinyGPSMinus::get_pos_age(unsigned long *fix_age) {
  if (fix_age) *fix_age = _last_position_fix == GPS_INVALID_FIX_TIME ? GPS_INVALID_AGE : fsk.millis() - _last_position_fix;
}

// date as ddmmyy, time as hhmmsscc, and age in milliseconds
void TinyGPSMinus::get_datetime(unsigned long *date, unsigned long *time, unsigned long *age) {
  if (date) *date = _date;
  if (time) *time = _time;
  if (age) *age = _last_time_fix == GPS_INVALID_FIX_TIME ? GPS_INVALID_AGE : fsk.millis() - _last_time_fix;
}

void TinyGPSMinus::crack_datetime(int *year, byte *month, byte *day, byte *hour, byte *minute, byte *second, byte *hundredths, unsigned long *age) {
  unsigned long date, time;
  get_datetime(&date, &time, age);
  if (year) {
    *year = date % 100;
    *year += *year > 80 ? 1900 : 2000;
  }
  if (month) *month = (date / 100) % 100;
  if (day) *day = date / 10000;
  if (hour) *hour = time / 1000000;
  if (minute) *minute = (time / 10000) % 100;
  if (second) *second = (time / 100) % 100;
  if (hundredths) *hundredths = time % 100;
}

float TinyGPSMinus::f_altitude() {
  return _altitude == GPS_INVALID_ALTITUDE ? GPS_INVALID_F_ALTITUDE : _altitude / 100.0;
}

float TinyGPSMinus::f_course() {
  return _course == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : _course / 100.0;
}

float TinyGPSMinus::f_hdop() {
  return _hdop == GPS_INVALID_HDOP ? GPS_INVALID_F_HDOP : _hdop / 100.0;
}

float TinyGPSMinus::f_ambiguity() {
  return _ambiguity;
}

float TinyGPSMinus::f_speed_knots() {
  return _speed == GPS_INVALID_SPEED ? GPS_INVALID_F_SPEED : _speed / 100.0;
}

float TinyGPSMinus::f_speed_mph() { 
  float sk = f_speed_knots();
  return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : _GPS_MPH_PER_KNOT * sk; 
}

float TinyGPSMinus::f_speed_mps() { 
  float sk = f_speed_knots();
  return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : _GPS_MPS_PER_KNOT * sk; 
}

float TinyGPSMinus::f_speed_kmph() { 
  float sk = f_speed_knots();
  return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : _GPS_KMPH_PER_KNOT * sk; 
}

const float TinyGPSMinus::GPS_INVALID_F_ANGLE = 1000.0;
const float TinyGPSMinus::GPS_INVALID_F_ALTITUDE = 1000000.0;
const float TinyGPSMinus::GPS_INVALID_F_SPEED = -1.0;
const float TinyGPSMinus::GPS_INVALID_F_HDOP = 100.0;
