
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define __CELL_ASSERT__
#include <assert.h>
#include <sys/syscall.h>
#include <sys/gpio.h>
#include <cell.h>

//extern int test(int zero, void *scratch, void *failures, double one);

int main(void)
{
    char *failedBuf = malloc(65536);


    uint64_t device_id = 0;

    int cell = sys_gpio_set(SYS_GPIO_DIP_SWITCH_DEVICE_ID, 0xf , 1);
    printf("cell = 0x%x data = 0x%x", cell);


    free(failedBuf);
    return 0;
}
