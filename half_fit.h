#ifndef HALF_FIT_H_
#define HALF_FIT_H_

#include <stdint.h>

typedef uint32_t mem_block_t;

typedef struct bucket_ptr{
	int head;
	int tail;
} bucket_t;

void  half_init( void );
void *half_alloc( int );
// or void *half_alloc( unsigned int );
void  half_free( void * );

#endif
