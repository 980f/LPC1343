#include "gpio.h"
#include "lpcperipheral.h" // for clock enable.

TestInitSequence gpio24 InitStep(1024) (24);
TestInitSequence gpio10 InitStep(1010) (10);


using namespace LPC;

/*
curiously: if you program a level triggered interrupt with low active level then leave it disabled
one can still poll the 'active' level in the raw interrupt status register.
likewise one can capture edges without enabling the interrupt.

Reading the pin via raw interrupt sense is only slightly more expensive than using the address==mask access.

*/

//  SFR DIR; //0x500p8000

//  SFR IS;  GPIOIS R/W 0x8004 Interrupt sense register for port n 0x00  1=level, 0=edge
//  SFR IBE; GPIOIBE R/W 0x8008 Interrupt both edges register for port n 0x00
//  SFR IEV; GPIOIEV R/W 0x800C Interrupt event register for port n 0x00

//  SFR IE;  GPIOIE R/W 0x8010 Interrupt mask register for port n 0x00
//  SFR RIS; GPIORIS R 0x8014 Raw interrupt status register for port n 0x00
//  SFR MIS; GPIOMIS R 0x8018 Masked interrupt status register for port n 0x00
//  SFR IC;  GPIOIC W 0x801C Interrupt clear register for port n 0x00

void GPIO::setDirection(bool output)const{
  GPIO::IrqControl myIrqc(*this);
  myIrqc.setDirection(output);
}

//void GPIO::Init(void) __attribute__((section(InitHardware-1)));//should precede any attempt to configure a hardware unit that uses pins.
/** turn clock on to gpio and iocon blocks. */
void GPIO::Init(void){
  enableClock(6); // gpio clock bit on,usually already is.
  enableClock(16); // iocon has to be turned on somewhere, might as well be here.
}

void GPIO::irq(bool enable)const{
GPIO::IrqControl myIrqc(*this);
  myIrqc.irq(enable);
}

void GPIO::irqAcknowledge()const{
  GPIO::IrqControl myIrqc(*this);
  myIrqc.irqAcknowledge();
}

void GPIO::setIrqStyle(GPIO::IrqStyle style, bool andEnable)const{
  GPIO::IrqControl myIrqc(*this);
  myIrqc.setIrqStyle(style,andEnable);
}


void GPIO::IrqControl::setIrqStyle(GPIO::IrqStyle style, bool andEnable)const{
  //disable before recongifuring
  clearRegister(0x10);


  //  atAddress(regbase)&=~mask; //force to input, user can make it an output and interrupt themselves later if they so wish.
  switch(style){
  case NotAnInterrupt : // in case someone forgets to explicitly select a mode, or we wish to dynamically deconfigure.
    //nothing to do, we have disabled it above so it doesn't matter if we leave it somewhat configured.
    break;
  case AnyEdge: // edge, either edge, input mode buslatch
    clearRegister(4); //pro forma clear of level sense
    setRegister(8);  //double edge
    clearRegister(12);//pro forma clear of direction
    break;
  case LowActive: // level, pulled up
    setRegister(4);
    clearRegister(8);
    clearRegister(12);
    break;
  case HighActive: // level, pulled down
    setRegister(4);
    clearRegister(8);
    setRegister(12);
    break;
  case LowEdge: // edge, pulled up
    clearRegister(4);
    clearRegister(8);
    clearRegister(12);
    break;
  case HighEdge:// edge, pulled down
    clearRegister(4);
    clearRegister(8);
    setRegister(12);
    break;
  }

  if(andEnable){
    setRegister(0x10);
  }
}

void GPIO::IrqControl::irq(bool enable) const{
  assignRegister(0x10,enable);
}

void GPIO::IrqControl::irqAcknowledge()const{
  setRegister(0x1C);
}

void GPIO::IrqControl::setDirection(bool output)const{
  assignRegister(0,output);
}


//////////////////////////

void GpioField::setDirection(bool forOutput)const{
  u16 mask= reinterpret_cast<unsigned>(address)>>2;
  u32 &directionRegister=atAddress((reinterpret_cast<unsigned>(address)&~0x7FFF)|0x8000);
  if(forOutput){
    directionRegister|= mask;
  } else {
    directionRegister &= ~mask;
  }
}

GpioField::GpioField(PortNumber portNum, unsigned msb, unsigned lsb):
  address( portBase(portNum) + ((1 << (msb + 3)) - (1 << (lsb + 2)))),
lsb(lsb){ /*empty*/}

