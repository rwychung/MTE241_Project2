#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "half_fit.h"

// Header->  prev_addr  next_addr  block_size  alloc_flag
//           0000000000 0000000000 00000000000 0

// 0x003FFFFF = 0000 0000 0011 1111 1111 1111 1111 1111b
#define SET_PREV_ADDR(header, addr) ((header) = ((header) & 0x003FFFFFU) | ((addr) << 22))
// 0xFFC00FFF = 1111 1111 1100 0000 0000 1111 1111 1111b
#define SET_NEXT_ADDR(header, addr) ((header) = ((header) & 0xFFC00FFFU) | ((addr) << 12))
// 0xFFFFF001 = 1111 1111 1111 1111 1111 0000 0000 0001b
#define SET_SIZE(header, size) ((header) = ((header) & 0xFFFFF001U) | ((size) << 1))
// 0xFFFFFFFE = 1111 1111 1111 1111 1111 1111 1111 1110b
#define SET_ALLOC(header, flag) ((header) = ((header) & 0xFFFFFFFEU) | (flag))

// 0x000003FF = 0000 0000 0000 0000 0000 0011 1111 1111b
#define GET_PREV_ADDR(header) (((header) >> 22) & 0x000003FFU)
#define GET_NEXT_ADDR(header) (((header) >> 12) & 0x000003FFU)
// 0x000007FF = 0000 0000 0000 0000 0000 0011 1111 1111b
#define GET_SIZE(header) (((header) >> 1) & 0x000007FFU)
#define GET_ALLOC(header) ((header) & 0x1U)

// Get the header to a block of memory received from the user.
#define GET_MEMORY_HEADER(memPtr) ((memPtr) - 1)

// Convert address to its index in the memory block.
#define ADDR_TO_INDEX(addr) ((addr) << 3)
#define INDEX_TO_ADDR(index) ((index) >> 3)

// Conversions
#define BLOCKS_TO_BYTES(blocks) ((blocks) << 5)
#define BLOCKS_TO_INDEX(blocks) ((blocks) << 3)
#define BYTES_TO_BLOCKS(bytes) ((bytes) >> 5)

const int MEMORY_SIZE = 32768;  // The total number of bytes available.
const int UNIT_BLOCK_SIZE = 32; // Unit memory block size is 32 bytes.
const int BUCKET_SIZE = 11;

/*
 * The block of memory will contain 8192 blocks of 32 bits.
 */
mem_block_t *memory = NULL;

/*
 * The buckets store the address of each 32 byte block as 10 bits.
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
int buckets[11] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

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

void printHeaderInfo(mem_block_t blockHeader)
{
    printf("***************************************\n");
    printf("Print header info\n");
    printf("Prev Addr: %d\n", GET_PREV_ADDR(blockHeader));
    printf("Next Addr: %d\n", GET_NEXT_ADDR(blockHeader));
    printf("Block size: %d\n", GET_SIZE(blockHeader));
    printf("Block size (bytes): %d\n", BLOCKS_TO_BYTES(GET_SIZE(blockHeader)));
    printf("Allocated: %s\n", GET_ALLOC(blockHeader) ? "Yes" : "No");
    printf("***************************************\n");
}

void printBucketInfo()
{
    printf("***************************************\n");
    printf("Printing Bucket Info\n");
    int i;
    for (i=0; i<11; i++)
    {
        printf("Bucket #%d Addr: %d \n", i, buckets[i]);
    }
    printf("***************************************\n");
}

void printMemory()
{
    printf("***************************************\n");
    printf("Printing Memory\n");
    int addr = 0;

    while(1)
    {
        printf("Current Addr: %d\n", addr);
        printHeaderInfo(memory[ADDR_TO_INDEX(addr)]);
        if (addr == GET_NEXT_ADDR(memory[ADDR_TO_INDEX(addr)]))
            break;
        addr = GET_NEXT_ADDR(memory[ADDR_TO_INDEX(addr)]);
    }
    printf("***************************************\n");
}

void checkMemory()
{
    int prevAddr = 0;
    int addr = 0;
    int size = 0;

    printf("Start checking memory\n");
    while(1)
    {
        printf("***************************************\n");
        printf("Current Addr: %d\n", addr);

        printHeaderInfo(memory[ADDR_TO_INDEX(addr)]);
        if (prevAddr != GET_PREV_ADDR(memory[ADDR_TO_INDEX(addr)]))
        {
            printf("MEMORY DEFRAGED: invalid reference to previous mem block at addr: %d\n", addr);
            printf("Current addr's prev: %d\n", GET_PREV_ADDR(memory[ADDR_TO_INDEX(addr)]));
            printf("Prev: %d\n", prevAddr);
        }

        size += BLOCKS_TO_BYTES(GET_SIZE(memory[ADDR_TO_INDEX(addr)]));
        if (addr == GET_NEXT_ADDR(memory[ADDR_TO_INDEX(addr)]))
            break;
        prevAddr = addr;
        addr = GET_NEXT_ADDR(memory[ADDR_TO_INDEX(addr)]);
        printf("***************************************\n");
    }

    if (size != MEMORY_SIZE)
    {
        printf("MEMORY DEFRAGED\n");
    }
    printf("Start checking memory\n");
}

void checkBuckets()
{
    int i;
    int addr = 0;
    int tempSize = 0;
    int unallocSize = 0;
    int bucketSize = 0;
    mem_block_t tempHeader;
    while (1)
    {
        tempHeader = memory[ADDR_TO_INDEX(addr)];
        tempSize = GET_SIZE(tempHeader);
        if (!GET_ALLOC(tempHeader))
        {
            unallocSize += tempSize;
        }
        if (addr == GET_NEXT_ADDR(memory[ADDR_TO_INDEX(addr)]))
            break;
        addr = GET_NEXT_ADDR(memory[ADDR_TO_INDEX(addr)]);
    }

    for (i=0; i < BUCKET_SIZE; i++)
    {
        if (buckets[i] != -1)
        {
            addr = buckets[i];
            while (1)
            {
                bucketSize += GET_SIZE(memory[ADDR_TO_INDEX(addr)]);
                if (addr == GET_NEXT_ADDR(memory[ADDR_TO_INDEX(addr) + 1]))
                    break;
                addr = GET_NEXT_ADDR(memory[ADDR_TO_INDEX(addr) + 1]);
            }
        }
    }

    if (bucketSize != unallocSize)
    {
        printf("Unalloc size = %d but size of blocks in bucket = %d\n", unallocSize, bucketSize);
    }
}

void bucketInit()
{
    int i;
    for (i=0; i<BUCKET_SIZE; i++)
    {
        buckets[i] = -1;
    }
}

int getMemBlockIndex(mem_block_t *memBlock)
{
    return (memBlock - memory);
}

/*
 * Returns the index of which a block of size belongs in.
 *
 * @param   size    The size of a block in bytes.
 */
int getBucketIndex(int size)
{
    int index = 0;

    size >>= 6;

    while (size)
    {
        size >>= 1;
        index++;
    }

    return index;
}

/*
 * Places the unallocated block of memory into the appropriate bucket.
 *
 * @param   emptyBlock  Header of the block to insert into approriate bucket.
 * @param   address     Address of the block of memory.
 */
void placeInBucket(mem_block_t emptyBlock, int address)
{
    //choose appropriate bucket
    //place into bucket and do linking
    int index = getBucketIndex(BLOCKS_TO_BYTES(GET_SIZE(emptyBlock)));

    if (buckets[index] == -1)
    {
        // Empty bucket
        SET_NEXT_ADDR(memory[ADDR_TO_INDEX(address) + 1], address);
    }
    else
    {
        // There are blocks in the bucket.
        SET_PREV_ADDR(memory[ADDR_TO_INDEX(buckets[index]) + 1], address);
        SET_NEXT_ADDR(memory[ADDR_TO_INDEX(address) + 1], buckets[index]);
    }

    // Insert block into the beginning of the bucket.
    SET_PREV_ADDR(memory[ADDR_TO_INDEX(address) + 1], address);
    buckets[index] = address;
}

/*
 * Removes the given block from its bucket.
 *
 * @param   blockIndex  The index of the block to remove in the memory array.
 */
void removeFromBucket(int blockIndex)
{
    int bucketIndex = getBucketIndex(BLOCKS_TO_BYTES(GET_SIZE(memory[blockIndex])));
    int blockAddr = INDEX_TO_ADDR(blockIndex);
    int prevBlockAddr = GET_PREV_ADDR(memory[blockIndex + 1]);
    int nextBlockAddr = GET_NEXT_ADDR(memory[blockIndex + 1]);
    int prevBlockIndex = ADDR_TO_INDEX(prevBlockAddr);
    int nextBlockIndex = ADDR_TO_INDEX(nextBlockAddr);

    if ((prevBlockAddr == blockAddr) && (nextBlockAddr == blockAddr))
    {
        // Current block is the only block.
        buckets[bucketIndex] = -1;
    }
    else if (prevBlockAddr == blockAddr)
    {
        // Current block is the first block.
        buckets[bucketIndex] = nextBlockAddr;
        SET_PREV_ADDR(memory[nextBlockIndex + 1], nextBlockAddr);
    }
    else if (nextBlockAddr == blockAddr)
    {
        // Current block is the end block.
        SET_NEXT_ADDR(memory[prevBlockIndex + 1], prevBlockAddr);
    }
    else
    {
        // Current block is between two blocks.
        SET_NEXT_ADDR(memory[prevBlockIndex + 1], nextBlockAddr);
        SET_PREV_ADDR(memory[nextBlockIndex + 1], prevBlockAddr);
    }
}

/*
 * Finds the smallest portion of the memory block needed to allocate the requested bytes.
 * Splits the memory block if sufficient space is left and places it into a new bucket.
 *
 * @param   memBlock        The block of memory which can be allocated.
 * @param   blockIndex      The index of the block of memory in the global memory array.
 * @param   bytesRequested  The number of bytes requested by the user.
 */
void splitMemoryBlock(mem_block_t memBlock, int blockIndex, int bytesRequested)
{
    int blockSize, blockSize2, blockIndex2;
    mem_block_t memBlock2;

    /* Split the block of memory returned by the bucket if necessary. */
    blockSize = GET_SIZE(memBlock);

    // Split the block of memory if the remaining size is > 32.
    if ((BLOCKS_TO_BYTES(blockSize) - bytesRequested) >= 32)
    {
        // Set the new block size to the next biggest.
        // One block size equals 32 bytes.
        blockSize2 = blockSize -
            ((bytesRequested / UNIT_BLOCK_SIZE) + ((bytesRequested % UNIT_BLOCK_SIZE) != 0));
        blockSize -= blockSize2;

        // Get the address of where the split unallocated memory block will be.
        // The block size received from the header number of 32 byte blocks.
        // Convert it to 32 bit blocks which is how the memory array is kept in.
        blockIndex2 = blockIndex + BLOCKS_TO_INDEX(blockSize);
        memBlock2 = memory[blockIndex2];

        /* Sets the header for the unallocated split block. */
        // The split block of memory will never be the first block.
        SET_PREV_ADDR(memory[blockIndex2], INDEX_TO_ADDR(blockIndex));

        // The split block could be the last block of memory.
        if (blockIndex == ADDR_TO_INDEX(GET_NEXT_ADDR(memBlock)))
        {
            // Last block in memory.
            SET_NEXT_ADDR(memory[blockIndex2], INDEX_TO_ADDR(blockIndex2));
        }
        else
        {
            // Set split block to point to next block and next block to point to split block.
            SET_NEXT_ADDR(memory[blockIndex2], GET_NEXT_ADDR(memBlock));
            SET_PREV_ADDR(memory[ADDR_TO_INDEX(GET_NEXT_ADDR(memBlock))],
                    INDEX_TO_ADDR(blockIndex2));
        }
        SET_SIZE(memory[blockIndex2], blockSize2);
        SET_ALLOC(memory[blockIndex2], 0);

        /* Sets the header for the allocated split block */
        SET_NEXT_ADDR(memory[blockIndex], INDEX_TO_ADDR(blockIndex2));
        SET_SIZE(memory[blockIndex], blockSize);

        // Place left over memory in new bucket.
        placeInBucket(memory[blockIndex2], INDEX_TO_ADDR(blockIndex2));
    }

    SET_ALLOC(memory[blockIndex], 1);
}

void* getBucketMemory(int bucketIndex, int bytesRequested)
{
    // Get the index of the address in the memory array to access the memory block.
    int blockIndex = ADDR_TO_INDEX(buckets[bucketIndex]);
    mem_block_t memBlock = memory[blockIndex];

    // Get the second 32 bit block of the block of memory which
    // contains information about the bucket the block is in.
    if (GET_NEXT_ADDR(memory[blockIndex + 1]) == buckets[bucketIndex])
    {
        // If there are no other blocks in bucket.
        buckets[bucketIndex] = -1;
    }
    else
    {
        // If there is another block in the bucket, get the next block of memory in the bucket.
        buckets[bucketIndex] = GET_NEXT_ADDR(memory[blockIndex + 1]);
    }

    // Remove the block to be allocated from its bucket.
    removeFromBucket(blockIndex);

    // Splits the memory block if necessary.
    splitMemoryBlock(memBlock, blockIndex, bytesRequested);

    // Return address to user accessible memory, which excludes the header.
    return (void *)(&memory[blockIndex + 1]);
}

void half_init(void)
{
    // Array of 32 bit blocks which make up the 32768 byte block.
    memory = (mem_block_t *)malloc(MEMORY_SIZE);
    bucketInit();

    // Sets the information in the header.
    SET_PREV_ADDR(memory[0], 0);
    SET_NEXT_ADDR(memory[0], 0);
    SET_SIZE(memory[0], BYTES_TO_BLOCKS(MEMORY_SIZE));
    SET_ALLOC(memory[0], 0);

    // Sets the information on bucket blocks.
    SET_PREV_ADDR(memory[1], 0);
    SET_NEXT_ADDR(memory[1], 0);

    buckets[10] = 0;
}

void *half_alloc(int size)
{
    int tempSize = size;
    int bucketIndex = 0;
    mem_block_t *newAlloc = NULL;
    void *memBlock = NULL;

//    printf("Request for %d bytes\n", size);
    // Don't allocate memory if request was for 0 bytes.
    if (size == 0)
        return NULL;

    size += 4; // Add 4 bytes for the header to the size of the block required.

    if (size > MEMORY_SIZE)
    {
        return NULL;
    }

    /*
     * Gets the bucket from which a block of size 'size' will be guarenteed to be in.
     *
     * Subtract 1 from the size to ensure a request for memory equal to the
     * lower limit of a bucket will be grabbed from the appropriate bucket.
     * ie. size = 64, we know bucket 1 will have a block >= 64.
     *     64 = 0100 0000 has most significant bit in 7th digit.
     *     65 = 0100 0001 has most significant bit in 7th digit.
     *     Based on below algorithm, index = 7-5 = 2 which means 64 and 65 will be placed
     *     in bucket 2. This is appropriate for size of 65, but 64 can fit in bucket 1.
     */
    tempSize--;

    // Offset bits so that all values <= 32 belong into bucket 0.
    tempSize >>= 5;

    // Shifts the size bits right to calculate its log2 value.
    while (tempSize)
    {
        tempSize >>= 1;
        bucketIndex++;
    }

    // Finds the next biggest bucket that contains blocks of memory.

    while (buckets[bucketIndex] == -1 && bucketIndex < 11)
    {
        bucketIndex++;
    }

    // If there are no buckets
    if (bucketIndex > 10)
        return NULL;

    memBlock = getBucketMemory(bucketIndex, size);
    checkBuckets();
    return memBlock;
}

void half_free(void * freeMemory)
{
    // One mem_block_t size before the memory address returns the header of the block.
    mem_block_t *freeBlock = GET_MEMORY_HEADER((mem_block_t *)freeMemory);
    int blockIndex = getMemBlockIndex(freeBlock);
    int startIndex = blockIndex;
    int blockSize = GET_SIZE(*freeBlock);
    int tempBlockIndex;

//    printf("Index of block to free: %d\n", blockIndex);

    /*
     * Coalesce the current block of memory with the previous and next blocks.
     * Caalescing previous block:
     *      1. Update starting index.
     *      2. Update block size.
     *      3. Update the new block's next pointer with the current block's next pointer.
     * Calescing next block:
     *      1. Update block size.
     *      2. Update the new block's next pointer with the second block's next pointer.
     */

    // Coalesce previous block. tempBlockIndex is the index of the previous block.
    tempBlockIndex = ADDR_TO_INDEX(GET_PREV_ADDR(*freeBlock));
    if (blockIndex != tempBlockIndex)
    {
        // Check if previous block can be coalesced.
        if (!GET_ALLOC(memory[tempBlockIndex]))
        {
            startIndex = tempBlockIndex;
            blockSize += GET_SIZE(memory[tempBlockIndex]);
            SET_NEXT_ADDR(memory[startIndex], GET_NEXT_ADDR(memory[blockIndex]));
            SET_PREV_ADDR(memory[ADDR_TO_INDEX(GET_NEXT_ADDR(memory[blockIndex]))],
                    INDEX_TO_ADDR(startIndex));

            // Remove block to be coalesced from its bucket.
            removeFromBucket(tempBlockIndex);
        }
    }

    // Coalesce next block. tempBlockIndex is the index of next block.
    tempBlockIndex = ADDR_TO_INDEX(GET_NEXT_ADDR(*freeBlock));
    if (blockIndex != tempBlockIndex)
    {
        if (!GET_ALLOC(memory[tempBlockIndex]))
        {
            blockSize += GET_SIZE(memory[tempBlockIndex]);
            if (tempBlockIndex == ADDR_TO_INDEX(GET_NEXT_ADDR(memory[tempBlockIndex])))
            {
                // If this is the last block, update the next pointer to its own address.
                SET_NEXT_ADDR(memory[startIndex], INDEX_TO_ADDR(startIndex));
            }
            else
            {
                SET_NEXT_ADDR(memory[startIndex], GET_NEXT_ADDR(memory[tempBlockIndex]));

                SET_PREV_ADDR(memory[(ADDR_TO_INDEX(GET_NEXT_ADDR(memory[tempBlockIndex])))],
                        INDEX_TO_ADDR(startIndex));
            }

            // Remove block to be coalesced from its bucket.
            removeFromBucket(tempBlockIndex);
        }
    }

    // Set header of the new coalesced block.
    SET_SIZE(memory[startIndex], blockSize);
    SET_ALLOC(memory[startIndex], 0);
    placeInBucket(memory[startIndex], INDEX_TO_ADDR(startIndex));
    checkBuckets();
}
