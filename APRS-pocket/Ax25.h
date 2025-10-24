#ifndef AX25_H
#define AX25_H
#include <Arduino.h>
#include "Fsk.h"

#define AX25_MAX_LEN 100      //longueur maximum d'un paquet
#define AX25_HEADER_SIZE 30   //longueur de l'entete pour 4 callsign (fixe)
#define AX25_PREAMBULE_BYTE 0x7E  //valeur de l'octet de préambule
#define AX25_PREAMBULE_LEN 50    //longeur du préambule en octets
#define AX25_CONTROL 0x03    //APRS-UI frame
#define AX25_PROTOCOL 0xF0    //Protocol Identifier


class Ax25 {

  public:
    Ax25(Fsk &f);
    virtual ~Ax25();
    void begin(int br,
               char *sourceCallsign ,
               char *destinationCallsign ,
               char *path1 ,
               char *path2);
    void debug();
    void txMessage(char *bufMsg);


  private:
    uint8_t * addCallsign(uint8_t *buf, char *callsign);
    uint16_t crcCcittUpdate(uint16_t crc, uint8_t data);

    void calculateCRC();
    void flipOut();
    void txFrame();
    void sendByte(uint8_t inbyte, bool flag);

    uint8_t *buffer;
    int frameLength;  //longeur de la trame à envoyer
    uint8_t stuff;  //compteur de bit stuffing
    uint8_t flip;   //flag d'inversion mark <-> space  
   
    Fsk & fsk;
};



#endif /* AX25_H */
