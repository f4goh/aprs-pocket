/* 
 * File:   Fsk.h
 * Author: Anthony & Philippe S (Touchard Washington)
 *
 * Created on 25 juillet 2021, 10:20
 */

#ifndef FSK_H
#define FSK_H
#include "Dds.h"

#define MARK  1200.0        //fréquence mark
#define SHIFT 1000.0        //saut de fréquence  pour calculer la fréquence space (space=mark+shift)

enum stopBits {
  BIT_1,
  BIT_1_5,
  BITS_2
};

class Fsk : public Dds 
{
public:
    Fsk(float mkFreq = MARK, float shFreq = SHIFT, float br = 1200);
    Fsk(const Fsk& orig);
    virtual ~Fsk();
    
    
    void setBitRate(float br); 
    void setMarkFrequence(float mkFreq);
    void setSpaceFrequence(float spFreq);
    
    void sendBit(bool value);
    void sendStopBit(stopBits nStop);
    void sendBitOff();
    
private:
    uint32_t incrementMark;                // Incrément de phase pour la fréquence Mark
    uint32_t incrementSpace;               // Incrément de phase pour la fréquence Space
    int nbEchPerBit;    //nombre d'échantillons pour un bit en accord avec le bit rate
};

#endif /* FSK_H */
