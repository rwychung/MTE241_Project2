#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "half_fit.h"

#define BYTES_TO_BIT(num) (num << 3)
#define BIT_TO_BYTES(num) (num >> 3)

// Header->  prev_addr  next_addr  block_size  alloc_flag
//           0000000000 0000000000 00000000000 0
// 0x003FFFFF = 0000 0000 0011 1111 1111 1111 1111 1111b
#define SET_PREV_ADDR(header, addr) (header = (header & 0x003FFFFFU) | (addr << 22))
// 0xFFC00FFF = 1111 1111 1100 0000 0000 1111 1111 1111b
#define SET_NEXT_ADDR(header, addr) (header = (header & 0xFFC00FFFU) | (addr << 12))
// 0xFFFFF001 = 1111 1111 1111 1111 1111 0000 0000 0001b
#define SET_SIZE(header, size) (header = (header & 0xFFFFF001U) | (size << 1))
// 0xFFFFFFFE = 1111 1111 1111 1111 1111 1111 1111 1110b
#define SET_ALLOC(header, flag) (header = (header & 0xFFFFFFFEU) | flag)

// 0x000003FF = 0000 0000 0000 0000 0000 0011 1111 1111b
#define GET_PREV_ADDR(header) ((header >> 22) & 0x000003FFU)
#define GET_NEXT_ADDR(header) ((header >> 12) & 0x000003FFU)
// 0x000007FF = 0000 0000 0000 0000 0000 0011 1111 1111b
#define GET_SIZE(header) ((header >> 1) & 0x000007FFU)
#define GET_ALLOC(header) (header & 0x1U)

// Convert address to its index in the memory block.
#define ADDR_TO_INDEX(addr) (addr << 3)

const int MEMORY_SIZE = 32768;  // The total number of bytes available.
const int UNIT_BLOCK_SIZE = 32; // Unit memory block size is 32 bytes.

/*
 * The block of memory will contain 8192 blocks of 32 bits.
 */
mem_block_t *memory = NULL;

/*
 * buckets[0] = 32 - 63 bytes
 * buckets[1] = 64 - 127 bytes
 * buckets[2] = 128 - 255 bytes
 * buckets[3] = 256 - 511 bytes
 * buckets[4] = 512 - 1023 bytes
 * buckets[5] = 1024 - 2047 bytes
 * buckets[6] = 2048 - 4095 bytes
 * buckets[7] = 4096 - 8191 bytes
 * buckets[8] = 8192 - 16383 bytes
 * buckets[9] = 16384 - 32767 bytes
 * buckets[10] = 32768 - 65535 bytes
 */
// The buckets store the address of each 32 byte block as 10bits.
int buckets[11] = {-1};

void printBinary(uint32_t num)
{
    int c, k;
    for (c = 31; c >= 0; c--)
    {
        k = num >> c;
        if (k & 1)
            printf("1");
        else
            printf("0");
        if ((c % 4) == 0)
            printf(" ");
    }
    printf("\n");
}

int getBucketIndex(int size)
{
    int index = 0;

    /*
     * Subtract 1 from the size to ensure a request for memory equal to the
     * lower limit of a bin will be grabbed from the appropriate bin.
     * ie. size = 64, we know bin 1 will have a block >= 64.
     *     64 = 0100 0000 has most significant bit in 7th digit.
     *     65 = 0100 0001 has most significant bit in 7th digit.
     *     Based on below algorithm, index = 7-5 = 2 which means 64 and 65 will be placed
     *     in bin 2. This is appropriate for size of 65, but 64 can fit in bin 1.
     */
    size--;

    // Offset bits so that all values <= 32 belong into bin 0.
    size >>= 5;

    // Shifts the size bits right to calculate its log2 value.
    while (size)
    {
        size >>= 1;
        index++;
    }

    return index;
}

int getBucketMemory(int index, int size)
{
    int blockIndex, splitBlockIndex, blockSize, splitBlockSize;
    mem_block_t memBlock, splitMemBlock;

    // Get the position of the address in the memory array.
    blockIndex = ADDR_TO_INDEX(buckets[index]);
    memBlock = memory[blockIndex]

    // Get the second 32 bit block of the block of memory which
    // contains information about the bucket the block is in.
    if (GET_NEXT_ADDR(memory[ADDR_TO_INDEX(buckets[index]) + 1]) == buckets[index])
    {
        // If there are no other blocks in bucket.
        buckets[index] = -1;
    }
    else
    {
        // If there is another block in the bucket, get the next block of memory in the bucket.
        buckets[index] = GET_NEXT_ADDR(memory[ADDR_TO_INDEX(buckets[index]) + 1]);
    }

    // Split memory
    blockSize = GET_SIZE(memBlock); // Size of the current block of memory.

    // Split the block of memory if the remaining size is > 32.
    if ((blockSize - size) >= 32)
    {
        // Set the new block size to the next biggest.
        // TODO: Get the biggest multiple of the size.
        //blockSize = ((size / UNIT_BLOCK_SIZE)) * UNIT_BLOCK_SIZE;
        // Get the address of where the split unallocated memory block will be.
        // The block size received from the header number of 32 byte blocks.
        // Convert it to 32 bit blocks which is how the memory array is kept in.
        splitBlockIndex = memIndex + BYTES_TO_BIT(blockSize);
        splitMemBlock = memory[splitBlockIndex];

        SET_NEXT_ADDR(memBlock, splitMemBlock);
        SET_SIZE(memBlock,)

    }













    // Update the split blocks of memory
    // Place left over memory in new bucket.
    // Return address to block of memory
}

void half_init(void)
{
    // Array of 32 bit blocks which make up the 32768 byte block.
    memory = (mem_block_t *)malloc(MEMORY_SIZE);

    SET_PREV_ADDR(memory[0], 0x3FE);
    printBinary(memory[0]);
    SET_NEXT_ADDR(memory[0], 0x3FE);
    printBinary(memory[0]);
    SET_SIZE(memory[0], 1024);
    printBinary(memory[0]);
    SET_ALLOC(memory[0], 1);
    printBinary(memory[0]);

    SET_PREV_ADDR(memory[0], 0x333);
    printBinary(memory[0]);
    SET_NEXT_ADDR(memory[0], 0x333);
    printBinary(memory[0]);
    SET_SIZE(memory[0], 100);
    printBinary(memory[0]);
    SET_ALLOC(memory[0], 0);
    printBinary(memory[0]);

    printf("---------------\n");
    printBinary(GET_PREV_ADDR(memory[0]));
    printBinary(GET_NEXT_ADDR(memory[0]));
    printBinary(GET_SIZE(memory[0]));
    printBinary(GET_ALLOC(memory[0]));

    // Sets the information in the header.
    SET_PREV_ADDR(memory[0], 0);
    SET_NEXT_ADDR(memory[0], 0);
    SET_SIZE(memory[0], 1024);
    SET_ALLOC(memory[0], 0);

    // Set the information for next and previous block in bin.
    SET_PREV_BUCKET_ADDR(memory[0], 0x333);
    printBinary(memory[1]);
    SET_NEXT_BUCKET_ADDR(memory[0], 0x333);
    printBinary(memory[1]);

    // Place into biggest bucket.
    buckets[10] = 0;
}

void *half_alloc(int size)
{
    int bucketIndex;
    mem_block_t *newAlloc = NULL;

    // Don't allocate memory if request was for 0 bytes.
    if (size == 0)
        return NULL;

    size += 4; // Add 4 bytes for the header to the size of the block required.

    if (size > MEMORY_SIZE)
    {
        printf("Not enough memory\n");
        return NULL;
    }

    bucketIndex = getBucketIndex(size);

    // Finds the next biggest bucket that contains blocks of memory.
    printf("Request of %d bytes belong into bin # %d\n", size - 4, bucketIndex);

    while (buckets[bucketIndex] == -1 && bucketIndex < 11)
    {
        bucketIndex++;
    }

    // If there are no buckets
    if (bucketIndex > 10)
        return NULL;


//    return getBucketMemory(bucketIndex, size);
}

void half_free(void * memory)
{
}
