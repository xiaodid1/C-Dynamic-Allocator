#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

/* The standard allocator interface from stdlib.h.  These are the
 * functions you must implement, more information on each function is
 * found below. They are declared here in case you want to use one
 * function in the implementation of another. */
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);

/* When requesting memory from the OS using sbrk(), request it in
 * increments of CHUNK_SIZE. */
#define CHUNK_SIZE (1<<12)

struct node{
    struct node* frontnode;
}*fmemory[8];

/*
 * This function, defined in bulk.c, allocates a contiguous memory
 * region of at least size bytes.  It MAY NOT BE USED as the allocator
 * for pool-allocated regions.  Memory allocated using bulk_alloc()
 * must be freed by bulk_free().
 *
 * This function will return NULL on failure.
 */
extern void *bulk_alloc(size_t size);

/*
 * This function is also defined in bulk.c, and it frees an allocation
 * created with bulk_alloc().  Note that the pointer passed to this
 * function MUST have been returned by bulk_alloc(), and the size MUST
 * be the same as the size passed to bulk_alloc() when that memory was
 * allocated.  Any other usage is likely to fail, and may crash your
 * program.
 */
extern void bulk_free(void *ptr, size_t size);

/*
 * This function computes the log base 2 of the allocation block size
 * for a given allocation.  To find the allocation block size from the
 * result of this function, use 1 << block_size(x).
 *
 * Note that its results are NOT meaningful for any
 * size > 4088!
 *
 * You do NOT need to understand how this function works.  If you are
 * curious, see the gcc info page and search for __builtin_clz; it
 * basically counts the number of leading binary zeroes in the value
 * passed as its argument.
 */
static inline __attribute__((unused)) int block_index(size_t x) {
    if (x <= 8) {
        return 5;
    } else {
        return 32 - __builtin_clz((unsigned int)x + 7);
    }
}

/*
 * You must implement malloc().  Your implementation of malloc() must be
 * the multi-pool allocator described in the project handout.
 */
void *malloc(size_t size) {
    void* answer; //the pointer to return
    if(size>0){   //valid request
        if(size <= (CHUNK_SIZE-8)){
            int level = block_index(size);
            int levelsize = 1 << level;
            if(fmemory==NULL){
                fmemory[0] = NULL;
                fmemory[1] = NULL;
                fmemory[2] = NULL;
                fmemory[3] = NULL;
                fmemory[4] = NULL;
                fmemory[5] = NULL;
                fmemory[6] = NULL;
                fmemory[7] = NULL;
            }
            if(fmemory[level-5]==NULL){
                void* begin = sbrk(CHUNK_SIZE);
                *(size_t*)begin = levelsize;
                for(int i = levelsize; i < 4096;i+=levelsize){
                    void* next = (begin + levelsize);
                    *(size_t*)next = levelsize;
                    ((struct node*)(next+sizeof(size_t))) -> frontnode = (struct node*)begin;
                    begin = next;
                }
                fmemory[level-5] = ((struct node*)(begin+sizeof(size_t))) -> frontnode;
                answer =  begin;
            } else if(fmemory[level-5]!=NULL){
                answer = fmemory[level-5];
                fmemory[level-5] = ((struct node*)(answer+sizeof(size_t))) -> frontnode;
                }
        } else if(size>(CHUNK_SIZE-8)){
            void* new = bulk_alloc(size+8);
            *(size_t*)(new) = size;
            answer =  new;
        }
        return answer+sizeof(size_t);
    }
    return NULL;
}

/*
 * You must also implement calloc().  It should create allocations
 * compatible with those created by malloc().  In particular, any
 * allocations of a total size <= 4088 bytes must be pool allocated,
 * while larger allocations must use the bulk allocator.
 *
 * calloc() (see man 3 calloc) returns a cleared allocation large enough
 * to hold nmemb elements of size size.  It is cleared by setting every
 * byte of the allocation to 0.  You should use the function memset()
 * for this (see man 3 memset).
 */
void *calloc(size_t nmemb, size_t size) {
    fprintf(stderr,"hi in callor");
    if(nmemb>0&&size>0){
        void* ptr = malloc(nmemb * size);
        memset(ptr, 0, nmemb * size);
        return ptr;
    }
    return NULL;
}

/*
 * You must also implement realloc().  It should create allocations
 * compatible with those created by malloc(), honoring the pool
 * alocation and bulk allocation rules.  It must move data from the
 * previously-allocated block to the newly-allocated block if it cannot
 * resize the given block directly.  See man 3 realloc for more
 * information on what this means.
 *
 * It is not possible to implement realloc() using bulk_alloc() without
 * additional metadata, so the given code is NOT a working
 * implementation!
 */
void *realloc(void *ptr, size_t size) {
    fprintf(stderr,"hi in realloc");
        if(ptr==NULL || size <= 0){
        return NULL;
    }
    size_t size1 = *(size_t*)(ptr-sizeof(size_t));
    if(size1<size){
        void* new = malloc(size);
        memcpy(new, ptr, size1);
        free(ptr);
        return new;
    }
    return ptr;
}

/*
 * You should implement a free() that can successfully free a region of
 * memory allocated by any of the above allocation routines, whether it
 * is a pool- or bulk-allocated region.
 *
 * The given implementation does nothing.
 */
void free(void *ptr) {
    size_t size = *(size_t*)(ptr-8);
    if(ptr == NULL){
        return;
    }
    if(size <= 0){
        return;
    }
    if(size<=CHUNK_SIZE){
        fprintf(stderr,"%zu",size);
        fprintf(stderr,"hi in size <= 4096\n");
        int level = block_index(size);
        ((struct node*)ptr) -> frontnode = fmemory[level-5];
        fmemory[level-5] = (struct node*)ptr;
    } else if(size > CHUNK_SIZE){
        fprintf(stderr,"hi in size > 4096\n");
        bulk_free(ptr-8, size+8);
    }
    return;
}
