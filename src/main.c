#include <stdio.h>

#include "assert.h"
#include "debug.h"
#include "intrinsics.h"
#include "memory.h"

typedef struct {
    u8 flag;
    byte arr[PAGE_SIZE - 1];
} Page;

int main(int argc, char **argv)
{
    MemArena mem = create_arena(PAGE_SIZE * 12, PAGE_SIZE * 2);
    Page *restrict struct1 = (Page*) alloc_struct(&mem, Page);
    struct1->flag = 42;

    Page *restrict struct2 = (Page*) alloc_struct(&mem, Page);
    struct2->flag = 43;

    for (int i = 0; i < 8; ++i)
        alloc_struct(&mem, Page);

    alloc_struct(&mem, Page);

    assert(struct1->flag == 42, "Flag is wrong");
    assert(struct2->flag == 43, "Flag is wrong");

    assert(struct2 == struct1 + 1, "Wrong pos");

    // TODO: Test _alloc_data when new region must created because new chunk is allocated
    //       (both when regions_arr is copied and when it isn't)
    //
    //       Also test alloc_array

    return 0;
}
