#include "stm32.h"

/** return the bitband address of a bit of a given address*/
volatile u32 *bandFor(volatile void *dcb, unsigned int bitnum){ //5: 2^5 bits per word. The 0xE0.. and 0x02 .. are cortex-M3 architecture values.
  return reinterpret_cast <u32 *> ((bitnum << 2) | (((u32) dcb) << 5) | ((((u32) dcb) & 0xE0000000) | 0x02000000));
}

void APBdevice::reset() const {
  sfrbit(resetter, rccBase(12), slot);
  *resetter = 1;
  *resetter = 0; //manual is ambiguous as to whether reset is a command or a state.
}

void APBdevice::setClockEnable(bool on) const {
  sfrbit(clocker, rccBase(24), slot);
  *clocker = on;
}

bool APBdevice::isEnabled() const {
  sfrbit(clocker, rccBase(24), slot);
  return *clocker;
}

void APBdevice::init() const { //frequently used combination of reset then enable clock.
  reset();
  setClockEnable();
}

u32 APBdevice::getClockRate() const {
  return clockRate(bus2 + 1);
}


//////////////////

#ifdef __linux__  //faking it
  u32 fakeram[2][1024];
#endif
