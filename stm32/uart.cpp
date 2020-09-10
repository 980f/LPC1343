#ifndef DEVICE
#error "you must define DEVICE to something like 103 or 407"
#endif

#include "uart.h"
#include "minimath.h"

#if DEVICE == 103
#include "gpio.h"
#elif DEVICE == 407

#include "gpiof4.h"

#endif

void Uart::setBaudrate(unsigned desired) {
  //note: the ST manuals are chocked full of nonsense with respect to baud rate setting.
  // just take the input clock rate and divide by the baud rate, round to the nearest integer, and put that value into the 16 bit BRR as if it were a simple integer.
  // you can ignore all the jabber about fractional baudrate and all the dicing and splicing, which under close inspection of ST's code does absolutely nothing.
  unsigned osc = getClockRate();
  unsigned newbaud = rate(osc, desired);

  if (newbaud != dcb.BRR) {
    b.enable = 0;
    dcb.BRR = newbaud;
  }
} /* setBaudrate */


unsigned Uart::bitsPerSecond() const {
  unsigned divisor = dcb.BRR;
  auto osc = getClockRate();

  return rate(osc, divisor);
}

/**
  * st's manual fails to note that 8 data bits + parity bit means you must set the word length to 9!
  * So: 9 if sending 9 bits and no parity or 8 bits with parity. Set to 8 for 7 bits with parity.
  */
void Uart::setParams(unsigned baud, unsigned numbits, char parityNEO, bool longStop, bool shortStop) { //19200,8,n,1
  b.enable = 0;
  //decode char to the control bits:
  b.parityOdd = parityNEO & (1 << 1); //bit 1 is high for Oh versus low for E
  b.parityEnable = parityNEO & 1; ////lsb is 1 for E or Oh, 0 for N.
  b._9bits = numbits == 9 || (numbits == 8 && b.parityEnable);
  b.halfStop = shortStop;
  b.doubleStop = longStop;
  setBaudrate(baud);
}

/** part of char time calculation, includes stop and start and parity, not just payload bits */
unsigned Uart::bitsPerByte(void) const {
  unsigned bits = 1; //the start bit

  if (b.doubleStop) {
    bits += 2;
  }
  if (b.halfStop) { //sorry, not rigged to deal with 1.5 stop bits,
    bits -= 0; //should be decreasing by 0.5 bits.
  }
  if (b._9bits) {
    return bits + 9;
  }
  return bits + 8; //todo:2 7 bits no parity!
} /* bitsPerByte */

/** timer ticks required to move the given number of chars. Involves numbits and baud etc.*/
unsigned Uart::ticksForChars(unsigned charcount) const {
  return charcount * bitsPerByte() * dcb.BRR;
}

void Uart::beReceiving(bool yes) {
  b.dataAvailableIE = yes; //which is innocuous if interrupts aren't enabled and it is cheaper to set it then to test whether it should be set.
  b.enableReceiver = yes;
  b.enable = yes || b.enableTransmitter;
}

void Uart::beTransmitting(bool yes) { //NB: do not call this when the last character might be on the wire.
  b.transmitCompleteIE = 0;  //so that we only check this on the last character of a packet.
  b.enable = yes || b.enableReceiver;
  b.transmitAvailableIE = yes; //and the isr will send the first char, we don't need to 'prime' DR here.
}

void Uart::reconfigure(unsigned baud, unsigned numbits, char parityNEO, bool longStop, bool shortStop) { //19200,8,n,1
  APBdevice::init();
  setParams(baud, numbits, parityNEO, longStop, shortStop);
}

void Uart::init(unsigned baud, char parityNEO, unsigned numbits) {
  reconfigure(baud, numbits, parityNEO);
  takePins(true, true); //after reconfigure as that reset's all usart settings including the ones this fiddles with.
  irq.enable(); //the reconfigure disables all interrupt sources, so enabling interrupts here won't cause any.
}

/*
F103: U1 = 1:14, others on 2:luno+15
F407: U1 = 2:  U2..5 = 1:lu+15   U6=2:
*/

Uart::Uart(unsigned stluno, unsigned alt) :
#if DEVICE == 103
  APBdevice(stluno>1 ? 1: 2, stluno>1 ? (stluno + 15): 14)
#elif DEVICE == 407
  APBdevice((stluno == 1 || stluno == 6) ? 2 : 1, (stluno == 1 ? 4 : stluno == 6 ? 5 : (stluno + 15)))
#endif
  , b(*reinterpret_cast <volatile UartBand *> (bandAddress)), dcb(*reinterpret_cast <volatile USART_DCB *> (blockAddress))
//the irq's are the same for the same luno, just needed to add uart6 for F407:
  , irq((stluno <= 3 ? 36 : stluno <= 5 ? 48 : 65) + stluno) //37,38,39  52,53 71
  , stluno(stluno), altpins(alt) {
  //not grabbing pins quite yet as we may be using a spare uart internally as a funky timer.
}

#if DEVICE == 103
/**
rx's are coded as floating inputs as RX is presumed to always be driven
hsin's are coded as pulled up inputs in case the pin is left unconnected, although really someone
should actively configure the pin's presence and not rely upon our weak pullup.
*/
//got tired of finesse:
#define makeTxPin(P,b) Pin(P, b).FN(rxtxSpeedRange)

static void grabInput(const Port &PX,int bn, char udf) {
//assignment is to get compiler to not prune the code (although it kindly warned us that it did).
  Pin(PX, bn).DI(udf)=1;
}

#define pinMux(stnum) theAfioManager.remap.uart##stnum=altpins;theAfioManager.remap.update()

void Uart::takePins(bool tx, bool rx, bool hsout, bool hsin){
  Portcode::Slew rxtxSpeedRange=bitsPerSecond()>460e3?Portcode::Slew::fast:Portcode::Slew::slow;//pin speed codes, 2Mhz rounded off signal too much past 460kbaud

  switch(stluno) {
  case 1:
    pinMux(1);
    if(hsin) {
      grabInput(PA, 11,'U');
    }
    if(hsout) {
      Pin(PA, 12).FN();
    }
    switch(altpins) {
    case 0 :
      if(tx) {
        makeTxPin(PA, 9);
      }
      if(rx) {
        grabInput(PA, 10,'F');
      }
      break;
    case 1 :
      if(tx) {
        makeTxPin(PB, 6);
      }
      if(rx) {
        grabInput(PB, 7,'F');
      }
      break;
    } /* switch */
    break;
  case 2:
    pinMux(2);
    switch(altpins) {
    case 0:
      if(tx) {
        makeTxPin(PA, 2);
      }
      if(rx) {
        grabInput(PA, 3,'F');
      }
      if(hsin) {
        grabInput(PA, 0,'U');
      }
      if(hsout) {
        Pin(PA, 1).FN();
      }
      break;
    case 1:
      //todo:3 set the AFIO remap field
      if(tx) {
        makeTxPin(PD, 5);
      }
      if(rx) {
        grabInput(PD, 6,'F');
      }
      if(hsin) {
        grabInput(PD, 3,'U');
      }
      if(hsout) {
        Pin(PD, 4).FN();
      }
      break;
    } /* switch */
    break;
  case 3:
    pinMux(3);
    //todo:3 add hs lines
    switch(altpins) {
    case 0:
      if(tx) {
        makeTxPin(PB, 10);
      }
      if(rx) {
        grabInput(PB, 11,'F');
      }
      break;
    case 1:
      if(tx) {
        makeTxPin(PC, 10);
      }
      if(rx) {
        grabInput(PC, 11,'F');
      }
      break;
    case 3:
      if(tx) {
        makeTxPin(PD, 8);
      }
      if(rx) {
        grabInput(PD, 9,'F');
      }
      break;
    } /* switch */
    break;
    //todo:3 uarts 4 and 5, which don't suffer from remap options.
  } /* switch */
  b.enableTransmitter = tx; //else the tx pin floats!
  //... would only dynamically play with this for single wire half duplexing as in some SPI modes, better to use a transceiver and
  //... a gpio pin than to play with the chip's own pin.
} /* takePins */
#endif

#if DEVICE == 407
//pin has selector for its function and selecting the function takes care of other aspects of it.
#define makeTxPin(P, b) Pin(P, b).FN(7,rxtxSpeedRange,'F')
#define makeRxPin(P, b) Pin(P, b).FN(7,rxtxSpeedRange,'U')

void Uart::takePins(bool tx, bool rx, bool hsout, bool hsin) {
  PinOptions::Slew rxtxSpeedRange = bitsPerSecond() > 460e3 ? PinOptions::Slew::fast : PinOptions::Slew::slow;  //pin speed codes, 2Mhz rounded off signal too much past 460kbaud
  switch (stluno) {
  case 1:
    if (tx) {
      if (altpins == 1) {
        makeTxPin(PB, 6);
      } else {
        makeTxPin(PA, 9);
      }
    }
    if (rx) {
      if (altpins == 1) {
        makeRxPin(PB, 7);
      } else {
        makeRxPin(PA, 10);
      }
    }
    break;
  case 2:
    if (tx) {
      if (altpins == 1) {
        makeTxPin(PD, 5);
      } else {
        makeTxPin(PA, 2);
      }
    }
    if (rx) {
      if (altpins == 1) {
        makeRxPin(PD, 6);
      } else {
        makeRxPin(PA, 3);
      }
    }

    break;
  }
}

#endif
//End of file
