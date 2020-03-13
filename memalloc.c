#include <unistd.h>
#include <string.h>
#include <pthread.h>

typedef struct block
{
	size_t size;		// How many bytes beyond this block have been allocated in the heap
	struct block *next; // Where is the next block in your linked list
	int free;			// Is this memory free?
} block_t;

#define BLOCK_SIZE sizeof(block_t)

// Global variables to keep track of
block_t *head = NULL;
block_t *tail = NULL;
pthread_mutex_t global_lock;

// A function that would find the best free block for a given size. It will return NULL when nothing has been found.
block_t *findFreeBlock(size_t s)
{
	block_t *curr = head;
	block_t *best = NULL;
	int counter = 0;

	// Iterate throughout the list
	while (curr != NULL)
	{
		// If the currrent block is free and the size is greater or equal to the size we want to find...
		if (curr->free && curr->size >= s)
		{
			// If there is no recorded best block, make the current block the best block
			if (best == NULL)
			{
				best = curr;
			}
			// Else if the best block's size is bigger than the current block, update the best block
			else if (best->size > curr->size)
			{
				best = curr;
			}
		}
		// Advance the current blokc
		curr = curr->next;
		counter++;
	}
	// Return the best block.
	return best;
}

// A malloc function that find the best-fit available spaces
void *mymalloc(size_t s)
{
	// If the size that is being requested is 0, it will return null.
	if (s == 0)
	{
		return NULL;
	}

	// We get the lock before continuing.
	pthread_mutex_lock(&global_lock);

	// We have the free block be the variable header
	block_t *freeBlock;
	freeBlock = findFreeBlock(s);

	// If there is a free block...
	if (freeBlock != NULL)
	{
		// Then the free block is no longer free, and we return the free block.
		freeBlock->free = 0;
		pthread_mutex_unlock(&global_lock);
		printf("malloc %zu bytes\n", s);

		// We return freeBlock + 1, as we don't want to include the freeBlock itself
		return freeBlock + 1;
	}

	// If the free block is not found...
	else
	{
		// We increase the heap to the new totalSize.
		block_t *newBlock;
		newBlock = sbrk(BLOCK_SIZE + s);

		// If the sbrk does work...
		if (newBlock != NULL)
		{
			// Update the freeBlock to be the other block.
			freeBlock = newBlock;
			freeBlock->size = s;
			freeBlock->free = 0;
			freeBlock->next = NULL;

			// If the head has not been established, the new block is the head
			if (head == NULL)
			{
				head = freeBlock;
			}

			// If the tail is already established, link the previous tail to new tail
			if (tail != NULL)
			{
				tail->next = freeBlock;
			}
			// Update tail

			tail = freeBlock;
			printf("malloc %zu bytes\n", s);
			pthread_mutex_unlock(&global_lock);

			return (freeBlock + 1);
		}

		// Else if the sbrk doesn't work, then return NULL
		else
		{
			pthread_mutex_unlock(&global_lock);
			return NULL;
		}
	}
}

// A function that would find the best-fit for the calloc.
void *mycalloc(size_t num, size_t s)
{
	// If the size of the array or the data allocated is 0, return null.
	if (!num || !s)
	{
		return NULL;
	}

	// Else just malloc the total size of the entire array.
	size_t totalSize = num * s;
	block_t *freeBlock;
	freeBlock = mymalloc(totalSize);
	printf("calloc %zu bytes\n", totalSize);
	memset(freeBlock, 0, totalSize);
	return freeBlock;
}

// A function that would free the function
void myfree(block_t *ptr)
{
	if (!ptr)
	{
		return;
	}

	block_t *delete;
	delete = (block_t *)ptr - 1;
	delete->free = 1;
	printf("Freed: %zu\n", delete->size);
}
