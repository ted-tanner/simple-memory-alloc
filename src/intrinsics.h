#ifndef __INTRINSICS_H

// Don't go through the effort of requesting page size from OS (for now at least)
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
