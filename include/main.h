/**
 * @file main.h
 * @author joebarteam11
 * @brief basic functions for the AX12 easy control
 * @version 0.2
 * @date 2021-02-19
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef MBED_MAIN_H
#define MBED_MAIN_H

#include "mbed.h"
#include "AX12.h"

#define TX D1
#define RX D0
#define MOTORID 1 //ID du moteur
#define BROADCAST 254
//#define ULTIMATE_MAX_TORQUE 1.0 //pourcentage du couple fixé max autorisé au moteur
#define LOAD_MAX_TORQUE 0.7 //[0.0 ; 1.0] //pourcentege de la valeur max de couple fixé (ici 70% de 40%...)
#define USB_BAUD 9600 // NE PAS METTRE 115200, sinon perte de communication avec le servomoteur
#define AX12_BASE_BAUD 1000000
#define AX12_BAUD 115200
#define OPENING_SPEED -0.9 //[-1.0 ; 0.0]
#define CLOSING_SPEED 0.9 //[0.0 ; 1.0]
#define DISABLE false



AX12 *Motor=NULL;


/**
 * @brief Fonction permettant d'initialiser le servomoteur avec le baud rate souhaité
 * 
 */
void initMotor(int baud, int ID = MOTORID){
   static AX12 servo(TX, RX, ID, baud); //Instanciation du servomoteur AX12A
   Motor = &servo;
}


/**
 * @brief Fonction permettant d'enclencher un Timer
 * 
 * @param tim est un pointeur vers le timer à enclencher
 * @return true si tout s'est passé correctement
 */
bool timerStart(Timer* tim){
    tim->reset();
    tim->start();
    return true;
}

/**
 * @brief Fonction permettant d'arrêter un Timer
 * 
 * @param tim est un pointeur vers le timer à arrêter
 * @return true si tout s'est passé correctement
 */
bool timerStop(Timer* tim){
    tim->stop();
    tim->reset();
    return true;
}

/**
 * @brief Fonction qui permet de demander au moteur de s'arrêter. On désactive toutes les bases de temps pour éviter de compter dans le vide
 * 
 */
void stopMotor(){
    #if DEBUG
        printf("Stopping motor...\n");
    #endif
    Motor->SetTorque(DISABLE);
}


void rotateClockwise(void){
    //Motor->SetTorque(1);
    Motor->SetCRSpeed(OPENING_SPEED);
}

void rotateCounterClockwise(void){
    Motor->SetCRSpeed(CLOSING_SPEED);
}

float getMotorLoad(){
    return Motor->GetLoad();
}

void setMotorID(int ID){
    Motor->SetID(ID);
}

void factoryReset(){
    printf("Factory reset\n");
    long baudrates[9]={1000000,500000,400000,250000,200000, 115200,57600,19200,9600};
    for(int i=0; i<9; i++){
        AX12 servo(TX, RX, BROADCAST, baudrates[i]);
        ThisThread::sleep_for(10ms);
        servo.FactoryReset();
    }//reset motors ID to 1
}

void setMotorBaud(int baud){
    AX12 servo(TX, RX, BROADCAST, AX12_BASE_BAUD);
    servo.SetMode(1); //See AX12 documentation or AX12.h
    servo.SetBaud(16); //baudrate = 1Mbps
}

#endif