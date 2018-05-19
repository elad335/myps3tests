
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define __CELL_ASSERT__
#include <assert.h>
#include <sys/syscall.h>
#include <sys/gpio.h>
#include <cell.h>

//extern int test(int zero, void *scratch, void *failures, double one);
inline void printcell(int cell)
{
     switch (cell)
     {
         case 0: {printf("cell is OK\n");  break;}
         default: printf("cell is %x", cell);
     }
}
int main(void)
{
    char *failedBuf = malloc(65536);


    uint64_t device_id = 0;

    int cell = sys_gpio_get(SYS_GPIO_DIP_SWITCH_DEVICE_ID, &cell);
   printcell(cell);

    free(failedBuf);
    return 0;
}
