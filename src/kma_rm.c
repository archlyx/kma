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
#include <stdint.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

typedef struct buffer_t
{
  struct buffer_t* next_buffer;
  struct buffer_t* prev_buffer;
  kma_page_t* page;
  uint16_t size;
} buffer_header_t;

typedef struct
{
  kma_page_t* next_page;
  buffer_header_t* first_buffer;
  unsigned int page_counter;
} page_header_t;

/************Global Variables*********************************************/
page_header_t* first_page;

/************Function Prototypes******************************************/
void init_page_header(kma_page_t*);
buffer_header_t* find_buffer(kma_size_t);
void remove_page(kma_page_t*);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
  kma_page_t* page;
  buffer_header_t* buffer;
  
  if ((size + sizeof(page_header_t*)) > PAGESIZE)
    return NULL;

  if (first_page == NULL) {
    page = get_page();
    first_page = page;
    init_page_header(page);
  }

  buffer = find_buffer(size);

  return (void*)buffer + sizeof(buffer_header_t);
}

void
init_page_header(kma_page_t* page)
{
  page_header_t* page_header = page->ptr;
  page_header->next_page = NULL;
  page_header->page_counter = 1;

  page_header-> 
}

void
kma_free(void* ptr, kma_size_t size)
{
  ;
}

#endif // KMA_RM
