/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the buddy algorithm
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
#ifdef KMA_BUD
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */
/* The minimal buffer size */
#define MINBUFSIZE 64
/* The number of minimal buffers in each page */
#define NUMBEROFBUF PAGESIZE / MINBUFSIZE

/* Calculte the index of the children and parent nodes */
#define LEFT_CHILD(n) ((n) * 2 + 1)
#define RIGHT_CHILD(n) ((n) * 2 + 2)
#define PARENT(n) (((n) + 1) / 2 - 1)

/* Calculte the offset within the page for given index */
#define OFFSET(n, size) (((n) + 1) * (size) - PAGESIZE)
/* Test if the give size is power of 2 */
#define IS_TWO_POWER(x) (!((x) & ((x) - 1)))
/* Calculate the mod of x w.r.t. 2^n */
#define MOD(x, n) ((x) & ((n) - 1))
/* Calculate the larger number of x and y*/
#define LARGER(x, y) ((x) > (y) ? (x) : (y))

/* The header in each page:
 * kma_page_t* next_page: pointer to next page
 * kma_page_t* prev_page: pointer to previous page
 * uint8_t large: mark the page that is large enough to include the header and the request mem
 * uint16_t longest_length: an array containing the available space in each node */
typedef struct {
  kma_page_t* next_page;
  kma_page_t* prev_page;
  uint8_t large;
  uint16_t longest_length[2 * NUMBEROFBUF - 1];
} page_header_t;

/************Global Variables*********************************************/
kma_page_t* first_page = NULL;

/************Function Prototypes******************************************/

/* Initialize the header of the page */
void init_header(kma_page_t*, kma_page_t*);
/* Find the page that is suitable for the allocation */
kma_page_t* find_alloc_page(kma_size_t);
/* Find the page that contains the memory to free */
kma_page_t* find_free_page(void*);
/* Remove a page from the link list */
void remove_page(kma_page_t*);

/* Round up the given size to 2^n */
static unsigned int round_size(unsigned int);
/* Find the real size of a node due to the header offset */
unsigned int real_size(unsigned int, unsigned int);
	
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void
init_header(kma_page_t* page, kma_page_t* prev_page)
{
  unsigned int i, node_size = 2 * PAGESIZE;
  unsigned int offset, pre_filled_offset;
  int effective_length;

  /* Fill the header to the page */
  page_header_t* page_header;
  page_header = (page_header_t*)(page->ptr);

  /* This new page will be in the end of the page list */
  page_header->prev_page = prev_page;
  page_header->next_page = NULL;

  /* Intially assume the page is large enough. Will change
   * the value of ->large later */
  page_header->large = 0;

  /* The page is pre-filled by the header which needs to be
   * avoided during allocation */
  pre_filled_offset = sizeof(page_header_t);

  /* Initialize the longest_length array */
  for (i = 0; i < 2 * NUMBEROFBUF - 1; i++)
  {
    if (IS_TWO_POWER(i + 1)) node_size = node_size / 2;
    
    /* If the node has intersection with the header, we need
     * to adjust its available space to effective_length*/
    offset = OFFSET(i, node_size);
    if (offset < pre_filled_offset)
    {
      effective_length = offset + node_size - pre_filled_offset;
      page_header->longest_length[i] = effective_length > 0 ? effective_length : 0;
    } else
      page_header->longest_length[i] = node_size;
  }
}

void*
kma_malloc(kma_size_t size)
{
  unsigned int node_size;
  unsigned int power_size;
  unsigned int offset;
  unsigned int index = 0; 

  kma_page_t* page;
  page_header_t* page_header;

  /* Find proper page to allocate the mem */
  page = find_alloc_page(size);
  if (page == NULL)
    return NULL;
  else
  {
    page_header = page->ptr;
    if ((size + sizeof(page_header_t)) > PAGESIZE)
    {
      /* If the page is not large enough to contain the header and
       * the requested mem, abandon the longest_length and only use
       * the prev, next and larger in the header */
      page_header->large = 1;
      return page->ptr + 2 * sizeof(kma_page_t*) + sizeof(uint8_t);
    }
  }

  /* Roundup the size if it is not power of 2 */
  if (!IS_TWO_POWER(size))
    power_size = round_size((unsigned int)size);
  else
    power_size = size;
  
  /* If the round-up value is less than minimal buffer size, set it
   * to the minimal buffer size */
  if (power_size < MINBUFSIZE)
    power_size = MINBUFSIZE;

  /* Traverse the tree to find proper node index to fill */
  for (node_size = PAGESIZE; node_size != power_size; node_size /= 2)
  {
    /* It has been guaranteed that there is enough space in the page when
     * calling find_alloc_page(). So we just need to find out the child
     * node with proper size that has enough space. */
    if (page_header->longest_length[LEFT_CHILD(index)] >= size)
      index = LEFT_CHILD(index);
    else
      index = RIGHT_CHILD(index);
  }

  /* Calculate the offset value the index we have just found */
  offset = OFFSET(index, node_size) + node_size - page_header->longest_length[index];
  /* Set the available space of this node to be zero*/
  page_header->longest_length[index] = 0;

  /* Traverse back to the parent node such that the available
   * space is updated */
  while (index)
  {
    index = PARENT(index);
    page_header->longest_length[index] = LARGER(page_header->longest_length[LEFT_CHILD(index)],
                                                page_header->longest_length[RIGHT_CHILD(index)]);
  }

  return page->ptr + offset;
}

void 
kma_free(void* ptr, kma_size_t size)
{
  kma_page_t* page = find_free_page(ptr);
  page_header_t* page_header = page->ptr;
  unsigned int left_length, right_length;
  unsigned int node_size, index = 0;
  unsigned int offset;

  /* If the page is marked as large, then it only holds 
   * just one chunk of mem that need to be free. Thus
   * just remove the page */
  if (page_header->large == 1)
  {
    remove_page(page);
    return;
  }
  
  /* Find the offset of the given pointer. Round up the begin offset of
   * the memory since it may have overlap with the header */
  node_size = MINBUFSIZE;
  offset = ptr - page->ptr;
  if (MOD(offset, MINBUFSIZE) != 0)
    offset = offset - MOD(offset, MINBUFSIZE);

  index = (offset + PAGESIZE) / node_size - 1;

  /* Traverse the tree from bottom up. If we find a node that is not free
   * with proper size, we stop */
  for (; page_header->longest_length[index] != 0; index = PARENT(index))
    node_size = node_size * 2;

  /* Calculate the real available space in the node */
  page_header->longest_length[index] = (uint16_t)real_size(index, node_size);

  /* Traverse the tree top down to update the free node */
  while (index)
  {
    index = PARENT(index);
    node_size = node_size * 2;
    left_length = page_header->longest_length[LEFT_CHILD(index)];
    right_length = page_header->longest_length[RIGHT_CHILD(index)];

    /* If the sum of the available space in the children nodes equals 
     * the available space in the parent node, coalesce them. */
    if (left_length + right_length == real_size(index, node_size))
      page_header->longest_length[index] = (uint16_t)real_size(index, node_size);
    else
      page_header->longest_length[index] = LARGER(left_length, right_length);
  }

  /* If the page is empty, remove and free it. */
  if (page_header->longest_length[0] == (PAGESIZE - sizeof(page_header_t)))
    remove_page(page);
}

kma_page_t*
find_alloc_page(kma_size_t size)
{
  kma_page_t* page = first_page;
  page_header_t* page_header;

  /* If the requested size is too large, return NULL */
  if ((size + 2 * sizeof(kma_page_t*) + sizeof(uint8_t)) > PAGESIZE)
    return NULL;

  /* If there is not first_page existing, request one */
  if (page == NULL)
  {
    page = get_page();
    first_page = page;
    init_header(page, NULL);
    return page;
  }

  /* Traverse the page link list to see if there is proper page for
   * the allocation */
  while (page)
  {
    page_header = (page_header_t*)page->ptr;

    /* If the page is marked as large, skip it. Else if the page has
     * enough space, return it. */
    if (page_header->large != 1 && page_header->longest_length[0] >= size)
      return page;

    /* If we reach the end of the page link list and find nothing, 
     * we just request new page and add it to the end of the list*/
    if (page_header->next_page == NULL)
    {
      page_header->next_page = get_page();
      init_header(page_header->next_page, page);
      return page_header->next_page;
    }

    page = page_header->next_page;
  }
  return NULL;
}

kma_page_t*
find_free_page(void* ptr)
{
  kma_page_t* page = first_page;
  page_header_t* page_header;
  int offset;

  /* Traverse the page link list to find where the ptr locates */
  while (page)
  {
    page_header = (page_header_t*)(page->ptr);
    offset = ptr - page->ptr;
    if (offset > 0 && offset < PAGESIZE)
      return page;
    page = page_header->next_page;
  }
  return NULL;
}

void
remove_page(kma_page_t* page)
{
  page_header_t* page_header = page->ptr;
  kma_page_t* next_page = page_header->next_page;
  kma_page_t* prev_page = page_header->prev_page;

  /* Link the prev and next nodes together, and remove the page */
  if (prev_page != NULL)
    ((page_header_t*)(prev_page->ptr))->next_page = next_page;

  if (next_page != NULL)
    ((page_header_t*)(next_page->ptr))->prev_page = prev_page;

  free_page(page);
}

static unsigned int
round_size(unsigned int size)
{
  /* | and >> operation can continuously make the lower-order bits
   * to 1. After that add another 1 to make it have higher-order 1.
   * e.g. 001010 -> 001111 ->(+1) 010000*/
  size = size | (size >> 1);
  size = size | (size >> 2);
  size = size | (size >> 4);
  size = size | (size >> 8);
  size = size | (size >> 16);
  return size + 1;
}

unsigned int
real_size(unsigned int index, unsigned int node_size)
{
  unsigned int size = node_size;
  unsigned int offset = OFFSET(index, node_size);
  unsigned int occupied_offset = sizeof(page_header_t);

  /* Just remove the overlapping part between the node
   * and the header */
  if (occupied_offset >= offset)
    size = size - (occupied_offset - offset);

  return size;
}

#endif // KMA_BUD
