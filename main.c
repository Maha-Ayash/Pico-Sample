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

// Pin definitions for Cars
#define CAR_RED_PIN 0
#define CAR_YELLOW_PIN 15
#define CAR_GREEN_PIN 16

// Pin definitions for Pedestrians (Assumed Pins, change if needed)
#define PED_RED_PIN 2
#define PED_GREEN_PIN 3

// Semaphore to signal that a pedestrian pressed the button
SemaphoreHandle_t xButtonSemaphore;
SemaphoreHandle_t xCarsRedSemaphore;
SemaphoreHandle_t xPedestriansRedSemaphore;
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

// Helper functions to change LEDs and print to terminal
void set_car_lights(bool red, bool yellow, bool green) 
{
    gpio_put(CAR_RED_PIN, red);
    gpio_put(CAR_YELLOW_PIN, yellow);
    gpio_put(CAR_GREEN_PIN, green);
    
    safe_printf("[CARS]        Red: %d | Yellow: %d | Green: %d\n", red, yellow, green);
}

void set_pedestrian_lights(bool red, bool green) 
{
    gpio_put(PED_RED_PIN, red);
    gpio_put(PED_GREEN_PIN, green);
    
    safe_printf("[PEDESTRIAN]  Red: %d | Green: %d\n\n", red, green);
}

void CarTrafficLightTask(void *pvParameters)
{
    // Default State: Cars go
    set_car_lights(false, false, true);

    while (true) {
        // Block here indefinitely until the button task "gives" the semaphore
        if (xSemaphoreTake(xButtonSemaphore, portMAX_DELAY) == pdTRUE) {
            safe_printf("\n>>> Pedestrian Button Pressed! Initiating sequence... <<<\n\n");
            
            // Wait a moment before changing traffic
            vTaskDelay(pdMS_TO_TICKS(1500));
            
            // Cars: Green to Yellow
            set_car_lights(false, true, false);
            vTaskDelay(pdMS_TO_TICKS(2000));
            
            // Cars: Yellow to Red
            set_car_lights(true, false, false);
            vTaskDelay(pdMS_TO_TICKS(1500)); // Security margin
            
            // Signal Pedestrian task that cars are safely stopped
            xSemaphoreGive(xCarsRedSemaphore);
            
            // Wait for Pedestrian task to finish crossing sequence
            xSemaphoreTake(xPedestriansRedSemaphore, portMAX_DELAY);
            
            // Cars: Red to Red-Yellow (Standard European cycle)
            set_car_lights(true, true, false);
            vTaskDelay(pdMS_TO_TICKS(1000));
            
            // Back to Default: Cars Green
            set_car_lights(false, false, true);
            
            // Consume any subsequent button presses that happened while they were crossing
            xSemaphoreTake(xButtonSemaphore, 0);
        }
    }
}

void PedestrianTrafficLightTask(void *pvParameters)
{
    // Default State: Pedestrians wait
    set_pedestrian_lights(true, false);

    while (true) {
        // Block here until cars have stopped
        if (xSemaphoreTake(xCarsRedSemaphore, portMAX_DELAY) == pdTRUE) {
            // Pedestrians: Red to Green
            set_pedestrian_lights(false, true);
            vTaskDelay(pdMS_TO_TICKS(5000)); // Time to cross the street
            
            // Pedestrians: Green to Red
            set_pedestrian_lights(true, false);
            vTaskDelay(pdMS_TO_TICKS(2000)); // Security margin before cars go
            
            // Signal Car task that pedestrians are safely back on the sidewalk
            xSemaphoreGive(xPedestriansRedSemaphore);
            
        }
    }
}

void ButtonTask(void *pvParameters)
{
    while (true) {
        // Listen for a character on USB terminal without blocking FreeRTOS completely
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            // Send signal to the traffic light task
            xSemaphoreGive(xButtonSemaphore);
            
            // Clear terminal input buffer if multiple keys were pressed (e.g., Return key)
            while (getchar_timeout_us(10000) != PICO_ERROR_TIMEOUT);
        }
        
        // Poll every 100ms
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void WatchdogTask(void *pvParameters)
{
    while (true) {
        safe_printf("[CARS]        Red: %d | Yellow: %d | Green: %d\n", gpio_get(CAR_RED_PIN), gpio_get(CAR_YELLOW_PIN), gpio_get(CAR_GREEN_PIN));
        safe_printf("[PEDESTRIAN]  Red: %d | Green: %d\n\n", gpio_get(PED_RED_PIN), gpio_get(PED_GREEN_PIN));
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void InitTask(void *pvParameters)
{
    // Initialize the binary semaphores
    xButtonSemaphore = xSemaphoreCreateBinary();
    
    xCarsRedSemaphore = xSemaphoreCreateBinary();

    xPedestriansRedSemaphore = xSemaphoreCreateBinary();
    xPrintfMutex = xSemaphoreCreateBinary();
    xSemaphoreGive(xPrintfMutex); // Initialize the printf mutex to be available

    // Create tasks
    xTaskCreate(CarTrafficLightTask, "CarTrafficLightTask", 1024, NULL, 1, NULL);
    xTaskCreate(PedestrianTrafficLightTask, "PedestrianTrafficLightTask", 1024, NULL, 1, NULL);
    xTaskCreate(ButtonTask, "ButtonTask", 1024, NULL, 1, NULL);
    xTaskCreate(WatchdogTask, "WatchdogTask", 1024, NULL, 1, NULL);
    
    // Delete the init task as it is no longer needed
    vTaskDelete(NULL);
}

int main()
{
    stdio_init_all();

    // Init car LEDs
    gpio_init(CAR_RED_PIN);     gpio_set_dir(CAR_RED_PIN, GPIO_OUT);
    gpio_init(CAR_YELLOW_PIN);  gpio_set_dir(CAR_YELLOW_PIN, GPIO_OUT);
    gpio_init(CAR_GREEN_PIN);   gpio_set_dir(CAR_GREEN_PIN, GPIO_OUT);
    
    // Init pedestrian LEDs
    gpio_init(PED_RED_PIN);     gpio_set_dir(PED_RED_PIN, GPIO_OUT);
    gpio_init(PED_GREEN_PIN);   gpio_set_dir(PED_GREEN_PIN, GPIO_OUT);
    sleep_ms(3000);
    printf("System Initialized. Starting FreeRTOS Scheduler...\n");

    // Create the initialization task at a higher priority (2) so it runs immediately
    // and sets up the semaphores and other tasks before they can execute.
    xTaskCreate(InitTask, "InitTask", 1024, NULL, 2, NULL);
    
    vTaskStartScheduler();
    
    while (1) {};
} 