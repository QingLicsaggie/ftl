/*----------------------------------------
**
*
* @brief     FTL
**
** @author    Qing Li(qingklee@gmail.com).
**
-------------------------------------*/
struct block {
    int number;                     
    struct block *next, *prev;
    int  Np;                         /*Number of pages in a block*/
    int  t;                          /*Number of rewrites can be experienced*/
    int *lba;                        /* lba[0..Np-1] = LBA, lba[i] means the logical-block-address of physical i-th page */
    int *rewrites;                    /*recodes how many times a specific page has been experiencing rewrites, i.e.,rewrites[i] (\in  \{0, 1, 2, ..., t\}) means the for the i-th page, how many rewrites it has been */ 
    int  in_pool;                     /* false if we're still the write frontier */
    int  n_valid;                     /* the number of valid pages for this block*/
    struct pool *pool;
};

/*constructor function of segment*/
struct block *block_new(int Np, int t, int num);

/*deconstructor function of segment*/
void block_del(struct block *b);

/*for a write at logical block address, lba, write it to the physical block address of a page, page, on a flash memory block self*/
void do_block_write(struct block *self, int page, int lba);


/*during gc, copy lba to page but do not change the rewrite times*/
void gc_wom_copy(struct block *self, int page, int lba);

/*for a overwrite operation at a logical block address, lba, mark it as invalid*/
void do_block_overwrite(struct block *self, int page, int lba);


struct map {
  struct block *blk;
  int    page_num;
};  //mapping table of this ftl.
 
struct ftl {
    struct block *free_list;
    struct map *map;
    int T, Np, U,  t;
    int int_writes, ext_writes, write_seq;
    int nfree, minfree;
    int npools;
  struct pool *pools;
};


//constructor function of ftl
struct ftl *ftl_new(int T, int U, int Np, int t);

//deconstructor function of ftl
void ftl_del(struct ftl*);

//put blk at the head of ftl's free linked-list.
void do_put_blk(struct ftl *self, struct block *blk);

//remove one block from the ftl's free linked-list.
struct block *do_get_blk(struct ftl *self);

//print the map table.
void print_map(struct ftl*ftl);


//void do_ftl_run(struct ftl *ftl, struct getaddr *addrs, int count);



struct pool {
    struct ftl *ftl;
    struct block *frontier;
    int Np, int_writes, ext_writes, current_page_num;
    int pages_valid, pages_invalid, length;
    struct block *bins[65]; /* for greedy - [i] has 'i' valid pages */
    int min_valid;
};

void greedy_pool_addseg(struct pool *self, struct block *blk);
struct block* greedy_pool_getseg(struct pool *self);
void  rewrite(struct ftl*, struct pool*, int);
void  del(struct pool*);


/*two utility functions*/
void print_block(struct block *seg);
void print_gc_candidates(struct pool *mypool);

void writeOnce(struct ftl*, struct pool*, int Np);
struct pool *greedy_pool_new(struct ftl *, int Np);


void check_new_block(struct pool *pool);
