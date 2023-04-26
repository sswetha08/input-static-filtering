#include <stdio.h>
#include <stdlib.h>
int main(){
   int id;
   int n;
   scanf("%d, %d", &id, &n);
   FILE * fp = fopen ("file.txt", "r");
   //int s = 0;
   char c;
   for (int i=0;i<n;i++){
      // s += rand();
      c = getc(fp);
   }
   printf("id=%d; sum=%d\n", id, n); 
}
