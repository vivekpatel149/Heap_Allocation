#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct block *)(ptr) - 1)


static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0; 
static int num_splits        = 0;
static int num_coalesces     = 0; 
static int num_blocks        = 0; 
static int num_requested     = 0; 
static int max_heap          = 0; 

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct block 
{
   size_t      size;  /* Size of the allocated block of memory in bytes */
   struct block *next;  /* Pointer to the next block of allcated memory   */
   bool        free;  /* Is this block free?                     */
};


struct block *FreeList = NULL; /* Free list to track the blocks available */
struct block *last_assigned = NULL; /* Last assigned pointer to the lastly assigned block for the next fit */ 

/* 
    Function to coalesces the free memory
    if the available block is free and the size is smaller then the required size
    then check if the next  block is free and merge them.
    Continue the process till you find the end of the memory or adjacent block is not free
    Prameter info:
    "curr" is the pointer to the block which is free and the next block is free
    "available choice" is the pointer to the best block available for the user for the defined fit
    "size" is the size of the block requested by the user
    "Type" for the type of "fit" user is using
    if type = 1 then it is "best fit" in which return
    if type = 2 then it is "worst fit"
    if type = 0 then it is "first fit" or "next fit"
*/
struct block *coaleces(struct block* curr, struct block* available_choice, size_t size, int type)
{
          size_t total_concurrent_size = curr->size;
          //Pointer to the Next block to the free block 
          struct block *Next = curr->next;
          int in_while = 1;
          int got = 0; //Going to determine how many times we are going to coalesce the blocks
          while( in_while )
          {
             if( Next->free )
             {
                 //Coalesces formation
                 //increasing the size of the block by adding the size of the next block and size of the structure
                 total_concurrent_size = total_concurrent_size + Next->size + sizeof(struct block);
                 //If it is free then do the coalesceing and incresing the coalesceing number
                 got = got + 1; 
                 curr->size = total_concurrent_size;
                 curr->next = Next->next;
                 //After merging go to the next block
                 Next = Next->next;
             }
             else
             {
                 in_while = 0;
             }  
          } 
          //Updating the global variable, who is taking care of coalesces 
          num_coalesces += got;
          //Decrease the number of blocks when you coalesces
          num_blocks -= got;
   
          if( !available_choice && curr->size >= size )
          {
              available_choice = curr;
          }         
          else if(curr->size >= size && type == 0) //for first fit and next fit
          {
             available_choice = curr;
          }
          else if(curr->size < available_choice->size && curr->size >= size && type == 1) // for best fit 
          {
             available_choice = curr;
          }
          else if(curr->size > available_choice->size && curr->size >= size && type == 2) // for worst fit
          {
             available_choice = curr;
          }
          
          return available_choice;
}

/* 
    Function to Split memory block
    if the available block size is greater then the required size
    then split the size and give user a block which is requested and split another block and make it free
    Prameter info:
    "available choice" is the pointer to the block which is free and a good choice  
    "size" requested size by the user
*/
struct block *split(struct block* available_choice, size_t size)
{
          size_t size_available = available_choice->size - size - sizeof(struct block);
          if( size_available >= 0 )
          {
             num_splits += 1;
             num_blocks += 1;
   
             //Setting the size of the best available block to the requested size
             available_choice->size = size;
             //Temporary pointer to new block for splitting the block
             struct block *available_next;
             available_next->next = available_choice->next;
             available_choice->next = available_next; 
             available_next->size = size_available ;
             available_next->free = true; 
          }
          else
          {
             //Over here we are giving the block with larger size 
             return available_choice;
          }           

          return available_choice;
}

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free blocks
 * \param size size of the block needed in bytes 
 *
 * \return a block that fits the request or NULL if no free block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct block *findFreeBlock(struct block **last, size_t size) 
{
   struct block *curr = FreeList;
   /* Pointer to a block which will store the best available chocie to allocate for the user*/
   struct block *available_choice = NULL;
     
#if defined FIT && FIT == 0
   /* First fit */
   while (curr ) 
   {
      if(curr->free && curr->size >= size)
      {
         return curr;
      }
      else if(curr->free && curr->next->free)
      {
          //Coalesces formation
          available_choice = coaleces(curr, available_choice, size, 0);
          if( available_choice )
          {
              if( available_choice->size == size)
              { 
                  return available_choice;
              }
              else
              {
                  return split(available_choice, size);
              }           
           }
       }
       *last = curr;
       curr  = curr->next;
   }
#endif

#if defined BEST && BEST == 0
   /* Best fit */
   
   /* If curr is not null then check for the best available block in the heap*/
   /* It will run in linear time  as it will check all the avaible blocks in the heap*/
   while(curr)
   {
       if( curr->free && curr->size >= size )
       {  
          /* If you get the perfect block just return it */
          if( curr->size == size) 
          {
             return cur;
          }
          /* If there is nothing in availbale choice then assign the given block which is free and bigger or equal then the required size */
          /* If the free block size is smaller then best_choice size then assign the smaller block as a best choice */ 
          if( !available_choice )
          {
             available_choice = curr;
          }         
          else if(curr->size < available_choice->size)
          {
             available_choice = curr;
          } 
       }
       else if( curr->free && (curr->next)->free )
       { 
           //Coalesces formation
           available_choice = coaleces(curr, available_choice, size, 1);
       }
        
       *last = curr;
       curr  = curr->next;
   }
   
   //finally allocating the perfect size block for best fit
   if( available_choice  )
   {
       if( available_choice->size == size)
       { 
          curr = available_choice;
       }
       else
       {
          curr = split(available_choice, size);
       }       
   }
   else
   {
       curr =  available_choice;
   }
#endif

#if defined WORST && WORST == 0
   /* If curr is not null then check for the best available block for worst control in the heap*/
   /* It will run in linear time  as it will check all the avaible blocks in the heap*/
   while(curr)
   {
       if( curr->free && curr->size >= size )
       {  
          /* If there is nothing in availbale choice then assign the given block which is free and bigger or equal then the required size */
          /* If the free block size is greaterer then worst_choice size then assign the greater block as a worst choice */ 
          if( !available_choice )
          {
             available_choice = curr;
          }         
          else if(curr->size > available_choice->size)
          {
             available_choice = curr;
          } 
       }
       else if( curr->free && (curr->next)->free )
       { 
           //Coalesces formation
           available_choice = coaleces(curr, available_choice, size, 2);
       }
        
       *last = curr;
       curr  = curr->next;
   }
   
   //finally allocating the perfect size block for worst fit
   if( available_choice  )
   {
       if( available_choice->size == size)
       { 
          curr = available_choice;
       }
       else
       {
          curr = split(available_choice, size);
       }       
   }
   else
   {
       curr =  available_choice;
   }
#endif

#if defined NEXT && NEXT == 0
   /* Next Fit */
   curr = last_assigned ; 
   while (curr && !(curr->free && curr->size >= size)) 
   {
      if(curr->free && curr->size >= size)
      {
         return curr;
      }
      else if(curr->free && curr->next->free)
      {
          //Coalesces formation
          available_choice = coaleces(curr, available_choice, size, 0);
          if( available_choice )
          {
              if( available_choice->size == size)
              { 
                  return available_choice;
              }
              else
              {
                  return split(available_choice, size);
              }           
           }
       }
    
       *last = curr;
       curr  = curr->next;
   }
   last_assigned = curr ;
#endif

   return curr;
}


/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated block of NULL if failed
 */
struct block *growHeap(struct block *last, size_t size) 
{
   /* Request more space from OS */
   struct block *curr = (struct block *)sbrk(0);
   struct block *prev = (struct block *)sbrk(sizeof(struct block) + size);

   assert(curr == prev); 
   /* OS allocation failed */
   if (curr == (struct block *)-1) 
   {
      return NULL;
   }
   else
   {
      //Increase the heap size and how many time it grows?
      max_heap = max_heap + (int) size;
      num_grows = num_gorws + 1;
      num_blocks += 1;
   }

   /* Update FreeList if not set */
   if (FreeList == NULL) 
   {
      FreeList = curr;
   }

   /* Attach new block to prev block */
   if (last) 
   {
      last->next = curr;
   }

   /* Update block metadata */
   curr->size = size;
   curr->next = NULL;
   curr->free = false; 
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free block of heap memory for the calling process.
 * if there is no free block that satisfies the request then grows the 
 * heap and returns a new block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{
   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Incrementing number of mallocs done*/
   num_mallocs = num_mallocs + 1;

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   /* Look for free block */
   struct block *last = FreeList;
   struct block *next = findFreeBlock(&last, size);

   /* TODO: Split free block if possible */

   /* Could not find free block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
   }
   else
   {
      //Increment the reuse pointer
      num_reuses = num_reuses + 1;
   }
   /* Could not find free block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }
   
   /* Mark block as in use */
   next->free = false;

   /* Return data address associated with block */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory block pointed to by pointer. if the block is adjacent
 * to another block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{
   
   if (ptr == NULL) 
   {
      return;
   }
   /* Incrementing number of frees done*/
   num_frees = num_frees + 1;
   
   /* Make block as free */
   struct block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   /* TODO: Coalesce free blocks if needed */
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
