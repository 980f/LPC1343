#ifndef UART_H
#define UART_H
//#include "eztypes.h"

namespace LPC {

/** uart manager class, not to be confused with hardware register class.
Extend/derive from it for interrupt driven usage.
*/

class Uart {
  /** the isr. a stub is locally generated to vector to it */
  void isr(void);
  /** called by isr on an input event.
   * negative values of @param are notifications of line errors, -1 for interrupts disabled */
  typedef bool (*Receiver)(int incoming);
  Receiver receive;
  /** called by isr when transmission becomes possible.
@return either an 8 bit unsigned character, or -1 to disable transmission events*/
  typedef int (*Sender)();
  Sender send;
protected:
  /** read at least one char from fifo, stop on error. @param LSRValue is recent read of line status register, @returns final read of line status register */
  unsigned tryInput(unsigned LSRValue);
public:
  Uart();
  /** set the baudrate using tweaky fields: */
  unsigned setBaudPieces(unsigned divider, unsigned mul, unsigned div, unsigned sysFreq) const;

  /** @param hertz is used to compute multiple register settings to get the closest to the desired value.
@returns the actual rate.@param sysFreq if left 0 uses the running clock's frequency for calculations else the given value is used.  */
  unsigned setBaud(unsigned hertz, unsigned sysFreq=0)const;
  /** @param coded e.g. "8N1" for typical binary protocol, 8E2 for modbus */
  void setFraming(const char*coded)const;

  /** called to request that transmit isr be enabled or disabled.
   * Note: if enabled and wasn't enabled then send() will immediately be called to get first byte to send, IE send() isn't only called from isr (unless we can arrange for that!). */
  void transmission(bool enabled=true);
  /** call to request that reception isr be enabled or disabled. */
  void reception(bool enabled=true);
  /** set send function. @returns this. */
  Uart& setTransmitter(Sender sender){
    this->send=sender;
    return *this;
  }
  /** set reception routine. . @returns this. */
  Uart& setReceiver(Receiver receiver){
    this->receive=receiver;
    return *this;
  }

};

}

#endif // UART_H
