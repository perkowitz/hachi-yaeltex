#ifndef UTILITY_H
#define UTILITY_H

// typedef and convenient functions for treating a 16-bit uint as a bit array
typedef uint16_t bit_array_16;
bool BitArray16_Get(bit_array_16 array, u8 index);
bit_array_16 BitArray16_Set(bit_array_16 array, u8 index, bool value);
bit_array_16 BitArray16_Toggle(bit_array_16 array, u8 index);




#endif
