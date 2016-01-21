#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "getaddr.h"
#include "ftlsim.h"
/*some double link-list utility function*/

/* insert b into the tail of list*/
 void list_add(struct block *b, struct block *list)
 {
    struct block *iter = list;
    while(iter->next != NULL)
      iter = iter->next;
    
    
    iter->next = b;
    b->prev = iter;
    b->next = NULL;
}

/* delete block b from the list*/
 void list_rm(struct block *b)
{
    b->prev->next = b->next;
    if(b->next)
      b->next->prev = b->prev;
 }

/* pop out list from the link list*/
struct block *list_pop(struct block *list)
{
    struct block *b = list->next;
    list_rm(b);
    return b;
}

/* ----------------------------------------------
 *
 * @brief:      constructor function for segment
 *  
 * @param[in]   Np -- number of pages in a block.
 *  
 * @param[in]   t  -- number of rewrites supported by WOM code 
 *
 * LAST TIME UPDATE: 10/04/2015
 *---------------------------------------------
*/
struct block *block_new(int Np, int t, int num)
{
    int i;
    struct block *blk = (struct block*) malloc(sizeof(struct block));
    blk->number       = num;  //to indicate whether this block is deleted or not.
    blk->Np           = Np;
    blk->t            = t;
    blk->lba          = malloc(Np * sizeof(int));
    blk->rewrites     = malloc(Np*sizeof(int));
    
    for (i = 0; i < Np; i++){
        blk->lba[i] = -1;
        blk->rewrites[i] = 0;
    }
    blk->next = blk->prev = NULL;
    return blk;
}
/*------------------------------------------------------ 
 *
 * @brief:  deconstructor function for block by marking 
 *          the magic number to zero. 
 *
 *  
 ----------------------------------------------------          
*/
void block_del(struct block *block)
{
    block->number = -1;
    free(block);
}


/*---------------------------------------------------
 * 
 * @brief:  A small utility function to do statistics for a block.
 *
------------------------------------------------------*/
void print_block(struct block *seg){
  int i = 0;
  for(i = 0; i < seg->Np; i++)
    printf("[%d %d] --> LBA %d \n", seg->number, i, seg->lba[i]);
}

/*-----------------------------------------------------------------
 *
 * @brief         For a write on a logical block address lba, which is
 *                mapped to physical block address page, deal with the
 *                case when page can not accormodate this write. More
 *                detailedly, self->rewrites[page] must be t  as it can
 *                not support another writes.   We mark the logical block
 *                address of page as -1, meaning this page is invalid 
 *                now and * set rewrites[page] to t + 1. We have to update
 *                some information, e.g., valid # and invalid #.
 *
 * @param[in]   
 *                 self -- physical block self.
 *                 page -- physical block address
 *                 lba  -- logical block address
 * 
 * @output         update mapping table, rewrite times and some information
 * 
 *LAST TIME UPDATE: 10/13/2015
 *
 *-------------------------------------------------------
*/
void do_block_overwrite(struct block *self, int page, int lba)
{
    assert(page < self->Np && page >= 0 && self->lba[page] == lba && self->rewrites[page] == self->t);
    /*mark this address as invalid now*/
    self->lba[page] = -1;    
    /*for this block, valid number of pages decreases by one.*/
    self->n_valid--;          
    /*the number of rewrite times for this page is t + 1 now, indicating it is invalid.*/
    self->rewrites[page] = self->t + 1; 
    
    if (self->pool && self->in_pool && self != self->pool->frontier) {
        self->pool->pages_valid--; 
        self->pool->pages_invalid++;


        if (self->pool->bins) {
    	    list_rm(self);   
            list_add(self, self->pool->bins[self->n_valid]);
            if (self->n_valid < self->pool->min_valid)
                self->pool->min_valid = self->n_valid;
        }
    }
    //  printf("LBA %d is marked invalid at PBA %d with block number %d\n", lba, page, self->number);
}

/*-----------------------------------------------------------------------------
 *
 * @brief:     for a logical write on logical block address lba, write it to 
 *             the block self with page number page.
 * @param[in]:
 *             self -- block
 *             page -- physical block address of a page
 *             lba  -- logical block address
 * 
 * @ouput      OUTPUT:  If this is the first write, update the mapping table, valid page numbers and number of rewrites
 *
 * LAST TIME UPDATE: 10/04/2015
 *
 *-------------------------------------------------------------------------             
*/
void do_block_write(struct block *self, int page, int lba)
{
    /*If the page is the first time to write, n_valid ++*/
    if(self->lba[page]== -1) self->n_valid++;
    /*page is mapped to lba*/
    self->lba[page] = lba; 
    /*increase the number of rewrites*/
    self->rewrites[page]++;                  
    /*we have to record the block as well as page info.*/
    self->pool->ftl->map[lba].blk = self; 
    self->pool->ftl->map[lba].page_num = page; 
    //printf("LBA %d is written to PBA %d with block %d \n", lba, page, self->number);
}
/*-------------------------------------------------------------------------------------
 *
 *@brief:            another garbage collection aglorithm for rewriting code, i.e.,
 *                   does not cange rewrite times.
 *------------------------------------------------------------------------------------
 */
void do_block_writeM(struct block *self, int page, int lba)
{
    /*page is mapped to lba*/
    self->lba[page] = lba;                   
    struct block *blk = self->pool->ftl->map[lba].blk;
    /*not change for simplicity.*/
    self->rewrites[page] = blk->rewrites[self->pool->ftl->map[lba].page_num]; 
    /*we have to record the block as well as page info.*/
    self->pool->ftl->map[lba].blk = self; 
    self->pool->ftl->map[lba].page_num = page;
    //printf("LBA :%d is written to PBA %d, and rewrite time is %d with block number %d  \n", lba, page, self->rewrites[page], self->number );
}
/*------------------------------------------------------------------
 *
 *  @brief:    another garbage collection aglorithm for rewriting code, 
 *             i.e., does not change rewrite times.
 *--------------------------------------------------------------------*/

void gc_wom_copy(struct block *self, int page, int lba)
{
    /* page is mapped to lba*/
    self->lba[page] = lba;                  
    struct block *blk = self->pool->ftl->map[lba].blk;
    /* not change for simplicity*/
    self->rewrites[page] = blk->rewrites[self->pool->ftl->map[lba].page_num];
    /*we have to record the block as well as page info*/
    self->pool->ftl->map[lba].blk = self; 
    self->pool->ftl->map[lba].page_num = page;
    //printf("LBA :%d is written to PBA %d, and rewrite time is %d  \n", lba, page, self->rewrites[page] );
}

/*-------------------------------------
 *
 * @brief: construction function of ftl.
 *
 *------------------------------------*/
struct ftl *ftl_new(int T, int U, int Np, int t)
{
    struct ftl *ftl = (struct ftl *)malloc(sizeof(struct ftl));
    ftl->T          = T;
    ftl->U          = U;
    ftl->Np         = Np;
    ftl->t          = t;
    ftl->map        = malloc(sizeof(struct map)*T*Np);
    ftl->int_writes = ftl->ext_writes = ftl->write_seq = 0;

    int i;
    for(i = 0; i < T; i++){
      struct block *blk = block_new(Np, t, i);
      do_put_blk(ftl, blk);
    }
    return ftl;
}

/*----------------------------------------------
 *
 * @brief: put blk at the head of ftl's free linked-list.
 *
 * ---------------------------------------------*/
void do_put_blk(struct ftl *self, struct block *blk)
{
    blk->next = self->free_list;
    self->free_list = blk;
    self->nfree++;
}

/*---------------------------------------------------------------
 *
 * @brief: remove one block from the ftl's free linked-list.
 *
-----------------------------------------------------------*/
struct block *do_get_blk(struct ftl *self)
{
    struct block *val = self->free_list;
    self->free_list = val->next;
    self->nfree--;
    return val;
}

/* -------------------------------------------------
 *
 * @ brief: deconstructor function of ftl.
 *
 *--------------------------------------------------*/
void ftl_del(struct ftl *ftl)
{
    struct block *b;

    while ((b = do_get_blk(ftl)) != NULL)
        block_del(b);
    free(ftl->map);
    free(ftl);
}
/* ------------------------------------------------------------
 *
 * @brif: construction of pool where the garbage collection is specified as greedy. 
 *
 *  LAST TIME UPDATED: 10/10/2015.
 *
---------------------------------------------------------------*/
struct pool *greedy_pool_new(struct ftl *ftl, int Np)
{
    struct pool *pool = (struct pool *)malloc(sizeof(struct pool));
    pool->ftl = ftl;
    pool->Np = Np;

    ftl->pools = pool;
    int i;
    for( i = 0; i <= Np; i++)
      pool->bins[i] = block_new(Np, ftl->t, -1);
   
    pool->min_valid = Np;
   
    struct block *iter = ftl->free_list;
    
    while(iter != NULL){
      iter->pool = pool;
      iter = iter->next;
    }
    
    pool->frontier = do_get_blk(ftl);
    pool->int_writes = pool->ext_writes = pool->current_page_num = 0;
    pool->length = 1;

    return pool;
}

/*---------------------------------------------------------
 *
 * @brief:  print out GC candidates.
 *
 *---------------------------------------------------------*/
void print_gc_candidates(struct pool *mypool){
  printf("GC Candidate \n");
     
     int i;
     for(i = 0; i <= mypool->Np; i++){
       if(mypool->bins[i]->next != NULL){
	 printf("\nlist %d: \n", i);
	 struct block *iter;
	 iter = mypool->bins[i]->next;
	 while(iter != NULL){
	   print_block(iter);
	   printf("\n");
	   iter = iter->next;
	 }
       }
       else
	 printf("list %d is empty \n", i);
     }
}


/*---------------------------------------
 *
 *  @brief:    print out the map table.
 *
 *  @param[in]: ftl
 *
-------------------------- ---------------*/
void print_map(struct ftl *ftl){ 
  int i;
  printf("\nftl mapping table\n");
  printf("(LBA, rewrite) --> (Block#, page#)\n");
  for(i = 0; i < ftl->U*ftl->Np; i++){
    struct block *b = ftl->map[i].blk;
    if(b){
      int pba = ftl->map[i].page_num;
      printf("(%d, %d) ---> (%d, %d) \n", i, b->rewrites[pba],b->number, pba );
    }
  }
}

/*--------------------------------------------------------------
* 
* @brief:    the original pool can not accommdate another new write,
*            thus we need to offer another frontier.
*
* @para:     pool and the new write frontier, block
*
*LAST TIME UPDATED: 10/06/2015
--------------------------------------*/ 
void greedy_pool_addseg(struct pool *pool, struct block *block)
{
    pool->length++;
    pool->current_page_num = 0;
    struct block *blk = pool->frontier;
    
   
    int i = 0, count = 0;
    for(i = 0; i < blk->Np; i++)
      if(blk->lba[i] != -1)
	count ++;
    blk->n_valid = count;

    pool->frontier->in_pool = 1; 
    pool->pages_valid += pool->frontier->n_valid;
    pool->pages_invalid += (pool->Np - pool->frontier->n_valid);


    list_add(blk, pool->bins[blk->n_valid]);
    if (blk->n_valid < pool->min_valid)
            pool->min_valid = blk->n_valid;

    pool->frontier = block;
    block->pool = pool;
}


/*-----------------------------------------------------------
 *
 * @brief:    Make sure the next page to write in frontier block
 *            is available (pool->i <= pool->Np). Otherwise, we 
 *            get a free segment from the free_list (do_get_blk),
 *            and add it to the pool(pool->addseg(pool, b))
 *
  -----------------------------------------------------------*/
void check_new_block(struct pool *pool)
{
  if (pool->current_page_num == pool->Np) {
        struct block *b = do_get_blk(pool->ftl);
        greedy_pool_addseg(pool, b);
    }
}

/*---------------------------------------------------------------------------
 *
 *@brief:   this is the for non-rewriting code case corresponding to the previous function.
 *
 *LAST TIME UPDATE 10/06/2015 
 *
--------------------------------------------------------------------------------*/
void writeOnce(struct ftl *ftl, struct pool *pool, int lba)
{
    ftl->int_writes++;
    
    while(pool->frontier->rewrites[pool->current_page_num] != 0){
 	  pool->current_page_num++;
	  check_new_block(pool);
    }
    do_block_writeM(pool->frontier, pool->current_page_num++, lba);
}

/*------------------------------------------------ 
 *
 * @brief:  given flash translation layer, ftl, and the rewriting pool, pool, 
 *          and logical-block-address, lba, we deal with the rewriting case. 
 *          Note that this is rewriting codes, thus three cases may happen, 
 *          one is rewriting, the other is the first time writing and last 
 *          one is the invalid. We have to concern those cases.
 * 
 * LAST TIME UPDATED: 10/05/2015.
--------------------------------------------- */
void rewrite(struct ftl *ftl, struct pool *pool, int lba)
{
    ftl->int_writes++;
    struct block *b = ftl->map[lba].blk;

    int page = ftl->map[lba].page_num;
    if (b != NULL && page >= 0 && page < b->Np &&b->rewrites[page] == b->t) {
      // printf("Case 1: Impossible to rewrite, we have mark it as invalid and write somewhere. \n ");
        do_block_overwrite(b, page, lba);
	while(pool->frontier->rewrites[pool->current_page_num] != 0){
	  pool->current_page_num++;
	  check_new_block(pool);
	}
	do_block_write(pool->frontier, pool->current_page_num++, lba);
    }
    else if(b != NULL && page >=0 && page < b->Np){
      //printf("Case 2: we can still write to this page, i.e., in this case it is rewriting.\n ");
      do_block_write(b, page, lba);
    }
    else {
      //printf("Case 3: first time rewrite\n");
      do_block_write(pool->frontier, pool->current_page_num++, lba);
    }
}


/*----------------------------------------------------------------
 *
 * @brief:     garbage collection process. the caller of this function
 *             is responsible for copying the remaining valid pages
 *
 * LAST TIME UPDATE: 10/10/2015.
---------------------------------------------------------------*/
struct block *greedy_pool_getseg(struct pool *pool)
{	
    int i = 0;
    for (i = 0; i < pool->Np; i++) 
      if (pool->bins[i]->next != NULL)
            break;
    pool->min_valid = i;

    struct block *b = list_pop(pool->bins[pool->min_valid]);
 
    /*indicate that the block is out of this pool now.*/
    b->in_pool = 0;
    
    /*update some meta information of this pool*/
    pool->pages_valid -= b->n_valid;
    pool->pages_invalid -= (pool->Np - b->n_valid);
    b->pool = NULL;

    
   
    for (i = 0; i < pool->Np; i++) 
      if (pool->bins[i]->next != NULL)
            break;
    pool->min_valid = i;
    pool->length--;

    return b;
}

/*--------------------------------------------------
 *
 *@brief:  obtain statistics
 *
  ----------------------------------------------------*/
void get_statistics(struct ftl *ftl){

  int statis[65][65];
  int i, j;

  for(i = 0; i <= ftl->Np; i++)
    for(j = 0; j <= ftl->Np; j++)
      statis[i][j] = 0;

  int bitMap[ftl->T];
  for(i = 0; i < ftl->T; i++)
    bitMap[i] = 0;

  int sum = 0;
  for(i = 0; i < ftl->U*ftl->Np; i++){
    struct block *free_list = ftl->map[i].blk;
    
    if(free_list == NULL) continue;
    if(bitMap[free_list->number] == 0){
      
      int t1 = 0, t2 = 0;
      for(j = 0; j < ftl->Np; j++){
	if(free_list->rewrites[j] == 1)
	  t1++;
	else if(free_list->rewrites[j] == 2)
	t2++;
      }
      statis[t1][t2]++;
      sum++;
      bitMap[free_list->number] = 1;
    }
  }
  printf("The statistics is as follows: \n");

  
  for(i = 0; i <= ftl->Np; i++)
     for(j = 0; j <= ftl->Np; j++){
      if(statis[i][j] !=0){
	printf("statis[ %d ][%d] is %f \n ", i, j, statis[i][j] /(1.0*sum));
      }
   }
}

/*--------------------------------------------------------
 *
 *@brief:          driver function for running experiment.
 *
 *@para:           addrs is the address generator, uniform, random, etc.
 *
 * LAST TIME UPDATE: 10/13/2015.
 *
 ----------------------------------------- ----------------*/
void do_ftl_run(struct ftl *ftl, struct getaddr *addrs, int count)
{
     int i, j;
   
     struct pool *pool = NULL;

     for (i = 0; i < count; i++) {
       //printf("\n %d-th experiment\n", i + 1);
        int lba = addrs->getaddr(addrs);
 	//printf("LBA %d\n ", lba);
    	pool = ftl->pools;

        ftl->ext_writes++;
        ftl->write_seq++;

    	/*write it to the pool*/
        rewrite(ftl, pool, lba);
    	if (pool->current_page_num == pool->Np) {
    	  struct block *b = do_get_blk(ftl);
    	  greedy_pool_addseg(pool, b);
    	}
	//printf("\nbefore GC\n");
	//print_map(ftl);
	//get_statistics(ftl);
	while (ftl->nfree < ftl->minfree) {
    	      struct block *b = greedy_pool_getseg(pool);
	      // printf("GC now and we choose the block with %d\n", b->number);
               for (j = 0; j < b->Np; j++){
	          if (b->rewrites[j] != b->t + 1) {
		    writeOnce(ftl, pool, b->lba[j]);
		    check_new_block(pool);
		  }
    		  /*garbage collection*/
    		  b->rewrites[j] = 0;
    	 	  b->lba[j] = -1;
	       }
    	    b->n_valid = 0;

            do_put_blk(ftl, b);
	    
	    /*printf("\nAfter GC\n");
	    print_map(ftl);
	    get_statistics(ftl);*/
	}//end gc
     }
}


