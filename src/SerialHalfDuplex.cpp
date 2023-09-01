/*
 * mbed Microcontroller Library
 * Copyright (c) 2006-2012 ARM Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * NOTE: This is an unsupported legacy untested library.
 */
#include "SerialHalfDuplex.h"

#include "mbed.h"
#include "pinmap.h"
#include "serial_api.h"
#include "gpio_api.h"

namespace mbed {

/**
 * @brief Constructeur de l'objet SerialHalfDuplex
 * 
 * @param tx pin de transmission
 * @param rx pin de reception
 * @param baud vitesse de communication (en bps)
 * 
 * @attention Les pins \p tx et \p rx doivent être physiquement reliés entre eux
 */
SerialHalfDuplex::SerialHalfDuplex(PinName tx, PinName rx, int baud)
    : SerialBase(tx, rx, baud)
{
    idx = 0;
    _txpin = tx;
    _baud = baud;
    DigitalIn TXPIN(_txpin);    // set as input
    pin_mode(_txpin, PullNone); // no pull
    pin_function(_txpin, 0);    // set as gpio
    SerialBase::baud(_baud);
    SerialBase::format(8,SerialBase::None,1);
    SerialBase::attach(callback(this,&SerialHalfDuplex::RXinterrupt),SerialBase::RxIrq);
}

// To transmit a byte in half duplex mode:
// 1. Disable interrupts, so we don't trigger on loopback byte
// 2. Set tx pin to UART out
// 3. Transmit byte as normal
// 4. Read back byte from looped back tx pin - this both confirms that the
//    transmit has occurred, and also clears the byte from the buffer.
// 5. Return pin to input mode
// 6. Re-enable interrupts

/**
 * @brief Cette fonction remplie le buffer de reception \p buffer avec des '0'
 * 
 * @param buffer est un pointeur vers le buffer à modifier
 */
bool SerialHalfDuplex::eraseBuffer(int *buffer){
    for(int i=0; i<BUF_SIZE;i++){
      buffer[i] = 0;
    }
    return true;
}

// Send one character to the serial port
/**
 * @brief Cette fonction permet d'envoyer un caractère sur la liaison série en mode Half-Duplex
 * 
 * @param c le caractère à envoyer
 * @return le caractère qui a été récupéré suite à l'envoi
 */
int SerialHalfDuplex::putc(int c){
    int retc;
    eraseBuffer(buf); // Peut-être pas utile mais on sait jamais...

    //Disable the interruption while sending the character
    core_util_critical_section_enter();

    serial_pinout_tx(_txpin);
    
    SerialBase::_base_putc(c);
    retc = SerialBase::_base_getc();     // reading also clears any interrupt
    
    pin_function(_txpin, 0);

    core_util_critical_section_exit();
    
    return retc;
}

//Triggered if sthg happen on the receiving pin
/**
 * @brief Cette fonction est appelée dès que quelque chose est disponible en réception sur le port série. 
 * Elle stocke les caractères reçus dans les cases vides (i.e. qui contienne un '0') du buffer \p buf , dans l'ordre où ceux-ci sont arrivés
 * 
 * @attention Le code d'erreur renvoyé par le PacketStatus de l'AX12 étant dans la plupart des cas '0', on remplace ca valeur par un '1' pour éviter un décalage dans les index
 * Une meilleure méthode serait la binevenue...
 */
void SerialHalfDuplex::RXinterrupt(void){
    while(readable()){
        buf[idx] = _base_getc();
        idx++;
    }    
}

// Take a character into the reception buffer and replace it with à 'z'
/**
 * @brief Cette fonction permet d'accéder au contenu du buffer de réception et remplace le caractère par un 'z' une fois la valeur récupérée
 * 
 * @param i index du caractère que l'on veut récupérer dans le buffer \p buf de réception
 * @return le caractère contenu dans la case correspondante à l'index \p i.
 */
int SerialHalfDuplex::getc(int i){
    idx = 0;  
    int retc = buf[i];
    buf[i]=0;
    return retc;    
}

} // End namespace