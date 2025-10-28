/* 
 * File:   Dds.h
 * Author: Anthony (f4goh@orange.fr) & philippe S (philaure@wanadoo.fr)
 *
 * Created on 23 juillet 2021, 11:35
 */

#ifndef DDS_H
#define DDS_H
#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
  

#define SAMPLING_FREQUENCY 26427.0    //fréquence d'échantillonage
#define SYNC 10             //gpio pour oscillo IRQ timer
#define DAC_CHANNEL 3


class Dds {
public:
    Dds(float _splFreq = SAMPLING_FREQUENCY,  
        int _syncLed = SYNC);
    
    Dds(const Dds& orig);
    virtual ~Dds();

    void begin();
    
    void setFrequency(float freq);
    void stop();
    void start();
    void off();
    uint32_t millis();
    void delay(uint16_t tm);
    void setPhase(int ph);
    static Dds* anchor;
    void interruption();
    int syncLed;
    volatile bool ddsEn;
    volatile uint8_t cptMs;
    volatile uint32_t countMs;
    
private:
    volatile uint32_t accumulateur;        // Accumulateur de phase

protected:
    uint32_t computeIncrementPhase(float freq);
    
    volatile uint32_t incrementPhase;      // Increment de phase courant
    volatile uint32_t dephase;             // Valeur du déphasage de la porteuse  
    volatile uint32_t compteur;            // Compteur d'échantillons
    float splFreq;
};

#endif /* DDS_H */
