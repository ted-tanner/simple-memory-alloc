#ifndef __INTRINSICS_H

// Don't go through the effort of requesting page size from OS (for now at least)
#define PAGE_SIZE 4096

#define mod_pow_2(operand1, operand2) ((operand1) & ((operand2) - 1))
    
#define __INTRINSICS_H
#endif
