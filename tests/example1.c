#pragma clang diagnostic ignored "-Wunused-label" 
#include <stdio.h>
#include <stdlib.h>
int main(){
   int id;
   int n;
   scanf("%d, %d", &id, &n);
   int s=0, i;
   key_point_1: for (i=0;i<n;i++) { 
       s = s + rand();
   }
   printf("id=%d; sum=%d\n", id, n); 
}
