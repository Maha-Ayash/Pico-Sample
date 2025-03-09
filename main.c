/*
 * LED blink with FreeRTOS
 */

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <unistd.h>
#include "pico/stdlib.h"
#include "semphr.h"
#include <time.h>

#include "hardware/pwm.h"


int pwm_level = 0;

void LED_Task()
{
    while(true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        pwm_set_gpio_level(0, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
        pwm_set_gpio_level(0, pwm_level);
    }
}

void LED_Dim()
{
    while(true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        pwm_level = (pwm_level + 10) % 100;
    }
}


int main()
{
    stdio_init_all();
    gpio_init(0);
    gpio_set_dir(0, GPIO_OUT);
    gpio_set_function(0, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(0);
    pwm_set_wrap(slice_num, 99);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 1);
    pwm_set_enabled(slice_num, true);

    xTaskCreate(LED_Task, "LED_Task", 1000, NULL, 1, NULL);
    xTaskCreate(LED_Dim, "LED_Dim", 1000, NULL, 1, NULL);
    vTaskStartScheduler();

} 