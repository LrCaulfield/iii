#include "knl/stm32f1xx.h"
#include "systick.h"

void delay(uint32_t ms)
{
    uint32_t ghost = time_ms;
    while (time_ms - ghost < ms);
}
