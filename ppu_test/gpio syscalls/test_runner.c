
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

    printf("Starting / Running gpio tests\n");

    uint64_t device_id = 0;
    volatile uint64_t * fail = (uint64_t*)failedBuf + 8;
    while (device_id < 100000){
    int cell = sys_gpio_get(device_id, fail);
    if (cell == 0) printf("device_id = 0x%x data = 0x%x", device_id ,*fail);
    device_id++;
    }

    free(failedBuf);
    return 0;
}
