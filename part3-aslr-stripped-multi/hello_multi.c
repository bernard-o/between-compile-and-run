#include <stdio.h>
int main(void){
    puts("opening (via puts)");
    printf("processed %d files in %.1f s\n", 3, 12.5);
    puts("closing (via puts)");
    return 0;
}
