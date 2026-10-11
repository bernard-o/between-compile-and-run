/* Exercises the formatting paths. Output is byte-identical to glibc (see README
 * for the one documented exception: exact decimal halfway rounding). */
#include <stdio.h>
int main(void){
  printf("int=%d neg=%d u=%u hex=%x/%X oct=%o\n", 42, -7, 300u, -1, 255, 64);
  printf("f=%.3f g=%g e=%.2e w=|%8.2f| z=%05d s=%-6s|\n",
         3.14159, 0.0001234, 123456.0, 2.0/3.0, 42, "hi");
  printf("64-bit: ld=%ld llu=%llu zu=%zu\n", 2147483648L, 18446744073709551615ULL, (unsigned long)123456);
  return 0;
}
