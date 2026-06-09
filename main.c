// /*
//  * LED-Ampelsteuerung mit FreeRTOS
//  */


#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "SSD1306/lib/ssd1306.h"

/* GPIO-Pinbelegung */
#define TASTER_PIN     16   // Taster für Fußgängeranforderung

#define LED_FUSS_ROT      14   // Fußgänger Rot
#define LED_FUSS_GRUEN    15   // Fußgänger Grün

#define LED_AUTO_GRUEN   13   // Auto Grün
#define LED_AUTO_GELB  12   // Auto Gelb
#define LED_AUTO_ROT     11   // Auto Rot


// Mutex (binäres Semaphor) zum Schutz von printf (Thread-Sicherheit)
SemaphoreHandle_t xPrintfMutex;


/* Gemeinsame Variable:
 * Signalisiert, ob ein Fußgänger die Ampel angefordert hat
 */
bool fussgaenger_wartet = false;


// Thread-sichere printf-Funktion
// Verhindert, dass mehrere Tasks gleichzeitig auf stdout schreiben
void safe_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (xPrintfMutex != NULL) {
        int ret = xSemaphoreTake(xPrintfMutex, 1000);

        if (ret == pdTRUE) {
            vprintf(format, args);
            xSemaphoreGive(xPrintfMutex);  // Mutex wieder freigeben
        }
        else {
            // Falls Mutex nicht verfügbar: Warnung und trotzdem Ausgabe
            printf(">>> [WARNING] Mutex nicht erhalten! Ausgabe evtl. fehlerhaft <<<\n");
            vprintf(format, args);
        }
    }
    else {
        // Falls Mutex noch nicht initialisiert
        printf(">>> [WARNING] Mutex nicht initialisiert! <<<\n");
        vprintf(format, args);
    }

    va_end(args);
}


// Setzt alle LEDs auf Ampelzustand
void setAmpelzustand(int autoGruen, int autoGelb, int autoRot, int fussGruen, int fussRot)
{
    gpio_put(LED_AUTO_GRUEN, autoGruen);
    gpio_put(LED_AUTO_GELB, autoGelb);
    gpio_put(LED_AUTO_ROT, autoRot);

    gpio_put(LED_FUSS_GRUEN , fussGruen);
    gpio_put(LED_FUSS_ROT , fussRot);
}

// Thread-sicheres Setzen der Fußgängeranforderung
void setFussgaengerAnforderung(bool value)
{
    if (xSemaphoreTake(xPrintfMutex, portMAX_DELAY) == pdTRUE) {
        fussgaenger_wartet = value;
        xSemaphoreGive(xPrintfMutex);
    }
}


// Thread-sicheres Lesen der Fußgängeranforderung
bool getFussgaengerAnforderung(void)
{
    bool request = false;

    // Zugriff wird über Mutex geschützt (hier etwas "overkill", aber sicher)
    if (xSemaphoreTake(xPrintfMutex, portMAX_DELAY) == pdTRUE) {
        request = fussgaenger_wartet;
        xSemaphoreGive(xPrintfMutex);
    }

    return request;
}

// Task 1: Ampelsteuerung
// Verantwortlich für den kompletten Ablauf der Ampelphasen
void AmpelTask(void *pvParameters)
{
    // Anfangszustand: Autos dürfen fahren, Fußgänger warten
    setAmpelzustand(1, 0, 0, 0, 1);
    safe_printf("[Ampel Task] Autos Gruen - Fussgaenger Rot\n");

    while (true) {

        // Nur wenn ein Fußgänger wartet, wird der Ablauf gestartet
        if (getFussgaengerAnforderung() == true) {

            // Autos Gelb (Vorwarnphase)
            setAmpelzustand(0, 1, 0, 0, 1);
            safe_printf("[Ampel Task] Autos Gelb - Fussgaenger Rot\n");
            vTaskDelay(pdMS_TO_TICKS(2000));

            // Autos Rot, kurze Sicherheitsphase
            setAmpelzustand(0, 0, 1, 0, 1);
            safe_printf("[Ampel Task] Autos Rot - Fussgaenger Rot\n");
            vTaskDelay(pdMS_TO_TICKS(1000));

            // Fußgänger Grün (Überqueren erlaubt)
            setAmpelzustand(0, 0, 1, 1, 0);
            safe_printf("[Ampel Task] Autos Rot - Fussgaenger Gruen\n");
            vTaskDelay(pdMS_TO_TICKS(5000));

            // Sicherheitsphase: alles Rot
            setAmpelzustand(0, 0, 1, 0, 1);
            safe_printf("[Ampel Task] Autos Rot - Fussgaenger Rot\n");
            vTaskDelay(pdMS_TO_TICKS(1000));

            // Autos wieder Gelb (Vorbereitung auf Grün)
            setAmpelzustand(0, 1, 0, 0, 1);
            safe_printf("[Ampel Task] Autos Gelb - Fussgaenger Rot\n");
            vTaskDelay(pdMS_TO_TICKS(2000));

            // Normalzustand wiederherstellen
            setAmpelzustand(1, 0, 0, 0, 1);
            safe_printf("[Ampel Task] Autos Gruen - Fussgaenger Rot\n");

            // Anfrage zurücksetzen (wichtig!)
            setFussgaengerAnforderung(false);
        }

        // Kleine Pause, damit CPU nicht unnötig belastet wird
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// Task 2: Tasterüberwachung
// Erkennt, ob ein Fußgänger den Knopf drückt
void TasterTask(void *pvParameters)
{
    while (true) {

        /* Pull-Up Logik:
         * 1 = nicht gedrückt
         * 0 = gedrückt
         */
        if (gpio_get(TASTER_PIN ) == 0) {

            // Nur neue Anfrage setzen, wenn noch keine aktiv ist
            if (getFussgaengerAnforderung() == false) {
                setFussgaengerAnforderung(true);
                safe_printf("[Taster Task] Taster gedrueckt!\n");
            }

            // Warten bis der Taster losgelassen wird (Entprellung light)
            while (gpio_get(TASTER_PIN ) == 0) {
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }

        // Polling-Intervall
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


// Initialisierungstask
// Wird einmal ausgeführt und startet alle anderen Tasks
void InitTask(void *pvParameters)
{
    // Mutex initialisieren (binäres Semaphor)
    xPrintfMutex = xSemaphoreCreateBinary();
    xSemaphoreGive(xPrintfMutex); // initial freigeben

    // Tasks erstellen
    xTaskCreate(AmpelTask, "AmpelTask", 1024, NULL, 1, NULL);
    xTaskCreate(TasterTask, "TasterTask", 1024, NULL, 1, NULL);

    // InitTask wird nicht mehr benötigt
    vTaskDelete(NULL);
}

void initHardware(void)
{
    // Taster konfigurieren (Eingang mit Pull-Up)
    gpio_init(TASTER_PIN);
    gpio_set_dir(TASTER_PIN, GPIO_IN);
    gpio_pull_up(TASTER_PIN);

    // Fußgänger-LEDs konfigurieren (Ausgänge)
    gpio_init(LED_FUSS_ROT);
    gpio_set_dir(LED_FUSS_ROT, GPIO_OUT);

    gpio_init(LED_FUSS_GRUEN);
    gpio_set_dir(LED_FUSS_GRUEN, GPIO_OUT);

    // Auto-LEDs konfigurieren (Ausgänge)
    gpio_init(LED_AUTO_GRUEN);
    gpio_set_dir(LED_AUTO_GRUEN, GPIO_OUT);

    gpio_init(LED_AUTO_GELB);
    gpio_set_dir(LED_AUTO_GELB, GPIO_OUT);

    gpio_init(LED_AUTO_ROT);
    gpio_set_dir(LED_AUTO_ROT, GPIO_OUT);
    // Alle LEDs initial ausschalten
    setAmpelzustand(0, 0, 0, 0, 0);

}

void initDisplay(void)
{
SSD1306_Init (SSD1306_ADDR);                                    // 0x3C

  // DRAWING
  // -------------------------------------------------------------------------------------
  SSD1306_ClearScreen ();                                         // clear screen
  SSD1306_DrawLine (0, MAX_X, 4, 4);                              // draw line
  SSD1306_SetPosition (7, 1);                                     // set position
  SSD1306_DrawString ("SSD1306 OLED DRIVER");                     // draw string
  SSD1306_DrawLine (0, MAX_X, 18, 18);                            // draw line
  SSD1306_SetPosition (40, 3);                                    // set position
  SSD1306_DrawString ("NWES IS BESTE FACH");                                // draw string
  SSD1306_SetPosition (53, 5);                                    // set position
  SSD1306_DrawString ("2021");                                    // draw string
  SSD1306_UpdateScreen (SSD1306_ADDR);                            // update

  sleep_ms (1000);
  SSD1306_InverseScreen (SSD1306_ADDR);

  sleep_ms (1000);
  SSD1306_NormalScreen (SSD1306_ADDR);
  printf("System Hallo");
}
int main()
{
    stdio_init_all();
    // Hardware (Pins, LEDs, Taster) initialisieren
    while(1)
        initDisplay();
    initHardware();
    sleep_ms(3000);
    printf("System initialisiert. Starte FreeRTOS Scheduler...\n");

    // InitTask mit höherer Priorität starten
    // → stellt sicher, dass Mutex + Tasks korrekt initialisiert werden
    xTaskCreate(InitTask, "InitTask", 1024, NULL, 2, NULL);

    // Scheduler starten (ab hier übernimmt FreeRTOS)
    vTaskStartScheduler();

    // Sollte nie erreicht werden
    while (1) {};
}