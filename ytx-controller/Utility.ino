#include "headers/Utility.h"


// typedef and convenient functions for treating a 16-bit uint as a bit array
static const uint16_t mask = 1;

bool BitArray16_Get(bit_array_16 array, u8 index) {
  return (array & (mask << index)) != 0;
}

bit_array_16 BitArray16_Set(bit_array_16 array, u8 index, bool value) {
  if (value) {
    return array | (mask << index);
  } else {
    return array & ~(mask << index);
  }
}

bit_array_16 BitArray16_Toggle(bit_array_16 array, u8 index) {
  return array ^ (mask << index);
}
