/**
 * @file main.h
 * @author Mickaël THEOT
 * @brief fichier header contenant les fonctions nécessaires au programme principal
 * Les macros et variables globales sont définies dans ce fichier
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

//Si 1, active le port série pour afficher les messages de DEBUG
#define DEBUG 0
#define DEBUG_MOTOR_INIT 0
#define CALIBRATION 0

#define TX D1
#define RX D0
#define OPEN_BTN A6
#define CLOSE_BTN A1
#define GREEN_LED D7
#define RED_LED D12
#define MOTORID 1 //ID du moteur
#define BROADCAST 254
//#define ULTIMATE_MAX_TORQUE 1.0 //pourcentage du couple fixé max autorisé au moteur
#define LOAD_MAX_TORQUE 0.7 //[0.0 ; 1.0] //pourcentege de la valeur max de couple fixé (ici 70% de 40%...)
#define OPN_DELAY 2000 //ms
#define CLS_DELAY 300 //ms
#define CHECK_LOAD_DELAY 500 //ms delais entre les mesures de couple (doit être > 100ms)
#define BLINK_DELAY 750ms //ms
#define RED_BLINK_DELAY 250ms //ms 
#define EMERGENCY_DELAY 1500ms
#define TIMEOUT 7000ms //ms delais normalment suffisant à la fermeture de la vanne
#define USB_BAUD 9600 // NE PAS METTRE 115200, sinon perte de communication avec le servomoteur
#define AX12_BASE_BAUD 1000000
#define AX12_BAUD 115200
#define OPENING_SPEED -0.9 //[-1.0 ; 0.0]
#define CLOSING_SPEED 0.9 //[0.0 ; 1.0]
#define ON true //VERSION FINALE (anode commune, commande du transistor)
#define OFF false //VERSION FINALE (anode commune, commande du transistor)
#define DISABLE false
#define VALIDATION 4 //Nombre de mesures consécutives devant être correctes
#define SYNC 1
#define ASYNC 0
#define RED -1

#if CALIBRATION
    UnbufferedSerial pc(USBTX,USBRX,USB_BAUD); 
#endif


InterruptIn openButton(OPEN_BTN);
InterruptIn closeButton(CLOSE_BTN); 
DigitalOut greenLED(GREEN_LED); 
DigitalOut redLED(RED_LED); 

Timer timBtnOpn; //timer pour compter le temps d'appuye sur le bouton d'ouverture et valider la demande
Timer timLoad;
Ticker blinkTicker; // ticker pour faire clignoter les LED pour indiquer que le moteur tourne
Timeout halt; // Timeout permettant d'arreter le moteur après un certain délai défini par la macro 'TIMEOUT'

AX12 *Motor=NULL;

short counter = 0;

bool flag_close_pressed = false;
bool flag_open_pressed = false;
bool flag_timeout = false;
bool start_blink = false;
bool isMoving = false;
bool opening = false;
bool closing = false;
bool emergency = false;
bool emergency_close = false;

/**
 * @brief Fonction permettant d'initialiser le servomoteur avec le baud rate souhaité
 * 
 */
void initMotor(int baud, int ID = MOTORID){
   static AX12 servo(TX, RX, ID, baud); //Instanciation du servomoteur AX12A
   Motor = &servo;
   //static AX12 *Motor = new AX12(TX, RX, ID, baud);
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
 * @brief Fonction attachée à une interruption qui passe le booléen \p flag_open_pressed à true quand un bouton est relaché
 * Cette fonction permet d'enclencher le Timer qui compte le temps d'appuie sur le bouton d'ouverture \p openButton
 * 
 * @attention Interrupt attached function, cannot use printf inside and can only be of 'void' type
 */
void openPressed(void){
    timerStart(&timBtnOpn);
    flag_open_pressed = true;
}

/** 
 * @brief Fonction attachée à une interruption qui passe le booléen \p flag_open_pressed à false quand un bouton est relaché
 * Cette fonction permet d'arrêter le Timer qui compte le temps d'appuie sur le bouton d'ouverture \p openButton
 * 
 * @attention Interrupt attached function, cannot use printf inside and can only be of 'void' type
 */
void openReleased(void){
    timerStop(&timBtnOpn);
    flag_open_pressed = false;
}

/** 
 * @brief Fonction attachée à une interruption qui passe le booléen \p flag_close_pressed à true quand un bouton est enfoncé
 * 
 * @attention Interrupt attached function, cannot use printf inside and can only be of 'void' type
 */
void closePressed(void){
    flag_close_pressed = true;
}

/** 
 * @brief Fonction attachée à une interruption qui passe le booléen \p flag_close_pressed à false quand un bouton est relaché
 * 
 * @attention Interrupt attached function, cannot use printf inside and can only be of 'void' type
 */
void closeReleased(void){
    flag_close_pressed = false;
}

/** 
 * @brief Fonction attachée à un Ticker ( \p blinkTicker ) qui est appelé toutes les \p BLINK_DELAY millisecondes pour changer l'état courant des LED
 *  
 * @attention Interrupt attached function, cannot use printf inside and can only be of 'void' type
 */
void switchLED(void){
    greenLED = !greenLED;
    redLED = !redLED;
}

/** 
 * @brief Fonction attachée à un Ticker ( \p blinkTicker ) qui est appelé toutes les \p BLINK_DELAY millisecondes pour changer l'état courant de la LED
 * @attention Interrupt attached function, cannot use printf inside and can only be of 'void' type
 */
void blinkRED(void){
    redLED = !redLED;
}

void emergencyStop(void){
    emergency_close = true;
    flag_timeout = true;
}

/** 
 * @brief Fonction appelé si le moteur tourne depuis trop longtemps sans avoir rencontré de couple résistant assez important.
 * 
 * @attention Interrupt attached function, cannot use printf inside and can only be of 'void' type
 */
void timeout(void){
    flag_timeout = true;
    greenLED = OFF;
    redLED = OFF;
}

/**
 * @brief Fonction permettant d'initialiser les boutons et les LEDs(voir détails dans les commentaires)
 * 
 */
void initButtonsAndLEDs(){
    #if DEBUG
        printf("Initialization\n");
        printf("\n");
    #endif

    //On fixe un état logique haut sur les broches du µC accueillant les boutons
    openButton.mode(PullUp);
    closeButton.mode(PullUp);

    //On attache les fonctions aux évenements associés à l'appuie/relachement d'un bouton
    openButton.fall(&openPressed);//Appuie sur le bouton vert -> on met le prog principal en pose et on appelle la fonction openPressed
    openButton.rise(&openReleased);//Relachement du bouton vert
    closeButton.fall(&closePressed);//Appuie sur le bouton vert
    closeButton.rise(&closeReleased);//Relachement du bouton rouge
    
    greenLED = OFF;
    redLED = OFF;
    greenLED = ON;
    redLED = ON;    
}

/**
 * @brief Cette fonction permet de détacher la fonction \p timeout du 
 * Timeout \p halt lorsque un mouvement s'est terminé avant le délai maximum défini par la macro \p TIMEOUT
 * 
 * @return true si tout s'est passé correctement
 */
bool cancelTimeout(void){
    #if DEBUG
        printf("Timeout canceled\n");
    #endif

    halt.attach(NULL,TIMEOUT);
    halt.detach();
    flag_timeout = false;
    return true;
}

/**
 * @brief Cette fonction permet d'attacher la fonction \p switchLED au Ticker \p BlinkTicker pour 
 * changer l'etat des LED toutes les \p BLINK_DELAY millisecondes
 * 
 * @note c'est à la fin de cette fonction que l'on déclenche le timeout
 * 
 * @param sync permet de définir si on souhaite un clignotement synchrone ( \p SYNC ) ou asynchrone ( \p ASYNC ) des LED
 */
void startBlinking(short sync){
    #if DEBUG
        printf("Starting blinking routine...\n");
    #endif

    if(sync==-1){
        greenLED = OFF;
        blinkTicker.attach(&blinkRED,RED_BLINK_DELAY);
    }
    else if(sync==1){
        greenLED = redLED;
        blinkTicker.attach(&switchLED,BLINK_DELAY);
    }
    else{
        greenLED = redLED;
        greenLED = !redLED;
        blinkTicker.attach(&switchLED,BLINK_DELAY);
        halt.attach(&timeout,TIMEOUT);
    }
}

/**
 * @brief Cette fonction permet d'envoyer la commande d'ouverture au servomoteur et d'enclencher le clignotement des LED et 
 * le Timer \p timLoad associé aux mesures de couple résistant.
 * Elle désactive le Timer \p timBtnOpn de gestion du temps d'appuie sur le bouton \p openButton
 * 
 */
void open(void){
    #if DEBUG
        printf("Openning valve...\n");
    #endif
    //Motor->SetTorque(1);
    Motor->SetCRSpeed(OPENING_SPEED);
    //On désactive ce Timer au cas où le bouton n'ait toujours pas été relaché par l'opérateur
    timerStop(&timBtnOpn);
    timerStart(&timLoad);
    startBlinking(ASYNC);
    opening = true;
    closing = false;
    isMoving = true;
}


/**
 * @brief Cette fonction permet d'envoyer la commande d'ouverture au servomoteur et d'enclencher le clignotement des LED et 
 * le Timer \p timLoad associé aux mesures de couple résistant.
 * 
 */
void close(void){
    #if DEBUG
        printf("Closing valve...\n");
    #endif
    //Motor->SetTorque(1);
    Motor->SetCRSpeed(CLOSING_SPEED);
    timerStart(&timLoad);
    startBlinking(ASYNC);
    closing = true;
    opening = false;
    isMoving = true;
}

/**
 * @brief Fonction qui permet d'arrêter le clignotement des LED en détachant
 * la fonction \p switchLED du Ticker cadencé à \p BLINK_DELAY millisecondes 
 * 
 */
void stopBlinking(void){
    blinkTicker.detach();
}

/**
 * @brief Fonction qui permet d'initialiser le programme et permet de reprogrammer un servomoteur neuf à 40% de son couple maximum et de diminuer sa vitesse de communication si les deux boutons sont enfoncés lors de la mise sous tension
 * 
 */
void initProgram(){
    initButtonsAndLEDs();
    ThisThread::sleep_for(100ms);
    if(!openButton.read() && !closeButton.read()){//si les boutons sont enfoncés avant l'allumage...
        long baudrates[9]={1000000,500000,400000,250000,200000, 115200,57600,19200,9600};
        for(int i=0; i<9; i++){
            AX12 servo(TX, RX, BROADCAST, baudrates[i]);
            ThisThread::sleep_for(10ms);
            servo.FactoryReset();
        }
        startBlinking(RED);
        while(true){}
    }
    else if (!closeButton.read()){
        AX12 servo(TX, RX, BROADCAST, AX12_BASE_BAUD);
        servo.SetMode(1);
        servo.SetBaud(16);
        //servo.SetMaxTorque(0.4);
        while(true){
            startBlinking(SYNC);
            ThisThread::sleep_for(3000ms);
            stopBlinking();
            startBlinking(ASYNC);
            ThisThread::sleep_for(3000ms);
            stopBlinking();
        }       
    }
    else if (!openButton.read()){
        AX12 servo(TX, RX, BROADCAST, AX12_BASE_BAUD);
        //servo.SetMaxTorque(ULTIMATE_MAX_TORQUE);
        while(true){
            startBlinking(SYNC);
            ThisThread::sleep_for(3000ms);
            stopBlinking();
            startBlinking(ASYNC);
            ThisThread::sleep_for(3000ms);
            stopBlinking();
        }       
    }
    else{
        initMotor(AX12_BAUD);
        #if DEBUG
            printf("Initialisation terminée\n");
        #endif
    }
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
    cancelTimeout();
    timerStop(&timBtnOpn);
    timerStop(&timLoad);
    isMoving = false;
    opening = false;
    closing = false;
}

/**
 * @brief Fonction qui lit la valeur du couple résistant sur le moteur. Si le pourcentage de couple 
 * dépasse \p MAX_TORQUE, on stoppe le clignotement des LED, on allume la LED correspondant à l'action
 * qui vient d'avoir lieu (ouverture ou fermeture ?) et on arrête le moteur.
 * 
 * LED rouge allumée: vanne fermée
 * LED verte allumée : vanne ouverte
 * 
 * @param compteur variable qui compte le nombre de mesures consécutives satisaifaisant le test.
 */
void checkLoad(void){
    //RAZ du timer
    timLoad.reset();
    //Test sur le couple résistant
    if(abs(Motor->GetLoad()) > LOAD_MAX_TORQUE){
        counter++;
        #if CALIBRATION || DEBUG
            printf("Compteur de validation: %i\n",counter);
        #endif
    } 
    else counter = 0;
    #if CALIBRATION || DEBUG
        char message[20]={0};
        float tmp = 100*abs(Motor->GetLoad());
        printf("Load : %f\n\n",tmp);
    #endif
    if(counter>=VALIDATION){
        #if CALIBRATION || DEBUG
            printf("Ok, shutting down servo...\n");
        #endif

        counter = 0;
        stopBlinking();
        if(opening && !closing){
            greenLED = ON;
            redLED = OFF;
        }
        else if (closing && !opening){
            greenLED = OFF;
            redLED = ON;
        }
        stopMotor();
    }
}

#endif