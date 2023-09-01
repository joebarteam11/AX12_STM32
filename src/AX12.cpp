/* mbed AX-12+ Servo Library
 *
 * Copyright (c) 2010, cstyles (http://mbed.org)
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "mbed.h"
#include "AX12.h"

AX12::AX12(PinName tx, PinName rx, int ID, int baud)
        : _ax12(tx,rx,baud) 
{
    _baud = baud;
    _ID = ID;
    _ax12.baud(_baud);
}

int AX12::FactoryReset (void) {
    #ifdef AX12_DEBUG
        printf("Resetting to factory...\n");
    #endif

    return (write(_ID, 0, 1, 0, 2));
}
// Set the mode of the servo
//  0 = Positional (0-300 degrees)
//  1 = Rotational -1 to 1 speed
int AX12::SetMode(int mode) {

    if (mode == 1) { // set CR
        SetCWLimit(0);
        SetCCWLimit(0);
        SetCRSpeed(0.0);
    } else {
        SetCWLimit(0);
        SetCCWLimit(300);
        SetCRSpeed(0.0);
    }
    return(0);
}


// if flag[0] is set, were blocking
// if flag[1] is set, we're registering
// they are mutually exclusive operations
int AX12::SetGoal(int degrees, int flags) {

    char reg_flag = 0;
    char data[2];

    // set the flag is only the register bit is set in the flag
    if (flags == 0x2) {
        reg_flag = 1;
    }

    // 1023 / 300 * degrees
    short goal = (1023 * degrees) / 300;
    if (AX12_DEBUG) {
        printf("SetGoal to 0x%x\n",goal);
    }

    data[0] = goal & 0xff; // bottom 8 bits
    data[1] = goal >> 8;   // top 8 bits

    // write the packet, return the error code
    int rVal = write(_ID, AX12_REG_GOAL_POSITION, 2, data, reg_flag);

    if (flags == 1) {
        // block until it comes to a halt
        while (isMoving()) {}
    }
    return(rVal);
}

//Set AX12 baud [default 1000000]
int AX12::SetBaud (int baud) {

    char data[1];
    data[0] = baud;

#ifdef AX12_DEBUG
    printf("Setting Baud rate to %d\n",baud);
#endif

    return (write(0xFE, AX12_REG_BAUD, 1, data));

}

// Set continuous rotation speed from -1 to 1
int AX12::SetCRSpeed(float speed) {

    // bit 10     = direction, 0 = CCW, 1=CW
    // bits 9-0   = Speed
    char data[2];

    int goal = int(0x3ff * abs(speed));

    // Set direction CW if we have a negative speed
    if (speed < 0) {
        goal |= (0x1 << 10);
    }

    data[0] = goal & 0xff; // bottom 8 bits
    data[1] = goal >> 8;   // top 8 bits

    // write the packet, return the error code
    int rVal = write(_ID, AX12_REG_MOVING_SPEED, 2, data);

    return(rVal);
}


int AX12::SetCWLimit (int degrees) {

    char data[2];
    
    // 1023 / 300 * degrees
    short limit = (1023 * degrees) / 300;

    if (AX12_DEBUG) {
        printf("SetCWLimit to 0x%x\n",limit);
    }

    data[0] = limit & 0xff; // bottom 8 bits
    data[1] = limit >> 8;   // top 8 bits

    // write the packet, return the error code
    return (write(_ID, AX12_REG_CW_LIMIT, 2, data));
}


int AX12::SetCCWLimit (int degrees) {

    char data[2];

    // 1023 / 300 * degrees
    short limit = (1023 * degrees) / 300;

    if (AX12_DEBUG) {
        printf("SetCCWLimit to 0x%x\n",limit);
    }

    data[0] = limit & 0xff; // bottom 8 bits
    data[1] = limit >> 8;   // top 8 bits

    // write the packet, return the error code
    return (write(_ID, AX12_REG_CCW_LIMIT, 2, data));
}

//Permet d'activer/désactiver le couple du servo
int AX12::SetTorque (bool state){
    char data[1];
    data[0] = state;

    if(AX12_DEBUG){
        printf("Setting torque to %i\n",state);
    }
    // write the packet, return the error code
    return (write(_ID, AX12_REG_ENABLE_TORQUE, 1, data));
}

int AX12::SetMaxTorque (float percentage) {
    char data[2];
    short limit = 1023 * (float)percentage;

    if (AX12_DEBUG) {
        printf("SetMaxTorque to 0x%x\n",limit);
    }

    data[0] = limit & 0xff; // bottom 8 bits
    data[1] = limit >> 8;   // top 8 bits

    // write the packet, return the error code
    return (write(_ID, AX12_REG_MAX_TORQUE, 2, data));

}

int AX12::SetID (int NewID, int CurrentID) {

    char data[1];
    data[0] = NewID;
    if (AX12_DEBUG) {
        printf("Setting ID from 0x%x to 0x%x\n",CurrentID,NewID);
    }
    return (write(CurrentID, AX12_REG_ID, 1, data));

}


// return 1 is the servo is still in flight
int AX12::isMoving(void) {

    char data[1];
    read(_ID,AX12_REG_MOVING,1,data);
    return(data[0]);
}


void AX12::trigger(void) {

    char TxBuf[16];
    char sum = 0;

    if (AX12_TRIGGER_DEBUG) {
        printf("\nTriggered\n");
    }

    // Build the TxPacket first in RAM, then we'll send in one go
    if (AX12_TRIGGER_DEBUG) {
        printf("\nTrigger Packet\n  Header : 0xFF, 0xFF\n");
    }

    TxBuf[0] = 0xFF;
    TxBuf[1] = 0xFF;

    // ID - Broadcast
    TxBuf[2] = 0xFE;
    sum += TxBuf[2];

    if (AX12_TRIGGER_DEBUG) {
        printf("  ID : %d\n",TxBuf[2]);
    }

    // Length
    TxBuf[3] = 0x02;
    sum += TxBuf[3];
    if (AX12_TRIGGER_DEBUG) {
        printf("  Length %d\n",TxBuf[3]);
    }

    // Instruction - ACTION
    TxBuf[4] = 0x04;
    sum += TxBuf[4];
    if (AX12_TRIGGER_DEBUG) {
        printf("  Instruction 0x%X\n",TxBuf[5]);
    }

    // Checksum
    TxBuf[5] = 0xFF - sum;
    if (AX12_TRIGGER_DEBUG) {
        printf("  Checksum 0x%X\n",TxBuf[5]);
    }

    // Transmit the packet in one burst with no pausing
    for (int i = 0; i < 6 ; i++) {
        _ax12.putc(TxBuf[i]);
    }
    // This is a broadcast packet, so there will be no reply

    return;
}


float AX12::GetPosition(void) {

    if (AX12_DEBUG) {
        printf("\nGetPosition(%d)",_ID);
    }

    char data[2];

    int ErrorCode = read(_ID, AX12_REG_POSITION, 2, data);
    short position = data[0] + (data[1] << 8);
    float angle = (position * 300)/1023;

    return (angle);
}


float AX12::GetTemp (void) {

    if (AX12_DEBUG) {
        printf("\nGetTemp(%d)",_ID);
    }
    char data[1];
    int ErrorCode = read(_ID, AX12_REG_TEMP, 1, data);
    float temp = data[0];
    return(temp);
}


float AX12::GetVolts (void) {
    if (AX12_DEBUG) {
        printf("\nGetVolts(%d)",_ID);
    }
    char data[1];
    int ErrorCode = read(_ID, AX12_REG_VOLTS, 1, data);
    float volts = data[0]/10.0;
    return(volts);
}


float AX12::GetLoad (void) {

    if (AX12_DEBUG) {
        printf("\nGetLoad(%d)",_ID);
    }

    char data[2];

    int ErrorCode = read(_ID, AX12_REG_LOAD, 2, data);
    short val = data[0] + (data[1] << 8);
    if(AX12_CALIB){
            printf("Raw value: %i\n",val);
        }
    if (val <=1023) {
        if(AX12_DEBUG){
            printf("CounterClockwise: %i\n",val);
        }
        return (-val/1023.0);
    }
    else {
        if(AX12_DEBUG){
            printf("Clockwise: %i\n",val);
        }
        return ((val-1024.0)/1023.0);
    }

}


int AX12::read(int ID, int start, int bytes, char* data) {

    char PacketLength = 0x4;
    char TxBuf[16];
    char sum = 0;
    char Status[16];

    Status[4] = 0xFE; // return code

    if (AX12_READ_DEBUG) {
        printf("\nread(%d,0x%x,%d,data)\n",ID,start,bytes);
    }

    // Build the TxPacket first in RAM, then we'll send in one go
    if (AX12_READ_DEBUG) {
        printf("\nInstruction Packet\n  Header : 0xFF, 0xFF\n");
    }

    TxBuf[0] = 0xff;
    TxBuf[1] = 0xff;

    // ID
    TxBuf[2] = ID;
    sum += TxBuf[2];
    if (AX12_READ_DEBUG) {
        printf("  ID : %d\n",TxBuf[2]);
    }

    // Packet Length
    TxBuf[3] = PacketLength;    // Length = 4 ; 2 + 1 (start) = 1 (bytes)
    sum += TxBuf[3];            // Accululate the packet sum
    if (AX12_READ_DEBUG) {
        printf("  Length : 0x%x\n",TxBuf[3]);
    }

    // Instruction - Read
    TxBuf[4] = 0x2;
    sum += TxBuf[4];
    if (AX12_READ_DEBUG) {
        printf("  Instruction : 0x%x\n",TxBuf[4]);
    }

    // Start Address
    TxBuf[5] = start;
    sum += TxBuf[5];
    if (AX12_READ_DEBUG) {
        printf("  Start Address : 0x%x\n",TxBuf[5]);
    }

    // Bytes to read
    TxBuf[6] = bytes;
    sum += TxBuf[6];
    if (AX12_READ_DEBUG) {
        printf("  No bytes : 0x%x\n",TxBuf[6]);
    }

    // Checksum
    TxBuf[7] = 0xFF - sum;
    if (AX12_READ_DEBUG) {
        printf("  Checksum : 0x%x\n",TxBuf[7]);
    }

    // Transmit the packet in one burst with no pausing
    for (int i = 0; i<8 ; i++) {
        _ax12.putc(TxBuf[i]);
    }

    // Wait for the bytes to be transmitted
    wait_us(20);

    // Skip if the read was to the broadcast address
    if (_ID != 0xFE) {
        
        // Receive the Status packet 6+ number of bytes read
        //ThisThread::sleep_for(100ms);
        wait_us(100*1000);
        for (int i=0; i<(6+bytes) ; i++){          
            Status[i] = (char)_ax12.getc(i);
        }

        // Copy the data from Status into data for return
        for (int i=0; i < Status[3]-2 ; i++) {
            data[i] = Status[5+i];
        }

        if (AX12_READ_DEBUG) {
            printf("\nStatus Packet\n");
            printf("  Header : 0x%x\n",Status[0]);
            printf("  Header : 0x%x\n",Status[1]);
            printf("  ID : 0x%x\n",Status[2]);
            printf("  Length : 0x%x\n",Status[3]);
            printf("  Error Code : 0x%x\n",Status[4]);

            for (int i=0; i < Status[3]-2 ; i++) {
                printf("  Data : 0x%x\n",Status[5+i]);
            }

            printf("  Checksum : 0x%x\n",Status[5+(Status[3]-2)]);
        }

    } // if (ID!=0xFE)

    return(Status[4]);
}


int AX12:: write(int ID, int start, int bytes, char* data, int flag) {
// 0xff, 0xff, ID, Length, Intruction(write), Address, Param(s), Checksum

    char TxBuf[16];
    char sum = 0;
    char Status[6];

    if (AX12_WRITE_DEBUG) {
        printf("\nwrite(%d,0x%x,%d,data,%d)\n",ID,start,bytes,flag);
    }

    // Build the TxPacket first in RAM, then we'll send in one go
    if (AX12_WRITE_DEBUG) {
        printf("\nInstruction Packet\n  Header : 0xFF, 0xFF\n");
    }

    TxBuf[0] = 0xff;
    TxBuf[1] = 0xff;

    // ID
    TxBuf[2] = ID;
    sum += TxBuf[2];

    if (AX12_WRITE_DEBUG) {
        printf("  ID : %d\n",TxBuf[2]);
    }

    // packet Length
    TxBuf[3] = 3+bytes;
    sum += TxBuf[3];

    if (AX12_WRITE_DEBUG) {
        printf("  Length : %d\n",TxBuf[3]);
    }

    // Instruction
    if (flag == 1) {
        TxBuf[4]=0x04;
        sum += TxBuf[4];
    } else {
        TxBuf[4]=0x03;
        sum += TxBuf[4];
    }

    if (AX12_WRITE_DEBUG) {
        printf("  Instruction : 0x%x\n",TxBuf[4]);
    }

    // Start Address
    TxBuf[5] = start;
    sum += TxBuf[5];
    if (AX12_WRITE_DEBUG) {
        printf("  Start : 0x%x\n",TxBuf[5]);
    }

    // data
    for (char i=0; i<bytes ; i++) {
        TxBuf[6+i] = data[i];
        sum += TxBuf[6+i];
        if (AX12_WRITE_DEBUG) {
            printf("  Data : 0x%x\n",TxBuf[6+i]);
        }
    }

    // checksum
    TxBuf[6+bytes] = 0xFF - sum;
    if (AX12_WRITE_DEBUG) {
        printf("  Checksum : 0x%x\n",TxBuf[6+bytes]);
    }
    
    int count = 7+bytes;

    if(flag == 2){
        TxBuf[0]=0xFF;
        TxBuf[1]=0xFF;
        TxBuf[2]=_ID;
        TxBuf[3]=0x02;
        TxBuf[4]=0x06;
        TxBuf[5]=0xFF-(TxBuf[2]+TxBuf[3]+TxBuf[4]);
        count = 6;
    }

    // Transmit the packet in one burst with no pausing
    for (int i = 0; i < count ; i++) {
        int retu = _ax12.putc(TxBuf[i]);
    }

    // Wait for data to transmit
    wait_us(20);

    // make sure we have a valid return
    Status[4]=0x00;

    // we'll only get a reply if it was not broadcast
    if (_ID!=0xFE) {

        // response is always 6 bytes
        // 0xFF, 0xFF, ID, Length Error, Param(s) Checksum
        //ThisThread::sleep_for(100ms);
        wait_us(100*1000);
        for (int i=0; i < 6 ; i++) {
            Status[i] = (char)_ax12.getc(i);
        }
        
        // Build the TxPacket first in RAM, then we'll send in one go
        if (AX12_WRITE_DEBUG) {
            printf("\nStatus Packet\n  Header : 0x%X, 0x%X\n",Status[0],Status[1]);
            printf("  ID : %d\n",Status[2]);
            printf("  Length : %d\n",Status[3]);
            printf("  Error : 0x%x\n",Status[4]);
            printf("  Checksum : 0x%x\n",Status[5]);
        }


    }

    return(Status[4]); // return error code

}