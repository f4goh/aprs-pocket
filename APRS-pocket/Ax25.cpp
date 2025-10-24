/*
   File:   Ax25.cpp
   Author: Anthony
   Created on 21 juillet 2021, 18:47
   Classe Ax25 simple conçue pour 4 callsigns
*/

#include "Ax25.h"

Ax25::Ax25(Fsk &f) :
  fsk(f) {
  buffer = new uint8_t[AX25_MAX_LEN];
}

Ax25::~Ax25() {
  delete[] buffer;
}

/**
   @brief Ax25::begin()
   @details Permet d'initialiser le Ax25
   @param *sourceCallsign indicatif de la source
  destinationCallsign indicatif du destinataire
  path1 chemin1
  path2 chemin2
*/

void Ax25::begin(int br, char *sourceCallsign, char *destinationCallsign, char *path1, char *path2) {
  uint8_t *ptr = buffer;
  fsk.setBitRate(br); //positionne le bitrate
  ptr = addCallsign(buffer, destinationCallsign); //ajoute les 2 callSigns et 2 chemins dans l'header
  ptr = addCallsign(ptr, sourceCallsign);
  ptr = addCallsign(ptr, path1);
  ptr = addCallsign(ptr, path2);

  ptr[-1] |= 1; //Marqueur fin de l'entete sur le dernier ssid

  *(ptr++) = AX25_CONTROL; //ajout ctrl + protocol
  *(ptr++) = AX25_PROTOCOL;
}

/**
   @brief Ax25::addCallsign(uint8_t *buf, char *callsign)
   @details Ajoute un callSign dans la trame entete
   @param *buf pointeur dans le buffer pour le positionnement du callsign
  callsign pointeur de la chaine callsign
          retourne le pointeur courant ds le buffer pour le prochain ajout
*/

uint8_t * Ax25::addCallsign(uint8_t *buf, char *callsign) {
  char ssid;
  char i;
  for (i = 0; i < 6; i++) {
    if (*callsign && *callsign != '-') *(buf++) = *(callsign++) << 1;
    else *(buf++) = ' ' << 1;
  }

  if (*callsign == '-') ssid = atoi(callsign + 1);
  else ssid = 0;

  *(buf++) = ('0' + ssid) << 1;
  return (buf);
}

/**
   @brief uint16_t Ax25::crcCcittUpdate(uint16_t crc, uint8_t data)
   @details calcul le crc d'un octet (valeur cumulée)
   @param crc précédent
          octet a prendre en compte
          retourne un nouveau crc
*/

uint16_t Ax25::crcCcittUpdate(uint16_t crc, uint8_t data) {
  data ^= crc & 0xFF;
  data ^= data << 4;

  return ((((uint16_t) data << 8) | (crc >> 8)) ^ (uint8_t) (data >> 4) ^ ((uint16_t) data << 3));
}

/**
   @brief Ax25::calculateCRC()
   @details calcul le crc de la trame complete et ajoute le crc en fin de trame
*/

void Ax25::calculateCRC() {
  uint8_t *s;
  uint16_t crc = 0xFFFF;
  for (s = buffer; s < buffer + frameLength; s++) {
    crc = crcCcittUpdate(crc, *s);
  }
  //Serial.println(~crc, HEX);
  *(s++) = ~(crc & 0xFF);
  *(s++) = ~((crc >> 8) & 0xFF);
  frameLength += 2;
}


/**
   @brief Ax25::txMessage(char *bufMsg)
   @details ajoute le message après l'entete
            calcul le crc, puis envoie la trame au modulateur fsk
   @param  bufMsg pointeur de chaine du message
*/

void Ax25::txMessage(char *bufMsg) {
  int bufLen = strlen(bufMsg);
  memcpy(buffer + AX25_HEADER_SIZE, bufMsg, bufLen);
  frameLength = AX25_HEADER_SIZE + bufLen;  
  calculateCRC();
  //debug();
  txFrame();
}

/**
   @brief Ax25::flipOut()
   @details met à zéro du bit stuffing et inverse le drapeau flip du mark <-> space
*/

void Ax25::flipOut() {
  stuff = 0; //mise à zéro du bit stuffing
  flip ^= 1; //inversion drapeau mark <-> space
}

/**
   @brief Ax25::sendByte(uint8_t inByte, bool flag)
   @details envoie un octet vers le modulateur fsk en gérant le bitstuffing
   @param  inByte octet a envoyer
           flag d'information pour ne pas faire de bitstuffing lors de l'envoi du préambule
           0 : bitstuffing
           1 : pas de bitstuffing
*/

void Ax25::sendByte(uint8_t inByte, bool flag) {
  uint8_t k, bt;
  //Serial.print(inByte,HEX);
  for (k = 0; k < 8; k++) {
    bt = inByte & 0x01; //masque sur le LSB 1st
    if (bt == 0) flipOut(); //si c'est un 0 alors appel inversion mark <-> space
    else { //c'est un 1 on reste sur la meme fréquence
      stuff++; //incrementation bit stuffing
      if ((flag == 0) && (stuff == 5)) //inversion mark <-> space pour le bit stuffing
      { //et si flag = 0 (pas de bit stuffing dans le préambule)
        fsk.sendBit(flip);
        flipOut();
      }
    }
    inByte >>= 1; //décallage vers la droite pour le prochain bit
    fsk.sendBit(flip); //envoi du bit en cours
  }
}

/**
   @brief Ax25::txFrame()
   @details envoie la trame complete en ajoutant le préambule de début et de fin
*/
void Ax25::txFrame() {
  int i;
  stuff = 0; //compteur de bit stuffing à zéro
  flip = 1;  //fréquence space activée
  fsk.start();
  fsk.sendBit(flip);  
  for (i = 0; i < AX25_PREAMBULE_LEN; i++) sendByte(AX25_PREAMBULE_BYTE, 1); //préambule de début
  for (i = 0; i < frameLength; i++) sendByte(buffer[i], 0); //envoi du packet    
  for (i = 0; i < AX25_PREAMBULE_LEN; i++) sendByte(AX25_PREAMBULE_BYTE, 1); //préambule de fin
  fsk.stop();
}

/**
   @brief Ax25::debug()
   @details impossible de faire la classe sans cela !!
*/

void Ax25::debug() {
  int n;
  Serial.print(F("longueur de la trame : "));
  Serial.print(frameLength);
  Serial.println(F(" Octets"));
  for (n = 0; n < AX25_HEADER_SIZE; n++) {
    Serial.print(buffer[n], HEX);
    Serial.print('(');
    Serial.print((char) (buffer[n] >> 1));
    Serial.print(')');
  }
  Serial.println(F("\n\rPDU APRS"));
  for (n = AX25_HEADER_SIZE; n < frameLength - 2; n++) {
    Serial.print((char) buffer[n]);
  }
  Serial.println(F("\n\rCRC"));
  Serial.print(buffer[frameLength - 2], HEX);
  Serial.print(',');
  Serial.println(buffer[frameLength - 1], HEX);
}
