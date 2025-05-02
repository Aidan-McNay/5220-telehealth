// =======================================================================
// LED.cpp
// =======================================================================
// Implementation of LED controls

#include "ui/LED_hw.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "utils/debug.h"


// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

LED_hw::LED_hw(int gpio_num) : gpio_num(gpio_num)
{
    debug("LED start\n");
    stdio_init_all();
    debug("LED GPIO %d\n", gpio_num);

    debug("LED middle\n");

    gpio_init(gpio_num);
    gpio_set_dir(gpio_num, GPIO_OUT);
    gpio_put(gpio_num, 0); // Start with LED off
    debug("LED end\n");
}

// -----------------------------------------------------------------------
//  Helper Functions
// -----------------------------------------------------------------------

void LED_hw::on()
{
    gpio_put(gpio_num, 1); // Turn on the LED
}

void LED_hw::off()
{
    gpio_put(gpio_num, 0); // Turn off the LED
}

void LED_hw::blink(int duration_ms)
{
    gpio_put(gpio_num, 1); // Turn on the LED
    sleep_ms(duration_ms);
    gpio_put(gpio_num, 0); // Turn off the LED
    sleep_ms(duration_ms);
}

