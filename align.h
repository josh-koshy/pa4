#define HEADER_SIZE     8
#define ALIGNMENT       16
#define ALIGN(size)     (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
