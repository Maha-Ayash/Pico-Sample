/*
 * LED blink with FreeRTOS
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include "pico/stdlib.h"


SemaphoreHandle_t xPrintfMutex;

// Thread-safe printf wrapper using a Mutex
void safe_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    if (xPrintfMutex != NULL) {
        int ret = xSemaphoreTake(xPrintfMutex, 1000);
        if(ret == pdTRUE) {
        vprintf(format, args);
        xSemaphoreGive(xPrintfMutex);
        }
        else {
            // If we can't get the mutex, just print without it (not thread-safe, but better than blocking)
            printf(">>> [WARNING] Could not obtain printf mutex! Output may be garbled. <<<\n");
            vprintf(format, args);
        }
    }
    else {
        // If the mutex hasn't been created yet, just print without it (not thread-safe, but better than blocking)
        printf(">>> [WARNING] Printf mutex not initialized! Output may be garbled. <<<\n");
        vprintf(format, args);
    }

    va_end(args);
}


void WatchdogTask(void *pvParameters)
{
    while (true) {
        safe_printf("[This is the Watchdog Task] System is alive at tick %lu\n", xTaskGetTickCount());
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void SecondaryTask(void *pvParameters)
{
    while (true) {
        safe_printf("[This is the Secondary Task] Running at tick %lu\n", xTaskGetTickCount());
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void InitTask(void *pvParameters)
{
    // Initialize the binary semaphores
    xPrintfMutex = xSemaphoreCreateBinary();
    xSemaphoreGive(xPrintfMutex); // Initialize the printf mutex to be available

    // Create tasks
    xTaskCreate(WatchdogTask, "WatchdogTask", 1024, NULL, 1, NULL);
    xTaskCreate(SecondaryTask, "SecondaryTask", 1024, NULL, 1, NULL);
    
    // Delete the init task as it is no longer needed
    vTaskDelete(NULL);
}

int main()
{
    stdio_init_all();

    // Init LEDs

    sleep_ms(3000);
    printf("System Initialized. Starting FreeRTOS Scheduler...\n");

    // Create the initialization task at a higher priority (2) so it runs immediately
    // and sets up the semaphores and other tasks before they can execute.
    xTaskCreate(InitTask, "InitTask", 1024, NULL, 2, NULL);
    
    vTaskStartScheduler();
    
    while (1) {};
} 