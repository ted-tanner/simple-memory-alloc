#ifndef __INTRINSICS_H

// NOTE: 64-bit
typedef signed char i8;
typedef signed short int i16;
typedef signed int i32;
typedef signed long long int i64;

typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef unsigned char byte;
typedef unsigned long int size_t;

#ifndef true

typedef _Bool bool;
#define true 1
#define false 0

#endif

// TODO: Request page size from the OS instead
#define PAGE_SIZE 4096

#define mod_pow_2(operand1, operand2) ((operand1) & ((operand2) - 1))

#ifdef _MSC_VER
#define FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
#define FORCEINLINE __attribute__((always_inline)) inline
#else
#error Unsupported compiler
#endif
    
#define __INTRINSICS_H
#endif
