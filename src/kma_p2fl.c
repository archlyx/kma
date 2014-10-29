/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the power-of-two free list
 *             algorithm
 *    Author: Stefan Birrer
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    Revision 1.2  2009/10/31 21:28:52  jot836
 *    This is the current version of KMA project 3.
 *    It includes:
 *    - the most up-to-date handout (F'09)
 *    - updated skeleton including
 *        file-driven test harness,
 *        trace generator script,
 *        support for evaluating efficiency of algorithm (wasted memory),
 *        gnuplot support for plotting allocation and waste,
 *        set of traces for all students to use (including a makefile and README of the settings),
 *    - different version of the testsuite for use on the submission site, including:
 *        scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *    Revision 1.1  2005/10/24 16:07:09  sbirrer
 *    - skeleton
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 ***************************************************************************/
#ifdef KMA_P2FL
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */
#define MINBUFSIZE 32;

typedef struct
{
  unsigned int page_counter;
  kma_page_t* page;
  struct free_list_t* free_lists;
} global_header_t;

typedef struct
{
  kma_size_t size;
  struct global_header_t* next_size;
  struct buffer_header_t* first_buffer;
} free_list_t;

typedef struct
{
  kma_page_t* page; 
  struct buffer_header_t* next_buffer;
} buffer_header_t;

/************Global Variables*********************************************/
global_header_t* global_header = NULL;

/************Function Prototypes******************************************/
void init_free_lists();
buffer_header_t* build_free_list(kma_size_t);

kma_size_t select_buffer_size(kma_size_t);
void* find_buffer(kma_size_t);

void remove_page(buffer_header_t* buffer, kma_page_t* page);
unsigned int is_last_buffer(buffer_header_t* buffer, kma_page_t* page);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void
init_free_lists()
{
  unsigned int size = MINBUFSIZE;
  unsigned int offset = sizeof(global_header_t);

  free_list_t* current_list;

  /* Request a page for managing the freelists globally */
  kma_page_t* page = get_page();

  /* Fill in the first free list */
  free_lists = global_header->free_lists;

  /* Fill in the header for the free lists in each size */
  current_list = free_lists;
  while (size <= PAGESIZE)
  {
    current_list->next_buffer = NULL;
    current_list->next_size = (global_header_t*)(page->ptr + offset);
    current_list->size = size;

    current_list = current_list->next_size;
    offset = offset + sizeof(global_header_t);
    size = size * 2;
  }
  current_list->next_size = NULL;
}

buffer_header_t*
build_free_list(kma_size_t size)
{
  unsigned int offset = size;

  kma_page_t* page = get_page();
  if (page == NULL) return NULL;

  buffer_header_t* current_buffer = page->ptr;

  while (offset < PAGESIZE)
  {
    current_buffer->next_buffer = (buffer_header_t*)(page->ptr + offset);
    current_buffer->page = page;

    offset = offset + size;
  }
  current_buffer->next_buffer = NULL;
  return (buffer_header_t*)(page->ptr);
}

void*
kma_malloc(kma_size_t size)
{
  kma_size_t buffer_size;

  /* If there is no free lists available, initialize them */
  if (free_lists == NULL)
    init_free_lists();
  
  /* Select the proper size for the request size */
  buffer_size = select_buffer_size(size);
  
  /* Pick up the proper size buffer from the free list */
  if (buffer_size != -1)
    return find_buffer(buffer_size);
  
  return NULL;
}

kma_size_t
select_buffer_size(kma_size_t size)
{
  /* Starting from the minimal buffer size, we look for
   * the proper 2^n size to fit the incoming size */
  kma_size_t buffer_size = MINBUFSIZE;
  while (buffer_size <= PAGESIZE)
  {
    if (buffer_size >= (size + sizeof(buffer_header_t)))
      return buffer_size;
    buffer_size = buffer_size * 2;
  }
  return -1;
}

void*
find_buffer(kma_size_t buffer_size)
{
  global_header_t* current_list = free_lists;
  buffer_header_t* current_buffer;

  /* Traverse the free lists to find the one with proper size */
  while (current_list->size != buffer_size)
    current_list = current_list->next_size;

  /* In the proper free list, check if there is free buffer */
  current_buffer = (buffer_header_t*)(current_list->first_buffer);
  if (current_buffer == NULL)
  {
    /* Build up the free lists if there is no free buffer */
    current_buffer = build_free_list(buffer_size);
    
    if (current_buffer == NULL)
      return NULL;
  }

  /* Connect the free list head with the next buffer, and
   * remove the current buffer from the free list */
  current_list->first_buffer = current_buffer->next_buffer;

  /* Reconnect the next_buffer to the free list such that
   * it can be freed later easily */
  current_buffer->next_buffer = current_list;

  return (current_buffer + sizeof(buffer_header_t));
}

void
kma_free(void* ptr, kma_size_t size)
{
  /* Get the header of the buffer */
  buffer_header_t* buffer = ptr - sizeof(buffer_header_t);

  /* Get the header of the corresponding free list */
  global_header_t* free_list = buffer->next_buffer;

  /* Add the buffer to the beginning of the free list */
  buffer->next_buffer = free_list->first_buffer;
  free_list->first_buffer = buffer;

  /* If this is the last nonfree buffer in the page, we need 
   * to free the page after free the buffer */
  if (is_last_buffer(free_list, buffer->page))
    remove_page(free_list, buffer->page);

  /* Free the global header page if there is no used buffer */
  if ()
  {
    free_page(free_lists->page);
    free_lists = NULL;
  }
}

unsigned int
is_last_buffer(global_header_t* free_list, kma_page_t* page)
{
  buffer_header_t* buffer = free_list->first_buffer;

  unsigned int page_id = page->id;
  unsigned int free_space = 0;

  /* Sum up all of the available free buffer on the given page
   * and test if it is equal to the page size */
  while (buffer != NULL)
  {
    if (buffer->page->id == page_id)
      free_space = free_space + free_list->size;

    buffer = buffer->next_buffer;
  }

  return ((free_space == PAGESIZE) ? 1 : 0);
}

void
remove_page(global_header_t* free_list, kma_page_t* page)
{
  unsigned int page_id = page->id;
  buffer_header_t* prev_buffer = NULL;
  buffer_header_t* current_buffer = free_list -> first_buffer; 

  /* First we need to remove those free buffers in the free list */
  while (current_buffer)
  {
    if ((current_buffer->page->id == page_id) && (prev_buffer != NULL))
    {
      /* In many cases there are several buffers in the same page connecting
       * continuously in the free list, thus we can remove them together */
      while ((current_buffer->page->id == page_id) && (current_buffer != NULL))
        current_buffer = current_buffer->next_buffer;

      if (current_buffer)
        prev_buffer->next_buffer = current_buffer;
      else
      {
        /* Arrive at the end of the free list, break */
        prev_buffer->next_buffer = NULL;
        break;
      }
    }
    prev_buffer = current_buffer;
    current_buffer = current_buffer->next_buffer;
  }

  buffer_list->next_buffer->size--;
  free_page(page);
}

#endif // KMA_P2FL
