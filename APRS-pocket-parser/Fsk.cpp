/*
   File:   Fsk.cpp
   Author: Anthony & Philippe (Touchard Washington)

   Created on 25 juillet 2021, 10:20
*/

#include "Fsk.h"

Fsk::Fsk(float mkFreq, float shFreq, float br ) :
  Dds()
{
  incrementMark  = Dds::computeIncrementPhase(mkFreq);
  incrementSpace = Dds::computeIncrementPhase(mkFreq + shFreq);
  setBitRate(br);
}

Fsk::Fsk(const Fsk& orig) {
}

Fsk::~Fsk() {
}



void Fsk::sendBit(bool value) {

  if (!value)
    Dds::incrementPhase = incrementMark; // active la fréquence Mark (basse fréquence) (0 logique)
  else
    Dds::incrementPhase = incrementSpace; // active la fréquence prédéterminée space (haute frequence) (1 logique)
  compteur = 0;
  while (compteur < nbEchPerBit) {
    //Serial.println(compteur);
  }
}

/**
   @brief Fsk::setBitRate(float br)
   @details calcul le bitrate du signal à transmettre en accord avec la féquence d'achantillonage
   @param   float  bitrate
*/

void Fsk::setBitRate(float br) {
  nbEchPerBit = round(splFreq / br);
  //Serial.println("------------");
  //Serial.println(nbEchPerBit);
}

/**
   @brief Fsk::sendStopBit(stopBits nStop)
   @details envoi d'un bit ou plusieurs bits de stop en accord avec le calcul du bit rate et nbEchPerBit
   @param stopBits nombre de bits de stop
*/

void Fsk::sendStopBit(stopBits nStop) {
  int nbIrq;
  switch (nStop) {
    case BIT_1: nbIrq = nbEchPerBit;
      break;
    case BIT_1_5: nbIrq = nbEchPerBit + (nbEchPerBit / 2);
      break;
    case BITS_2: nbIrq = nbEchPerBit * 2;
      break;
  }
  Dds::incrementPhase = incrementSpace; // active la fréquence prédéterminée space
  Dds::compteur = 0;
  while (Dds::compteur < nbIrq);
}


/**
   @brief Fsk::sendBitOff()
   @details envoi d'un bit en accord avec le calcul du bit rate nbEchPerBit
   @param  aucun la sortie du dds ne génère aucune sinudoide meme si l'interruption Timer est toujours en fonctionement
           Méthode pour la génération d'un signal tout ou rien OOK (On Off Keying)
*/
void Fsk::sendBitOff() {
  Dds::off();
  Dds::compteur = 0;
  while (Dds::compteur < nbEchPerBit);
}

/**
   @brief Configure le saut de phase pour la fréquence Marq
   @param mkFreq
*/
void Fsk::setMarkFrequence(float mkFreq) {
  incrementMark  = Dds::computeIncrementPhase(mkFreq);

}

/**
   @brief Configure le saut de phase pour la fréquence Space
   @param mkFreq
*/
void Fsk::setSpaceFrequence(float spFreq) {
  incrementSpace = Dds::computeIncrementPhase(spFreq);
}
