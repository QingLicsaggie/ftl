#include<stdio.h>
#include"ftlsim.h"
#include"getaddr.h"
void main(){
 
  struct ftl *ftl = ftl_new(2000/(1-0.17) + 4, 2000, 4, 2);

  struct pool *pool = greedy_pool_new(ftl, 4);

  ftl->minfree = 4;

  int i;
  /*for(i = 0; i < 2; i++){
    struct seq *seq = seq_new(); 
    do_ftl_run(ftl, &seq->handle, ftl->U*ftl->Np);
    // get_statistics(ftl);
    }*/

  for(i = 0; i < 200; i++){
    printf("Running:\n\n");
    struct uniform *uniform = uniform_new(ftl->U*ftl->Np);
  
    do_ftl_run(ftl, &uniform->handle, ftl->U*ftl->Np);

    printf("\nExperiment %d\nwrite amplification is %f \n", i + 1, ftl->int_writes/(1.0*ftl->ext_writes));
    
    ftl->int_writes = ftl->ext_writes = 0;

    get_statistics(ftl);
    }
}
