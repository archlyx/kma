# EECS343 Opearting System #
Project #2, Kernel Memory Allocator
Name: Shuangping Liu (2609206)

## ALGORITHMS ##
Here I have implemented **THREE** algorithms for the KMA, namely

- KMA_RM: Resource Map (First fit)
- KMA_BUD: Buddy system
- KMA_P2FL: Power of two free list (**Extra credit**)

## IMPLEMENTATION & RESULTS ##
All of the following results are produced in my own desktop with 64-bit Linux system.

### KMA_RM ###

```
COMPETITION: running KMA_RM on 5.trace
Competition binary successfully completed the trace
./kma_competition: Running in competition mode
Page Requested/Freed/In Use:  1588/ 1588/    0
Competition average ratio: 2.610596
Test: PASS

Best time (out of 5 runs): 21.71
Competition score: 78.386039
```

The first fit RM algorithm is designed based on two structs: `page_header_t` for globally managing the number of the pages and locally managing the used buffers in a certain page; `buffer_header_t` is placed at the head of each free buffer. All of the free buffers are connected by double link list, and a global pointer is used to fetch them whenever needed. The free buffer list is alway kept ordered by their addresses such that it is easier to be found later.

The efficiency and time of the RM is quite low as expected. On the one hand, the first fit policy sometimes will assign very big buffer to very small request size, which causes memory waste. On the other hand, it is always needed to traverse a quite long free list which takes quite amount of time.

### KMA_BUD ###
```
COMPETITION: running KMA_BUD on 5.trace
Competition binary successfully completed the trace
./kma_competition: Running in competition mode
Page Requested/Freed/In Use: 10217/10217/    0
Competition average ratio: 0.580203
Test: PASS

Best time (out of 5 runs): .49
Competition score: .774299
```

The buddy system algorithm is implemented using binary tree array insteand of bitmap. The buffers with different sizes are place in different depth in the tree. The nodes in the tree have already been indexed and each of their offsets within a page can be derived from the index. This array saves the available length of each node such that when traversing the tree from top down, the available length can used to justify which branch we need to go with. When coelescing the buffers, we just check whether the available space in parent node is the same as the sum of the child nodes.

To improve the performance of the buddy system, I use several macros to replace the functions to save time. However, since I use the link list ot save and tranverse the pages, it is still quite slow. The efficiency is pretty good, since buddy system can avoid external fragmentation. And due to the randomization of the input requests, the internal fragmentation is minimized somehow. I have tried different minimum buffer size, and the performance is optimized when it is 64 byte.

### KMA_P2FL ###
```
COMPETITION: running KMA_P2FL on 5.trace
Competition binary successfully completed the trace
./kma_competition: Running in competition mode
Page Requested/Freed/In Use: 10222/10222/    0
Competition average ratio: 0.619608
Test: PASS

Best time (out of 5 runs): .05
Competition score: .080980
```

The p2fl algoithm is implemented using three different headrs. `global_header_t` is used to globally manage the free lists and pages, `free_list_t` is used to save the free lists of each size, and `buffer_header_t` is set in the head of each buffer such that it can be tracked as a link list. The global header and the free list head are placed in a single page such that only when there is no other pages can we free it. Although this has undermine the efficiency somehow, it boost the performance quite well. When we receive a request, a new page is requested if there is no available free buffers, and this page is divided to buffers with same size. This way the address of each buffer within this page can be easily inferred which also benefits our performance.

The p2fl has the best performance among the three algorithms I implemented. Clearly we use a used_space variable to track the used space in each page such that every time we just need to check this variable to see if the page need to be freed and this design doubles the performance of the algorithm. 

## ANALYSIS & SUMMARY ##

Clearly, RM requests the least number of pages since every page it requests can be used to contain any size of memory, which eliminate the external fragmentation. However, since it needs to frequently traverse the free link list, the performance is pretty bad.

Buddy system has the best efficiency because the request trace is pretty randomnized and the internal fragmentation is well controlled. If the request size is always a little bit larger than 2^n, buddy system will have big performance descresing. Also the running time of buddy system is better than RM but still not good because of a design cavact that the page link list is always being traversed when finding the properpage.

P2fl has the best performance since I have greatly eliminate the link list traversing. The good efficiency also comes from the input trace besides its own advantages.
