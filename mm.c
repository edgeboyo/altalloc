#include <string.h>
#include <stdio.h>

/* When requesting memory from the OS using sbrk(), request it in
 * increments of CHUNK_SIZE. */
#define CHUNK_SIZE (1<<12)


#define ST_SIZE sizeof(memList)


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
 * created with bulk_alloc().  The pointer passed to this
 * function MUST have been returned by bulk_alloc(), and the size MUST
 * be the same as the size passed to bulk_alloc() when that memory was
 * allocated.  Any other usage is likely to fail, and may crash your
 * program.
 */
extern void bulk_free(void *ptr, size_t size);

/*
 * This computes the log base 2 of the allocation block size
 * for a given allocation.  To find the allocation block size from the
 * result of this function, use 1 << block_size(x).
 *
 * Note that its results are NOT meaningful for any
 * size > 4088!
 */
static inline __attribute__((unused)) int block_index(size_t x) {
    if (x <= 8) {
        return 5;
    } else {
        return 32 - __builtin_clz((unsigned int)x + 7);
    }
}

/*
 * Structure that holds the metadata
 */
typedef struct memList {
	void *memPointer; //pointer for identification
	size_t size; //size of allocation
	struct memList* next; //this is a linked list, so this points to next entry or NULL if end
} memList;

//this is where all metadata is held
memList *memoryBlock = NULL;

/*
 * This is to add value to memList structure
 * Last value is NULL, so it searches for it first
 */
void addVal(memList **ptr, void *memPtr, size_t size){
	if(*ptr == NULL){ //if NULL found
		memList *tail = (memList*) bulk_alloc(ST_SIZE); //allocate memory for next entry
		tail->memPointer = memPtr; //copy pointer values and size
		tail->size = size; 
		tail->next = NULL; //last entry, so set as NULL
		*ptr = tail; //save entry as last
		return; 
	}
	addVal(&((*ptr)->next), memPtr, size); //if not found look further
}

/*
 * Looks and returns size of pointer memory block.
 * Removes entry as well
 */
size_t findtoFree(memList **ptr, void *mptr){
	memList *cur = *ptr;
	if(*ptr == NULL){ //if not found return -1 as error handle
		return -1;
	}
	if(cur->memPointer == mptr){ //if found, find and return size and..
		size_t size = cur->size;
		*ptr = cur->next; //connect previous entry and next
		bulk_free((void *)cur, ST_SIZE); //deallocate memory
		return size;
	}
	return findtoFree(&(cur->next), mptr); //if not found look further

}

/*
 * Used for debugging. Shows entire memory status.
 * Function is recursive and continues until end of memeory queue
 */
void memoryDump(memList *ptr){
	if(ptr == NULL){
		return;
	}
	printf("PTR TO: %p\nSIZE: %ld\n\n", ptr->memPointer, ptr->size); //print info
	memoryDump(ptr->next); //go to next entry
}

void *malloc(size_t size) {
    void *ptr = bulk_alloc(size); //bulk allocate memory
    addVal(&memoryBlock, ptr, size); //add metadata to memList struct
    return ptr;
}

void *calloc(size_t nmemb, size_t size) { 
    void *ptr = malloc(nmemb * size); //allocate correct amount of memory
    memset(ptr, 0, nmemb * size); //set entire memory space as 0
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    ptr = (void *)ptr;
    size_t sizeO = findtoFree(&memoryBlock, ptr); //find old size
    if(size == -1){ //if size == -1, then display error message
        printf("ERROR OCCURED GETTING POINTER SIZE!\n");
        return NULL;
    }
    void *newPtr = malloc(size); //get new dynamic allocation size
    memcpy(newPtr, ptr, sizeO >= size ? size : sizeO); //copy memory, use the smaller of two sizes
    bulk_free(ptr, sizeO); //free up old memory block
    return newPtr;
}

void free(void *ptr) {
    ptr = (void *)ptr;
    size_t size = findtoFree(&memoryBlock, ptr); //find old size
    if(size == -1){ //if size == -1, then display error message
        printf("ERROR OCCURED GETTING POINTER SIZE!\n");
        return;
    }
    bulk_free(ptr, size); //free up memory block
    return;
}
