#pragma clang diagnostic ignored "-Wunused-label" 
#include <stdio.h>
int arr[20][20];
int main(int argc, char **argv) 
{
   int n, j, k;
   int *arr2 = (int*)arr;

   scanf("%d, %d", &n, &j); // Take in square matrix size and init value
   k = n*n; // total elements

   int i;
   key_point: for (i=0; i<k; i++) 
   {
		arr2[k] = j;
		printf("arr[%d] = %2d\n", i, arr2[i]);
   }

}