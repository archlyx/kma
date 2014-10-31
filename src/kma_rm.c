/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the resource map algorithm
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
#ifdef KMA_RM
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

typedef struct 
{
  void* next;
  void* prev;
  int size;  
} buffer_header_t;

typedef struct 
{
  void* this;
  int page_counter;
  int buffer_counter;
  void* first_free_buffer;
} page_header_t;

/************Global Variables*********************************************/
kma_page_t* kma_page_entry = NULL;

/************Function Prototypes******************************************/
/* Add the free buffer to the free list */
void add_buffer (void* addr, int size);
/* Remove the used buffer from the free list */
void remove_buffer (void* addr);

/* Initialize the header of the page */
void init_page_header(kma_page_t *page);
/* Find the free buffer with suitable size */
void* find_buffer(int size); 

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void
add_buffer (void* buffer, int size)
{
  /* First set up the free buffer header */
  ((buffer_header_t*)buffer)->size = size;
  ((buffer_header_t*)buffer)->prev = NULL;

  /* If the free buffer is before the first free buffer */
  page_header_t* entry_page = (page_header_t*)kma_page_entry->ptr;
  void* current_buffer = (void*)entry_page->first_free_buffer;
  if (buffer < current_buffer)
  {
    ((buffer_header_t*)(entry_page->first_free_buffer))->prev = (buffer_header_t*)buffer;
    ((buffer_header_t*)buffer)->next = ((buffer_header_t*)(entry_page->first_free_buffer));
    entry_page->first_free_buffer = (buffer_header_t*)buffer;
    return;
  }
  /* If the free buffer is the first one */
  else if (buffer == current_buffer)
  {
    ((buffer_header_t*)buffer)->next = NULL;
    return;
  }
  /* If the free buffer is somewhere in the free list */
  else
  {
    /* First find the where to insert the free buffer */
    while (((buffer_header_t*)current_buffer)->next != NULL && current_buffer < buffer)
      current_buffer = ((void*)(((buffer_header_t*)current_buffer)->next));

    /* Update the link list pointer */
    buffer_header_t* tmp = ((buffer_header_t*)current_buffer)->next;

    if (tmp != NULL)
      tmp->prev = buffer;

    ((buffer_header_t*)current_buffer)->next = buffer;
    ((buffer_header_t*)buffer)->prev = current_buffer;
    ((buffer_header_t*)buffer)->next = tmp;
  }
}

void
remove_buffer(void* buffer)
{
  buffer_header_t* ptr = (buffer_header_t*) buffer;
  buffer_header_t* next_buffer = ptr->next;
  buffer_header_t* prev_buffer = ptr->prev;

  /* If there is only one buffer there */ 
  if (prev_buffer == NULL && next_buffer == NULL)
  {
    page_header_t* tmp_page = kma_page_entry->ptr; 
    tmp_page->first_free_buffer = NULL;
    kma_page_entry = 0;

    return;
  }
  /* If the buffer is the last one */ 
  else if (next_buffer == NULL)
  {
    prev_buffer->next = NULL;
    return;
  }
  /* If the buffer is the first one */ 
  else if (prev_buffer == NULL)
  {
    page_header_t* tmp_page = kma_page_entry->ptr; 
    next_buffer->prev = NULL;
    tmp_page->first_free_buffer = next_buffer;
    return;
  }
  /* If the buffer is somewhere in the free list */ 
  else
  {
    buffer_header_t* tmp1 = ptr->prev;
    buffer_header_t* tmp2 = ptr->next;

    tmp1->next = tmp2;
    tmp2->prev = tmp1;
  }
}

void*
kma_malloc(kma_size_t size)
{
  /* If the request size is too large, return NULL */
  if ((size + sizeof(void *)) > PAGESIZE) {
    return NULL;
  }    

  /* Initialize the global entry if not exist */
  if (kma_page_entry == NULL) {
    kma_page_t* page = get_page();
    kma_page_entry = page;
    init_page_header(page);
  }

  /* Find the suitable free buffer */
  void *buffer_addr;
  buffer_addr = find_buffer(size);

  /* Increment the buffer counter */
  page_header_t* page = BASEADDR(buffer_addr);
  (page->buffer_counter)++;

  return buffer_addr;
}

void
init_page_header(kma_page_t *page) {
  /* Set up the pointer to the current page */
  page_header_t *pagehead;
  *((kma_page_t**) page->ptr) = page;

  /* Set up the counters */
  pagehead = (page_header_t*) (page->ptr);
  pagehead->page_counter = 0;
  pagehead->buffer_counter = 0;

  /* Add the free buffer to the list */
  pagehead->first_free_buffer = (buffer_header_t*)((long int)pagehead + sizeof(page_header_t));
  add_buffer(((void*)(pagehead->first_free_buffer)),(PAGESIZE - sizeof(page_header_t)));

}

void*
find_buffer(int size) {
  int header_size = sizeof(buffer_header_t);
  if (size < sizeof(buffer_header_t)) size = sizeof(buffer_header_t);

  page_header_t *entry_page;
  entry_page = (page_header_t*)(kma_page_entry->ptr);
  buffer_header_t* current_buffer = ((buffer_header_t *)(entry_page->first_free_buffer));

  while (current_buffer)
  {
    /* If the buffer size is too small, continue */
    if (current_buffer->size < size)
    {
      current_buffer = current_buffer->next;
      continue;
    }
    /* If the buffer size is fairly good, remove it from the free list */
    else if (current_buffer->size == size || (current_buffer->size - size) < header_size)
    {
      remove_buffer(current_buffer);
      return ((void*)current_buffer);
    }
    /* If the buffer size is too large, remove it from the free list
     * and add the rest of the space to the free list */
    else
    {
      add_buffer((void*)((long int)current_buffer + size), (current_buffer->size - size));
      remove_buffer((current_buffer));
      return((void*)current_buffer);
    }
  }

  /* Request a new page and reassign the free buffer */
  kma_page_t* new_page = get_page();
  init_page_header(new_page);
  entry_page->page_counter++;
  return find_buffer(size);
}

void
kma_free(void* ptr, kma_size_t size)
{
  /* Add the given buffer to the free list */
  add_buffer(ptr, size);
  page_header_t* base_addr = BASEADDR(ptr);
  /* Decrement the buffer counter in that page */
  base_addr->buffer_counter = base_addr->buffer_counter - 1;

  /* Now let's check if the page needs to be freed */
  page_header_t* first_page = (page_header_t*)(kma_page_entry->ptr);
  int is_continue = 1, num_page = first_page->page_counter;

  page_header_t* last_page;
  for (; is_continue; num_page--) {
    /* First go to the last page */
    last_page = (((page_header_t*)((long int)first_page + num_page * PAGESIZE)));
    is_continue = 0;

    /* If the last page is empty, conitnue to free it */
    if(last_page->buffer_counter == 0){
      is_continue = 1;
      
    /* Go over all the free buffers belonging to that page */
    buffer_header_t* current_buffer;
    for(current_buffer = first_page->first_free_buffer; current_buffer != NULL;
        current_buffer = current_buffer->next)
    {
      if(BASEADDR(current_buffer) == last_page) remove_buffer(current_buffer);
    }
    
    /* If this is the first page, we need to free everything */
    if (last_page == first_page)
    {
      is_continue = 0;
      kma_page_entry = NULL;
    }

    free_page(last_page->this);

    /* If this is not the first page, just decrement the number of pages */
    if(kma_page_entry != NULL)
      first_page->page_counter -= 1;

    }
  }
}
#endif // KMA_RM
