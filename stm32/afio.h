#ifndef AFIO_H
#define AFIO_H

#include "stm32.h"

struct AfioBand {
  unsigned int event[32]; //skipping event selector
  unsigned int spi1;
  unsigned int i2c1;
  unsigned int uart1;
  unsigned int uart2;
  unsigned int uart3[2];
  unsigned int tim1[2];
  unsigned int tim2_12;
  unsigned int tim2_34;
  unsigned int tim3[2];
  unsigned int tim4;
  unsigned int can1[2];
  unsigned int PD01;
  //more when we get around to needing them
};

struct AfioEvent {
  unsigned evPin:4;
  unsigned evPort:3; //letter for port -'A'
  unsigned evEnable:1;
  unsigned :24;
};

struct AfioRemap {
  unsigned spi1:1;
  unsigned i2c1:1;
  unsigned uart1:1;
  unsigned uart2:1;
  unsigned uart3:2;
  unsigned tim1:2;
  unsigned tim2:2;
  unsigned tim3:2;
  unsigned tim4:1;
  unsigned can1:2;
  unsigned PD01:1;
  unsigned tim5:1;
  unsigned :4;
  unsigned ethernet:1;
  unsigned can2:1;
  unsigned rmii:1;
  unsigned swj:3; //jtag debugger port, take care! seems like with 10 pin port this should be a 1
  unsigned :1;
  unsigned spi3:1;
  unsigned t2itr:1;
  unsigned ptp:1;//
};

#include "shadow.h"
class AfioManager : public APBdevice,public Shadowed<AfioRemap,u32> {
public:
  AfioBand &b;
public:
  AfioManager(void);

};
extern AfioManager theAfioManager;//named per soliton() but address is computed via constructor so we aren't using that macro

#endif // AFIO_H
