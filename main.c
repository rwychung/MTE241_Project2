#include "half_fit.h"

int main(void)
{
    half_init();
    half_alloc(63);
    half_alloc(64);
    half_alloc(10);
    half_alloc(12312);
    half_alloc(27);
    half_alloc(28);
    half_alloc(32764);
    half_alloc(32);
    return 0;
}
