#include "types.h"
#include "riscv.h"
#include "defs.h"

// Pseudo random number generator
unsigned rand() {
    static unsigned r = 0;
    return r = r ? ((16807 * r) % ((unsigned)-1 >> 1)) : random;
}
