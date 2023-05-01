#pragma clang diagnostic ignored "-Wunused-label" 
#include <stdio.h>
#include <stdlib.h> 

int main(int argc, char **argv) 
{
   int n, j, total;

   scanf("%d, %d", &n, &j); // Take in square matrix size and init value
   total = n*n; // total elements

   int* arr = (int*) malloc(total * sizeof(int));
   int i;
   key_point: for (i=0; i<total; i++) 
   {
		arr[i] = j;
		printf("arr[%d] = %2d\n", i, arr[i]);
   }

}