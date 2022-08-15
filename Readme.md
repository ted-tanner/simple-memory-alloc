# Memory Allocator

### Ideas
* To avoid visiting potentially every member of the regions array in O(n) time, each region should contain a pointer to the regions before and after it. Once a region is found that is suitable for allocating a chunk of the given size, use the before and after pointers to determine if the regions may be coalesced. Coagulated 
