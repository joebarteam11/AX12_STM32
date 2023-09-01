#include "mbed.h"
#include "AX12.h"
#include "main.h"

#define MOTOR_ID 1
#define MOTOR_BAUD 1000000
#define BROADCAST 254

AX12 *Motor=NULL;

void initMotor(int baud, int ID = MOTORID){
   static AX12 servo(TX, RX, ID, baud); //Instanciation du servomoteur AX12A
   Motor = &servo;
}

void rotateClockwise(void){
    //Motor->SetTorque(1);
    Motor->SetCRSpeed(OPENING_SPEED);
}

void rotateCounterClockwise(void){
    Motor->SetCRSpeed(CLOSING_SPEED);
}

void stopMotor(){
    Motor->SetTorque(DISABLE);
}

float getMotorLoad(){
    return Motor->GetLoad();
}

void setMotorID(int ID){
    Motor->SetID(ID);
}

void factoryReset(){
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

initMotor(AX12_BAUD);