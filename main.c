#include "half_fit.h"

int main(void)
{
    half_init();
    half_alloc(1000);
    half_alloc(1000);
    half_alloc(1000);
    half_alloc(1000);
    half_alloc(1000);
    half_alloc(1000);
    half_alloc(1000);
    half_alloc(10000);
    // Not enough memory to allocate these.
    half_alloc(10000);
    half_alloc(10000);
    return 0;
}
