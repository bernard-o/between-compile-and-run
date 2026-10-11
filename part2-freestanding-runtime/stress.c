#include <stdio.h>
int main(void){
  printf("int=%d neg=%d hex=%x f=%.3f g=%g w=|%8.2f| z=%05d s=%-6s|\n",
         42,-7,255,3.14159,0.0001234,2.5,42,"hi");
  printf("long=%ld big=%llu\n", 1234567890L, 18446744073709551615ULL);
  return 0;
}
