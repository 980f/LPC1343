#pragma once

#include "gpio.h" // to declare buttons and lamps.

class P1343devkit {
public:
  LPC::InputPin<2, 9> button;
  //other button is 'wakeup'
  //reset button is pio0/0, if reset functionality is defeated.
  LPC::OutputPin<3, 0> led0;
  LPC::OutputPin<3, 1> led1;
  LPC::OutputPin<3, 2> led2;
  LPC::OutputPin<3, 3> led3;
  LPC::OutputPin<2, 4> led4;
  LPC::OutputPin<2, 5> led5;
  LPC::OutputPin<2, 6> led6;
  LPC::OutputPin<2, 7> led7;
  P1343devkit();
  ~P1343devkit();
  /** set lamps as an 8-bit number, not particular swift in execution since they are scattered about the i/o space*/
  int operator =(int lamp );
  /** set led by ordinal.*/
  void led(unsigned which);
  /** invert state of one led */
  void toggleLed(unsigned which=0);
};
