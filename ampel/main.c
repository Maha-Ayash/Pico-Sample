// /*
//  * LED blink with FreeRTOS
//  */

// #include <FreeRTOS.h>
// #include <task.h>
// #include <stdio.h>
// #include <unistd.h>
// #include "pico/stdlib.h"
// #include "semphr.h"
// #include <time.h>

// int startrot = 1;
// int startgelb = 0;
// int startgruen = 0;

// void sleep1(double s) {
//     time_t cur_time = time(NULL);
//     while ((difftime(time(NULL), cur_time)) < s);
// }

// void WarteAuf(int* start)
// {
//     while(*start==0)
//     {
//         vTaskDelay(1);
//     }
// }

// void TaskstörungEinschalten();
// {
//     gpio_put(14, 1);

// }
// void TaskstörungAusschalten();
// {
//     gpio_put(14, 0);
// }
// void RoteLampeEinschalten()
// {
//     gpio_put(0, 1);
// }

// void RoteLampeAusschalten()
// {
//     gpio_put(0, 0);
// }

// void GelbeLampeEinschalten()
// {
//     gpio_put(15, 1);
// }

// void GelbeLampeAusschalten()
// {
//     gpio_put(15, 0);
// }

// void GrueneLampeEinschalten()
// {
//     gpio_put(16, 1);
// }

// void GrueneLampeAusschalten()
// {
//     gpio_put(16, 0);
// }

// void WarteMs(int ms)
// {
//     vTaskDelay(pdMS_TO_TICKS(ms));
// }

// void TaskRot()
// {
//     while(true) {
//         WarteAuf(&startrot);
//         startrot = 0;
//         RoteLampeEinschalten();
//         WarteMs(1500);
//         startgelb = 1;
//         RoteLampeAusschalten();
//     }
// }

// void TaskGelb()
// {
//     while(true) {
//         WarteAuf(&startgelb);
//         startgelb=0;
//         GelbeLampeEinschalten();
//         WarteMs(2000);
//         startgruen=1;
//         GelbeLampeAusschalten();
//     }
// }

// void TaskGruen()
// {
//     while(true) {
//         WarteAuf(&startgruen);
//         startgruen=0;
//         GrueneLampeEinschalten();
//         WarteMs(1000);
//         for(int	i=0;i<4;i++)
//         {
//             GrueneLampeAusschalten();
//             WarteMs(500);
//             GrueneLampeEinschalten();
//             WarteMs(500);
//         }
//         GrueneLampeAusschalten();
//         startgelb=1;
//         WarteAuf(&startgelb);
//         startgelb=0;
//         GelbeLampeEinschalten();
//         WarteMs(2000);
//         startrot=1;
//         GelbeLampeAusschalten();     
//     }
// }
// void TaskStörung()
// {
//     while(true) {
//         GelbeLampeAusschalten();
//         WarteMs(500);
//         GelbeLampeEinschalten();
//         WarteMs(500);
//     }
// }



// int main()
// {
//     stdio_init_all();
//     gpio_init(0);
//     gpio_set_dir(0, GPIO_OUT);
//     gpio_init(15);
//     gpio_set_dir(15, GPIO_OUT);
//     gpio_init(16);
//     gpio_set_dir(16, GPIO_OUT);
    

//     xTaskCreate(TaskRot, "TaskRot", 1000, NULL, 1, NULL);
//     xTaskCreate(TaskGelb, "TaskGelb", 1000, NULL, 1, NULL);
//     xTaskCreate(TaskGruen, "TaskGruen", 1000, NULL, 1, NULL);
//     vTaskStartScheduler();
    
// } 
/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

static char event_str[128];

void gpio_event_string(char *buf, uint32_t events);

void gpio_callback(uint gpio, uint32_t events) {
    // Put the GPIO event(s) that just happened into event_str
    // so we can print it
    gpio_event_string(event_str, events);
    printf("GPIO %d %s\n", gpio, event_str);
}

int main() {
    stdio_init_all();

    printf("Hello GPIO IRQ\n");
    gpio_init(4); // Schalter an GPIO-Pin 4 initialisieren
    gpio_init(5); // LED an GPIO-Pin 5 initialisieren
    gpio_set_dir(5, GPIO_OUT); // LED als Ausgang konfigurieren

    gpio_set_irq_enabled_with_callback(4, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true,
                                       &gpio_callback); // Interrupt für den Schalter aktivieren

    // Warten, da der Schalter und die LED externe Hardware erfordern
    while (1);
}

static const char *gpio_irq_str[] = {
        "LEVEL_LOW", // 0x1
        "LEVEL_HIGH", // 0x2
        "EDGE_FALL", // 0x4
        "EDGE_RISE" // 0x8
};

void gpio_event_string(char *buf, uint32_t events) {
    for (uint i = 0; i < 4; i++) {
        uint mask = (1 << i);
        if (events & mask) {
            // Kopiere diesen Ereignisstring in die Benutzerzeichenfolge
            const char *event_str = gpio_irq_str[i];
            while (*event_str != '\0') {
                *buf++ = *event_str++;
            }
            events &= ~mask;

            // Wenn weitere Ereignisse vorhanden sind, füge ", " hinzu
            if (events) {
                *buf++ = ',';
                *buf++ = ' ';
            }
        }
    }
    *buf++ = '\0';
}
