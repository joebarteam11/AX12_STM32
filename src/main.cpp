#include "mbed.h"
#include "AX12.h"
#include "main.h"

#define CONTINOUS_MODE 0

int main(void){
    //factoryReset(); // run this line code ALONE to reset all connected motors ID to 1 (factory reset), then reboot the servo by unpluging and repluging it
    //setMotorBaud(AX12_BAUD); // then run this line to change the baudrate of the motor to the one defined in main.h Servo needs to be rebooted after this line
    
    #if CONTINOUS_MODE
    //Test program in continuous mode
    initMotor(AX12_BAUD);
    Motor->SetMode(1); //Continuous mode

    rotateClockwise();
    ThisThread::sleep_for(1s);
    stopMotor();
    ThisThread::sleep_for(1s);
    rotateCounterClockwise();
    ThisThread::sleep_for(1s);
    stopMotor();

    printf("Test program ended (continuous mode)\n)");

    #else
    // To test the motor in position mode

    initMotor(AX12_BAUD);
    Motor->SetMode(0); //Position mode
    
    Motor->SetGoal(0);   // go to 0 degrees
    ThisThread::sleep_for(2s);
    Motor->SetGoal(300); // go to 300 degrees
    // Print the load every 500ms
    while(1){
        printf("Load : %f\n", getMotorLoad());
        ThisThread::sleep_for(500);
    }
    #endif
}
