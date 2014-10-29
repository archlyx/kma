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
  kma_size_t size;
  struct global_header_t* next_size;
  struct buffer_header_t* first_buffer;
} global_header_t;

typedef struct
{
  kma_page_t* page; 
  struct buffer_header_t* next_buffer;
} buffer_header_t;

/************Global Variables*********************************************/
global_header_t* free_lists = NULL;

/************Function Prototypes******************************************/
void init_free_lists();
buffer_header_t* build_free_list(kma_size_t);

kma_size_t select_buffer_size(kma_size_t);
void* find_buffer(kma_size_t);

void remove_free_lists();
void remove_page(buffer_header_t* buffer, kma_page_t* page);
unsigned int is_last_buffer(buffer_header_t* buffer, kma_page_t* page);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void
init_free_lists()
{
  unsigned int size = MINBUFSIZE;
  unsigned int offset = sizeof(global_header_t);

  global_header_t* current_list;

  /* Request a page for managing the freelists globally */
  kma_page_t* page = get_page();

  /* Fill in the first free list */
  free_lists = (global_header_t*)(page->ptr);

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
  kma_page_t* page = get_page();
  if (page == NULL) return NULL;

  buffer_list->next_buffer->size++;
  buffer_t* top = page->ptr;

  top->next_size = NULL;
  top->size = size;
  top->page = page;
  int offset = size;
  buffer_t* current;

  while (offset < PAGESIZE)
  {
    current = page->ptr + offset;
    top->next_buffer = current;
    top = current;
    offset += size;
    current->next_size = NULL;
    current->size = size;
    current->page = page;
  }
  top->next_buffer = NULL;
  return (buffer_t *) page->ptr;
}

void*
kma_malloc(kma_size_t size)
{
  if (buffer_list == NULL)
    init_buffer_list();
  
  kma_size_t block_size = choose_block_size(size);
  
  if (block_size != -1)
    return alloc_block(block_size);
  
  return NULL;
}

void
init_buffer_list(void)
{
  kma_page_t* page = get_page();
  buffer_list = page->ptr;

  int offset = sizeof(buffer_t);

  buffer_list->next_buffer = page->ptr + offset;
  buffer_list->page = page;
  buffer_list->size = 0;

  buffer_t* current = buffer_list;
  offset += sizeof(buffer_t);
  int size = MINBLOCKSIZE;
  while (size <= PAGESIZE)
  {
    current->next_size = page->ptr + offset;
    current = current->next_size;
    current->next_buffer = NULL;
    current->size = size;
    current->page = page;
    size *= 2;
    offset += sizeof(buffer_t);
  }
  current->next_size = NULL;
}

kma_size_t
choose_block_size(kma_size_t size)
{
  int test_size = MINBLOCKSIZE;
  while(test_size <= PAGESIZE)
  {
    if (test_size >= (size + sizeof(buffer_t)))
      return test_size;
    test_size *= 2;
  }
  return -1;
}

void*
alloc_block(kma_size_t block_size)
{
  buffer_t* top = buffer_list;
  while(top->size < block_size)
    top = top->next_size;
  buffer_t* buf = top->next_buffer;
  if (buf == NULL)
  {
    buf = make_buffers(top->size);
    
    if (buf == NULL)
      return NULL;
  }

  top->next_buffer = buf->next_buffer;
  buf->next_buffer = top;

  return ((void*)buf + sizeof(buffer_t));
}

buffer_t* make_buffers(kma_size_t size)
{
  kma_page_t* page = get_page();
  if(page == NULL)
      return NULL;

  buffer_list->next_buffer->size++;
  buffer_t* top = page->ptr;

  top->next_size = NULL;
  top->size = size;
  top->page = page;
  int offset = size;
  buffer_t* current;

  while(offset < PAGESIZE)
  {
    current = page->ptr + offset;
    top->next_buffer = current;
    top = current;
    offset += size;
    current->next_size = NULL;
    current->size = size;
    current->page = page;
  }
  top->next_buffer = NULL;
  return (buffer_t *) page->ptr;
}

void
kma_free(void* ptr, kma_size_t size)
{
  buffer_t* buf;

  //retrace to the buffer header
  buf = (buffer_t*)(ptr - sizeof(buffer_t));

  //retrace to the size header
  buffer_t* size_header = buf->next_buffer;

  //connect the size header to the buffer header
  buf->next_buffer = size_header->next_buffer;
  size_header->next_buffer = buf;

  //if this is the last free buffer in the buffer list
  if(last_buf(size_header, buf->page) == 1)
      //free the page associated with the particular size header
      free_page_from_header(size_header, buf->page);
  //if no available buffer in the array  
  if(buffer_list->next_buffer->size == 0)
      //remove the buffer list
      remove_buffer_list();
}

int last_buf(buffer_t* size_header, kma_page_t* page)
{
  buffer_t* buf = size_header->next_buffer;
  //by counting all the avaialbe free buffer size
  kma_size_t used_sofar = 0;

  while(buf != NULL)
  {
      if(buf->page == page)
        used_sofar += size_header->size;
      buf = buf->next_buffer;
  }
  //if it equal to page size, then means no buffer is used
  if (used_sofar == PAGESIZE)
    return 1;
  return 0;
}

void free_page_from_header(buffer_t* size_header, kma_page_t* page)
{
  buffer_t* prev = size_header;
  buffer_t* top = size_header->next_buffer;
  //remove all the buffers in the list
  while(top != NULL){
    if(top->page == page)
    {
      while(top != NULL && top->page == page)
          top = top->next_buffer;
      if(top != NULL){
        prev->next_buffer = top;
        prev = top;
        top = top->next_buffer;
      }
      else
        prev->next_buffer = NULL;
    }
    else{
      prev = top;
      top = top->next_buffer;
    }
  }
  buffer_list->next_buffer->size--;
  free_page(page);
}

void remove_buffer_list(void) {
  free_page(buffer_list->page);
  buffer_list = NULL;
}

#endif // KMA_P2FL
