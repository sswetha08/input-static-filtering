#include <stdio.h>
int main() {
   FILE *f = fopen("new.txt", "r");
   int ch = getc(f);
   while (ch != EOF) {
      putchar(ch);
      ch = getc(f);
   }
   fclose(f);
   getchar();
   return 0;
}