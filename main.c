
#include "mymalloc.h"



int main() {

 int *q = mymalloc(30*sizeof(int));
 q[0]=45;
 printf("%d\n",q[0]);
 myfree(q);

   
   return 0;
}