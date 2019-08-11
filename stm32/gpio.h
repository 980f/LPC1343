#pragma once

#include "stm32.h"

/** the 16 bits as a group.
  * Note well that even when the group is not enabled the port can be read from (as long as it exists).
  *
  * For pins that are optional to a module use (const Pin *) parameters a pass nulls. Trying to create safely non-functional pins is expensive and the check for 'has a pin' is the same cost, but only burdens those pin uses which ca be optionally present. There are usually some port bits that aren't pinned out which can be used as dummies when a null pointer to a pin just isn't convenient.
  */

struct Port /*Manager*/ : public APBdevice {
  static bool isOutput(unsigned pincode);

  /** a contiguous set of bits in a a single Port */
  struct Field {
    const Address odr;
    const Address at;
    const unsigned lsb;
    const unsigned mask; //derived from width
    Field(unsigned pincode,const Port &port, unsigned lsb, unsigned msb);
    /** insert @param value into field, shifting and masking herein, i.e always lsb align the value you supply here */
    void operator =(unsigned value)const;
    /** toggle the bits in the field that correspond to ones in the @param value */
    void operator ^=(unsigned value)const;
    /** @returns the value set by the last operator =, ie the requested output- not what the physical pins are reporting. */
    operator u16() const;
    /** read back the actual pins */
    u16 actual() const;
    //more operators only at need
  };

  /** @param letter is the uppercase character from the stm32 manual */
  Port(char letter);
  /**
    * configure the given pin.
    todo:M enumerize the pin codes (but @see InputPin and OutputPin classes which construct codes for you.)
    */
  void configure(unsigned bitnum, unsigned code) const;
  /** @returns accessor object for "output data register" */
  ControlWord odr(void) const {
    return ControlWord(registerAddress(12));
  }

};


//these take up little space and it gets annoying to have copies in many places.
extern const Port PA;
extern const Port PB;
extern const Port PC;
extern const Port PD;
extern const Port PE;
//extern const Port F;
//extern const Port G;


//GPIO interrupt configuration options. Some devices may not support some options, but most do so this is defined here.
enum IrqStyle {
  NotAnInterrupt = 0, // in case someone forgets to explicitly select a mode
  AnyEdge, // edge, either edge, input mode buslatch
  LowActive, // level, pulled up
  HighActive, // level, pulled down
  LowEdge, // edge, pulled up
  HighEdge   // edge, pulled down
};


/**
  * this class manages the nature of a pin, and provides cacheable accessors for the pin value.
  * you may declare each as const, the internals are all const.
  */
struct Pin /*Manager*/ {
  const unsigned bitnum;
  const Port &port;

  void output(unsigned int code, unsigned int mhz, bool openDrain) const; /* output */

  Pin(const Port &port, unsigned bitnum);
  /** @returns this after configuring it for analog input */
  const Pin& AI(void) const;
  /** @returns bitband address for input after configuring as digital input, pull <u>U</u>p, pull <u>D</u>own, or leave <u>F</u>loating*/
  ControlWord DI(char UDF = 'D') const;
  /** @returns bitband address for controlling high drive capability [rtfm] */
  ControlWord highDriver(void) const;
  /** configure as simple digital output */
  ControlWord DO(unsigned int mhz = 2, bool openDrain = false) const;
  /** configure pin as alt function output*/
  const Pin& FN(unsigned int mhz = 2, bool openDrain = false) const;
  /** declare your variable volatile, reads the actual pin, writing does nothing */
  ControlWord reader(void) const;
  /** @returns reference for writing to the physical pin, reading this reads back the DESIRED output */
  ControlWord writer(void) const;

  /** for special cases, try to use one of the above which all call this one with well checked argument */
  void configureAs(unsigned int code) const;

  /** raw access convenience. @see InputPin for business logic version of a pin */
  operator bool(void)const{
    return reader();
  }

  /** @returns pass through @param truth after setting pin to that value.
 @see OutputPin for business logic version */
  bool operator = (bool truth)const{
    writer()=truth;
    return truth;//#don't reread the pin, nor its actual, keep this as a pass through
  }

};

/** base class for InputPin and OutputPin that adds polarity at construction time.
 //not templated as we want to be able to pass Pin's around. not a hierarchy as we don't want the runtime cost of virtual table lookup. */
class LogicalPin {
protected:
  const ControlWord bitbanger;
  /** the level that is deemed active  */
  const bool active;
  bool polarized(bool operand)const{
    return active!=operand;
  }

  LogicalPin(Address registerAddress,bool active=1);
public:

  /** @returns for outputs REQUESTED state of pin, for inputs the actual pin */
  operator bool(void)const{
    return polarized(bitbanger);
  }
};

/**
hide the volatile and * etc that I sometimes forget.
*/
class InputPin :public LogicalPin {

public:
  InputPin(const Pin &pin, char UDF = 'D', bool active=1);
  InputPin(const Pin &pin, bool active);  //pull the opposite way of the 'active' level.
  //maydo: add method to change pullup/pulldown bias while running

};

/**
a digital output made to look like a simple boolean.
Note that these objects can be const while still manipulating the pin.
*/
class OutputPin :public LogicalPin {

public:
  OutputPin(const Pin &pin, bool active=1, unsigned int mhz = 2, bool openDrain = false);

  /** @returns pass through @param truth after setting pin to that value */
  bool operator = (bool truth)const{
    bitbanger=polarized(truth);
    return truth;//don't reread the pin, nor its actual, keep this as a pass through
  }

  /** set to given value, @returns whether a change actually occurred.*/
  bool changed(bool truth) const{
    truth=polarized(truth);
    if(bitbanger!=truth){
      bitbanger=truth;
      return true;
    }
    return false;
  }

  bool actual() const {//todo:00 see if this survived ControlWord class.
    return polarized((&bitbanger)[-32]);//idr is register preceding odr, hence 32 bits lower in address
  }

  /** actually invert the present state of the pin */
  void toggle();
};
