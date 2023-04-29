#pragma clang diagnostic ignored "-Wunused-label"
#include <stdio.h>
#include <stdlib.h>
int main(){
    char str1[1000];
    FILE * fp = fopen ("file.txt", "r");
    char c;
    int len=0;
    while (1){
        c=getc(fp); 
        key_point_1: if (c == EOF) break;
        str1[len++] = c;
        key_point_2: if (len>=1000) break; 
    }
    //printf("%s\n", str1);
}
