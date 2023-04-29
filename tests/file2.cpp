#pragma clang diagnostic ignored "-Wunused-label"
#include <stdio.h>
#include <stdlib.h>
int main(){
   int id;
   int n;
   scanf("%d, %d", &id, &n);
   int s=0,v;
   v = n*10;
   key_point_1: for (int i=0;i<v;i++){ 
       s = s + rand();
   }
   printf("id=%d; sum=%d\n", id, n); 
}
