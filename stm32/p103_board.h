#ifndef P103_BOARD_H
#define P103_BOARD_H

#include "gpio.h"

/**
olimex/iar stm32-P103 board devices
*/
struct P103_board {
  OutputPin led; //low active.
  InputPin button; //also can be the wake_up special function
  //B6,B7 are pulled up for use with I2C
  //PB10,11 are pulled up
  //PA4 spi1 ss
  //PB15 sdcard miso
  P103_board();
};

#endif // P103_BOARD_H