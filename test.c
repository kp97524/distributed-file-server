/******************************************************************************

                            Online C Compiler.
                Code, Compile, Run and Debug C program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/

#include <stdio.h>
#include<stdlib.h>

void bin(unsigned int n)
{
    if (n > 1)
        bin(n >> 1);
 
    printf("%d", n & 1);
}

void set_bit_zero(int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
  unsigned int x =  bitmap[index] & ~(1 << (offset -1));
}
int main()
{
 set_bit(5);

    return 0;
}
