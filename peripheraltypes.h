#pragma once

#include "eztypes.h"
#include "bitbanger.h"
#include "boolish.h"

/**
 *  types used for declaration of peripherals.
 *
 *  The SFR* template classes have a peculiar usage pattern, caused by trying to make all related code be generated inline.
 *  At hardware module definition time one creates a typedef for each field rather than an instance.
 *  At each place of use one creates an instance and then assigns to that instance to write a value or simply reference that instance for a read.
 *  This precludes extern'ing global instances which then must take up real data space (const so not a big cost) and then be accessed via an extra level of indirection compared to what the 'create local instance' can result in.
 *  If we could create a global instance in a header file but get just one actual object created we would do that.
 * Volatile is used to keep the compiler from optimizing away accesses that have physical side effects.
I am working on replacing *'s with &'s, its a statisitcal thing herein as to which is better.
*/

//inline void setBit(unsigned  address, unsigned bit){
//  *atAddress(address)|= 1<<bit;
//}

//inline void clearBit(unsigned  address, unsigned  bit){
//  *atAddress(address) &= 1<<bit;
//}

///** ensure a 0:1 transition occurs on given bit. */
//inline void raiseBit(unsigned  address, unsigned  bit){
//  clearBit(address, bit);
//  setBit(address, bit);
//}

/** for a private single instance block */
#define soliton(type, address) type & the ## type = *reinterpret_cast <type *> (address);

/** the following are mostly markers, but often it is tedious to insert the 'volatile' and dangerous to leave it out. */
typedef volatile u32 SFR;
/** marker for non-occupied memory location */
typedef const u32 SKIPPED;
// packable byte:
typedef volatile u8 SFR8;
typedef volatile u16 SFR16;
// todo: 16 bit ones, with word spacers.
// todo: macro for skip block


/** most cortex devices follow arm's suggestion of using this block for peripherals */
const u32 PeripheralBase(0x40000000);//1<<30


/** Multiple contiguous bits in a register
 * Note: This creates a class per sf register field, but the compiler should inline all uses making this a compile time burden but a runtime minimalization.
 * Note: 'volatile' isn't used here as it is gratuitous when the variable isn't nominally read multiple times in a function.
 */
template <unsigned sfraddress, int pos, int width = 1> class SFRfield {
  enum {
    /** mask positioned */
    mask = ((1 << width) - 1) << pos
  };

  inline SFR &sfr() const {
    return *reinterpret_cast<SFR *>(sfraddress);
  }
private:
  SFRfield(const SFRfield &other) = delete;

public:
  SFRfield(){
    //this constructor is needed due to use of explict on the other constructor
  }

  explicit SFRfield(unsigned initlizer){
    this->operator =(initlizer);
  }

  // read
  inline operator uint32_t() const {
    return (sfr() & mask) >> pos;
  }

  // write
  inline void operator =(uint32_t value) const {
    sfr() = ((value << pos) & mask) | (sfr() & ~mask);
  }

  void operator +=(unsigned value) const {
    sfr()=sfr()+value;
  }
};


/** single bit, ignoring the possibility it is in bitbanded memory (an stm32 special feature it seems).
 *  This is NOT derived from SFRfield as we can do some optimizations that the compiler might miss (or developer might have disabled)*/
template <unsigned sfraddress, int pos> class SFRbit : public BoolishRef {
  enum {
    mask = 1 << pos
  };

  inline SFR &sfr() const {
    return *reinterpret_cast<SFR *>(sfraddress);
  }
public:
  // read
  inline operator bool() const {
    return (sfr() & mask) != 0;
  }
  // write
  bool operator =(bool value) const{
    if(value) {
      sfr() |= mask;
    } else {
      sfr() &= ~mask;
    }
    return value;
  }
};

///** macro for first template argument for SFRfield and SFRbit, mates to cmsis style type declarations which can't use c bit-fields.*/
//#define SFRptr(absaddress, blocktype, member) (absaddress + offsetof(blocktype, member))
//todo: constexpr version
