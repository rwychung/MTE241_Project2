#include "half_fit.h"

int main(void)
{
    void *ptr1, *ptr2;
    half_init();
    ptr1 = half_alloc(1000);
    ptr2 = half_alloc(1012);

    printMemory();
    printBucketInfo();

    half_free(ptr1);
    printMemory();
    printBucketInfo();

    half_free(ptr2);
    printMemory();
    printBucketInfo();
    return 0;
}
