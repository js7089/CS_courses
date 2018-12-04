/*

 * mm.c - Malloc with segregated free lists. Realloc is simply free followed by malloc. * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define MAX(x,y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into word */
#define PACK(size,alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block(payload) ptr, compute address of its header and footer */
#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* for explicit list manipulation */
/* Given block ptr bp, compute address of next and previous blocks <of freelist> */
#define NEXT_FBLKP(bp) (GET(bp))
#define PREV_FBLKP(bp) (GET(bp+WSIZE))

/* Given block ptr bp, compute address of next and previous blocks <locally> */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* pointer heads for free list(sizes tentative) */
int debug=0; // debug = 1 to print all results
void* flist0 = NULL;	// 8 ~ 16 (payload Bytes)
void* flist1 = NULL;	// 17 ~ 64
void* flist2 = NULL;	// 65 ~ 512
void* flist3 = NULL;	// 513 ~ 2048
void* flist4 = NULL;	// 2049 ~ 4096
void* flist5 = NULL;	// 4097 ~ 8192
void* flist_inf = NULL;	// 8193 ~ inf

void putlist(void* ptr);
void dellist(void* ptr);
void insert_in_head(void** flist, void* node);
void* find_from_flist(void* head, size_t size);
void delete(void** flist, void* node);
int fg(size_t size);
void fb();

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // base + WSIZE = first block(huge one sized 4092(4+4088+4Bytes))
    void *p = mem_sbrk(CHUNKSIZE) + WSIZE;	// HEAD ptr
    void *end = (char *)mem_heap_hi() - 7;	// FOOT ptr

    memset(p+WSIZE,'\0',CHUNKSIZE-2*DSIZE);
    PUT(p,PACK(CHUNKSIZE-DSIZE,0));
    PUT(end,PACK(CHUNKSIZE-DSIZE,0));

    flist0 = NULL; flist1=NULL; flist2=NULL; flist3=NULL; flist4=NULL; flist5=NULL;
    flist_inf=NULL;

    // assert whether init() is correct
    
    insert_in_head(&flist4, (char*)p + WSIZE);	// PAYLOAD ptr
    return 0;
}

/* find_from_flist - Retrieve through given free list. 
 * If no match found, this returns NULL.
 * If first-fit is found, return block address.
 * No other OPs. JUST FIND IT!
 */
void* find_from_flist(void *head, size_t size){	
    void *ptr = head;
    if(head == NULL) return NULL;
    if(size+DSIZE <= GET_SIZE((char *)ptr-WSIZE))  return ptr;	// head is returned
    
    while( (void *)NEXT_FBLKP(ptr) != NULL ){	// if NEXT isn't null, retrieve through it
	ptr = (void*) NEXT_FBLKP(ptr);
	if(size+DSIZE <= GET_SIZE((char *)ptr-WSIZE)) return ptr;
    }
    return NULL;
}

// allocate
// addr = payload ptr, size = payload size
void allocate(void *addr, size_t size){
    size_t orig_size = GET_SIZE(HDRP(addr))-DSIZE;
    void* fnext = NEXT_FBLKP(addr);
    void* fprev = PREV_FBLKP(addr);

    int class1 = fg(orig_size);

    if(orig_size==size){
	PUT(HDRP(addr), PACK(size+DSIZE,1));
	PUT(FTRP(addr), PACK(size+DSIZE,1));

	// putlist(addr);
	switch(class1){
	    case 0:
		delete(&flist0,addr);
		break;
	    case 1:
		delete(&flist1,addr);
		break;
	    case 2:
		delete(&flist2,addr);
		break;
	    case 3:
		delete(&flist3,addr);
		break;
	    case 4:
		delete(&flist4,addr);
		break;
	    case 5:
		delete(&flist5,addr);
		break;
	    case 6:
		delete(&flist_inf,addr);
		break;
	    default:
		printf("size error\n");
		break;
	}
	return;
    }else if(orig_size == size+8){
	// Mark allocation states first
	PUT(FTRP(addr), PACK(8,0));
	PUT(FTRP(addr)-WSIZE, PACK(8,0));
	PUT(HDRP(addr),PACK(orig_size,1));
	PUT(FTRP(addr),PACK(orig_size,1));
	// then remove from free list
	switch(class1){
	    case 0:
		delete(&flist0,addr);
		break;
	    case 1:
		delete(&flist1,addr);
		break;
	    case 2:
		delete(&flist2,addr);
		break;
	    case 3:
		delete(&flist3,addr);
		break;
	    case 4:
		delete(&flist4,addr);
		break;
	    case 5:
		delete(&flist5,addr);
		break;
	    case 6:
		delete(&flist_inf,addr);
		break;
	    default:
		printf("size error\n");
		break;
	}
	return;

    } else if(orig_size > size + 8) {
	// split
	PUT(FTRP(addr), PACK(orig_size-size,0));
	PUT(HDRP(addr), PACK(size+DSIZE,1));
	PUT(FTRP(addr), PACK(size+DSIZE,1));
	PUT(NEXT_BLKP(addr)-WSIZE, PACK(orig_size-size,0));
	switch(class1){
	    case 0:
		delete(&flist0,addr);
		break;
	    case 1:
		delete(&flist1,addr);
		break;
	    case 2:
		delete(&flist2,addr);
		break;
	    case 3:
		delete(&flist3,addr);
		break;
	    case 4:
		delete(&flist4,addr);
		break;
	    case 5:
		delete(&flist5,addr);
		break;
	    case 6:
		delete(&flist_inf,addr);
		break;
	    default:
		printf("delete error\n");
		break;
	}
	void *split = (char*) NEXT_BLKP(addr);
	int class = fg( GET_SIZE(split-WSIZE) -DSIZE); // actual PL size
	switch(class){
	    case 0:
		insert_in_head(&flist0,split);
		break;
	    case 1:
		insert_in_head(&flist1,split);
		break;
	    case 2:
		insert_in_head(&flist2,split);
		break;
	    case 3:
		insert_in_head(&flist3,split);
		break;
	    case 4:
		insert_in_head(&flist4,split);
		break;
	    case 5:
		insert_in_head(&flist5,split);
		break;
	    default:
		if((int)GET_SIZE(split-WSIZE)-DSIZE>0)	insert_in_head(&flist_inf,split);
		break;
	}	

}
}

/* For free list manipulation
 * void insert_in_head(void* flist, void* node);
 * void delete(void* flist, void* node);
 * void find_from_flist(void* flist, size_t size);
 */

/*
 * insert_in_head(void* flist, void* node)
 * Insert ptr Node at HEAD of flist
 */
void insert_in_head(void** flist, void* node){	// node is ptr of payload
    // CASE 1. Empty list
    if(*flist == NULL){
	*flist = node;
	PUT(node,(char*)NULL);
	PUT(node+WSIZE,(char*)NULL);
	return;
    } else {	// CASE 2. Not Empty list
	void *head = *flist;			// head
	void *head_next = NEXT_FBLKP(head);	// head->next
	void *head_prev = NULL;//PREV_FBLKP(head);	// head->prev (Possibly NULL)
	
	PUT(node,(char*)head);			// node->next = head
	PUT(node+WSIZE, (char*)head_prev);	// node->prev = head->prev
	
	PUT(head+WSIZE, (char*)node);		// head->prev = node
	*flist = node;
    }

}

/* delete(void* flist, void* node)
 * remove ptr node from freelist
 */
void delete(void** flist, void* node){	// node : payload ptr
    if(*flist == NULL) return;

    void *node_next = NEXT_FBLKP(node);
    void *node_prev = PREV_FBLKP(node);
    if((node_prev==NULL)&&(node_next==NULL)){
	// node is the ONLY element
	*flist = NULL;
	PUT(node,NULL);
	PUT(node+WSIZE,NULL);
	return;
    }

    if(node_prev == NULL){
	// delete from head
	PUT( (char*)node_next + WSIZE, NULL );
	*flist = node_next;
	PUT(node,NULL);
	PUT(node+WSIZE,NULL);
	return;
    }else if(node_next == NULL){
	PUT(node_prev,NULL);	// node->prev = NULL
	PUT(node,NULL);
	PUT((char*) node + WSIZE,NULL);
	return;
    }else{
	PUT(node_prev, node_next);
	PUT(node_next+WSIZE, node_prev);
	PUT(node,NULL);
	PUT((char*)node + WSIZE,NULL);
	return;
    }

}

int fg(size_t size){		// size = PAYLOAD size
    if(size <= 16){			// 1~2DW
	return 0;
    }else if(size>16 && size <=64){
	return 1;
    }else if(size>65 && size <= 512){
	return 2;
    }else if(size>512 && size <= 2048){
	return 3;
    }else if(size>2048 && size <= 4096){
	return 4;
    }else if(size>4096 && size <= 8192){
	return 5;
    }else{
	return 6;
    }
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if(size <= 0) return NULL;		// ignore this case

    size_t blk_size = ALIGN(size);
    void* tmp;
    
    int fgroup = fg(blk_size);

    alloc:
    switch(fgroup){
	case 0:
	    if( (tmp = find_from_flist(flist0, blk_size)) != NULL){
		allocate(tmp,blk_size);
		return tmp;
	    }
	    // find from flist0
	case 1:
	    // ... flist1
	    if( (tmp=find_from_flist(flist1, blk_size)) != NULL){
		allocate(tmp,blk_size);		
		return tmp;
	    }
	case 2:
	    // ... flist2
	    if( (tmp=find_from_flist(flist2,blk_size)) != NULL){
		allocate(tmp,blk_size);
		return tmp;
	    }
	case 3:
	    // ... flist3
	    if( (tmp=find_from_flist(flist3,blk_size)) != NULL){
		allocate((char*)tmp, blk_size);
		return tmp;
	    }
	case 4:
	    if( (tmp = find_from_flist(flist4,blk_size)) != NULL){
		allocate((char*)tmp,blk_size);
		return tmp;
	    }
	    // ... flist4
	case 5:
	    if( (tmp=find_from_flist(flist5,blk_size)) != NULL){
		allocate(tmp,blk_size);
		return tmp;
	    }
	    // ... flist5
	case 6:
	    if( (tmp=find_from_flist(flist_inf,blk_size)) != NULL){
		allocate(tmp,blk_size);
		return tmp;
	    }
	    // ... break;
	default:		// THIS PART!!
	    printf("");
	    // extend heap by CHUNKSIZE until there's enough space
	    // Get the rightmost free block size
	    size_t rblocksize = GET_SIZE(mem_heap_hi()+1-DSIZE);
	    if( GET_ALLOC(mem_heap_hi()+1-DSIZE) == 1 ){
		rblocksize = 0;
	    }
	    void* rblock = mem_heap_hi()+1-WSIZE;
	    int classR = fg(rblocksize-DSIZE);
	    if((GET_ALLOC(mem_heap_hi()+1-DSIZE) == 0)){
	    if(rblocksize-DSIZE>0){
		dellist(rblock-rblocksize+WSIZE);
	    }
	    size_t extended_heap = 0;
	    while( rblocksize+extended_heap < blk_size+DSIZE ){
		void* base_;
		if((base_=mem_sbrk(CHUNKSIZE))==NULL){
			printf("Failed to sbrk()\n");
			return NULL;
		}
		extended_heap += CHUNKSIZE;
	    }
	    PUT(mem_heap_hi()+1-DSIZE, PACK(rblocksize+extended_heap,0));
	    PUT(mem_heap_hi()+1-WSIZE-rblocksize-extended_heap, PACK(rblocksize+extended_heap,0));
	    void* rmost_block = mem_heap_hi()+1-rblocksize-extended_heap;

	    // Extended heap fragment into free list
	    // Mark it as free
	    // and find a suitable flist
	    int class = fg(rblocksize+extended_heap-DSIZE);
	    switch(class){
		case 4:
		    insert_in_head(&flist4, rmost_block);
		    break;
		case 5:
		    insert_in_head(&flist5, rmost_block);
		    break;
		case 6:
		    insert_in_head(&flist_inf, rmost_block);
		    break;
		default:
		    printf("ext_heap = %d\n",extended_heap);
		    printf("rblocksize+ext_heap-DSIZE = %d\n",rblocksize+extended_heap-DSIZE);
		    printf("insert error\n");
		    break;
	    }
	    goto alloc;

	    }else{	// rightmost block is allocated
		size_t extended_heap = 0;
		while( extended_heap < blk_size + DSIZE ){
		    mem_sbrk(CHUNKSIZE);
		    extended_heap += CHUNKSIZE;
		}
		PUT(rblock, PACK(extended_heap,0));
		PUT(mem_heap_hi()+1-DSIZE, PACK(extended_heap,0));
		int class = fg(extended_heap-DSIZE);
		switch(class){
		    case 4:
			insert_in_head(&flist4, rblock+WSIZE);
			break;
		    case 5:
			insert_in_head(&flist5, rblock+WSIZE);
			break;
		    case 6:
			insert_in_head(&flist_inf, rblock+WSIZE);
			break;
		    default:
			printf("Extended_heap - DSIZE = %d\n",extended_heap-DSIZE);
			break;
		}
		goto alloc;
	    }
	    break;
	    // if no block found from freelist, go for mem_alloc_top()
	    // which is MAX(addr_of_footer)+4 of allocated block
	    // Extend the heap by blk_size(requested)
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    // NO COALESCING VERSION
    size_t bsize = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr),PACK(bsize,0));
    PUT(FTRP(ptr),PACK(bsize,0));

    int fblock = (ptr == mem_heap_lo() + DSIZE);
    int lblock = (FTRP(ptr) == mem_heap_hi()+1-DSIZE);

    void* prev_node = PREV_BLKP(ptr);
    void* next_node = NEXT_BLKP(ptr);
    size_t prev_size = GET_SIZE( HDRP(PREV_BLKP(ptr)));
    size_t next_size = GET_SIZE( HDRP(NEXT_BLKP(ptr)));
    int prev_alloc = GET_ALLOC( HDRP(PREV_BLKP(ptr)));
    int next_alloc = GET_ALLOC( HDRP(NEXT_BLKP(ptr)));
    void* leftmost = ptr;
    size_t total = bsize;	// total size

    if( (!lblock) && (!fblock)){
    if( (prev_size>0)&&(!prev_alloc) ){
	// Coalesce with previous block
	// remove prev_block from freelist it belongs to
	if( 16 <= prev_size ){
	    int bclass = fg(prev_size-DSIZE);
	    switch(bclass){
		case 0:
		    delete(&flist0,prev_node);
		    break;
		case 1:
		    delete(&flist1,prev_node);
		    break;
		case 2:
		    delete(&flist2,prev_node);
		    break;
		case 3:
		    delete(&flist3,prev_node);
		    break;
		case 4:
		    delete(&flist4,prev_node);
		    break;
		case 5:
		    delete(&flist5,prev_node);
		    break;
		case 6:
		    delete(&flist_inf,prev_node);
		    break;
	    }
	}
	// and merge
	PUT(HDRP(prev_node),PACK(prev_size+bsize,0));
	PUT(FTRP(ptr),PACK(prev_size+bsize,0));
	total += prev_size;
	leftmost = prev_node;
    }
    if( (next_size>0)&&(!next_alloc) ){
	// Coalesce with next block
	// remove prev_block from freelist it belongs in
	if( 16 <= next_size ){
	    int c_class = fg(next_size-DSIZE);
	    switch(c_class){
		case 0:
		    delete(&flist0,next_node);
		    break;
		case 1:
		    delete(&flist1,next_node);
		    break;
		case 2:
		    delete(&flist2,next_node);
		    break;
		case 3:
		    delete(&flist3,next_node);
		    break;
		case 4:
		    delete(&flist4,next_node);
		    break;
		case 5:
		    delete(&flist5,next_node);
		    break;
		case 6:
		    delete(&flist_inf,next_node);
		    break;
	    }
	}
	// and merge
	size_t lsize = GET_SIZE(HDRP(leftmost));
	PUT(HDRP(leftmost),PACK(lsize+next_size,0));
	PUT(FTRP(leftmost),PACK(lsize+next_size,0));
	total += next_size;
    }
	putlist(leftmost);
	}else{
	    if(lblock && fblock){
		//printf("PUT WHOLE HEAP IN LIST\n");
		size_t siz = mem_heap_hi()-mem_heap_lo()-7;
		PUT(mem_heap_lo()+4,PACK(siz,0));
		PUT(mem_heap_hi()-7,PACK(siz,0));
		flist0=NULL; flist1=NULL; flist2=NULL; flist3=NULL; flist4=NULL; flist5=NULL; flist_inf=NULL;
		putlist(mem_heap_lo()+4);
		return;
	    }
	    if(fblock){
		if(!next_alloc){
		    dellist(next_node);
		    PUT(HDRP(ptr),PACK(bsize+next_size,0));
		    PUT(FTRP(ptr),PACK(bsize+next_size,0));
		    putlist(ptr);
		}else{
		    PUT(HDRP(ptr),PACK(bsize,0));
		    PUT(FTRP(ptr),PACK(bsize,0));
		    putlist(ptr);
		}
		return;
	    }
	    if(lblock){
		if(!prev_alloc){
		    dellist(prev_node);
		    PUT(HDRP(prev_node),PACK(bsize+prev_size,0));
		    PUT(FTRP(prev_node),PACK(bsize+prev_size,0));
		    putlist(prev_node);
		}else{
		    PUT(HDRP(ptr),PACK(bsize,0));
		    PUT(FTRP(ptr),PACK(bsize,0));
		    putlist(ptr);
		}
	    }
	}

}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    if (size == 0){
	mm_free(ptr);
	return NULL;
    }
    copySize = GET_SIZE(HDRP(ptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

// helper function
void putlist(void* ptr){
// put ptr(payload) to appropriate list
    size_t size = GET_SIZE(HDRP(ptr));
    int class = fg(size-DSIZE);
    switch(class){
	case 0:
	    insert_in_head(&flist0,ptr);
	    break;
	case 1:
	    insert_in_head(&flist1,ptr);
	    break;
	case 2:
	    insert_in_head(&flist2,ptr);
	    break;
	case 3:
	    insert_in_head(&flist3,ptr);
	    break;
	case 4:
	    insert_in_head(&flist4,ptr);
	    break;
	case 5:
	    insert_in_head(&flist5,ptr);
	    break;
	case 6:
	    insert_in_head(&flist_inf,ptr);
	    break;
    }
}

void dellist(void* ptr){
    size_t size = GET_SIZE(HDRP(ptr));
    int class = fg(size-DSIZE);
    switch(class){
	case 0:
	    delete(&flist0,ptr);
	    break;
	case 1:
	    delete(&flist1,ptr);
	    break;
	case 2:
	    delete(&flist2,ptr);
	    break;
	case 3:
	    delete(&flist3,ptr);
	    break;
	case 4:
	    delete(&flist4,ptr);
	    break;
	case 5:
	    delete(&flist5,ptr);
	    break;
	case 6:
	    delete(&flist_inf,ptr);
	    break;
    }
}

