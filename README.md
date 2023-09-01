# AX12_STM32
Library for Dynamixel AX12 usage with STM32 boards 

Tested on Nucleo F303k8 an F446RE

The AX12 servo is connected directly to the STM32 board, no adapter is needed.

To run this code, connect the AX12 data pin to **both the RX and TX** pins of the STM32 board. The AX12 is powered externaly but with a common ground with the STM32 board.