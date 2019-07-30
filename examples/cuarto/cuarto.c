#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include "wifi.h"


#define Relay1    16    //D0
#define Relay2    5     //D1

#define button1   14    //D5
#define button2   12    //D6    
#define button3   13    //D7

bool state1 = false;
bool state2 = false;


// ----------------------------------------------------   Inicializacion de wifi  -------------------------------------------------

static void wifi_init() {
    struct sdk_station_config wifi_config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
    };

    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&wifi_config);
    sdk_wifi_station_connect();
}

// --------------------------------------------------------   Logica de Relay1   -----------------------------------------------------

void relay1_write(bool on) {                             
    gpio_write(Relay1, on ? 0 : 1);
}


void relay1_init() {                                      
    gpio_enable(Relay1, GPIO_OUTPUT);
    relay1_write(state1);
}


void relay1_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            relay1_write(true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            relay1_write(false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    relay1_write(state1);
    vTaskDelete(NULL);
}


void relay1_identify(homekit_value_t _value) {
    printf("Relay 1 identify\n");
    xTaskCreate(relay1_identify_task, "Relay Identify", 128, NULL, 2, NULL);
}


homekit_value_t relay1_on_get() {
    return HOMEKIT_BOOL(state1);
}


void relay1_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    state1 = value.bool_value;
    relay1_write(state1);
}


// --------------------------------------------------------   Logica de Relay2   -----------------------------------------------------

void relay2_write(bool on) {
    gpio_write(Relay2, on ? 0 : 1);
}


void relay2_init() {
    gpio_enable(Relay2, GPIO_OUTPUT);
    relay2_write(state2);
}


void relay2_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            relay2_write(true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            relay2_write(false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    relay2_write(state2);
    vTaskDelete(NULL);
}


void relay2_identify(homekit_value_t _value) {
    printf("Relay 2 identify\n");
    xTaskCreate(relay2_identify_task, "Relay Identify", 128, NULL, 2, NULL);
}


homekit_value_t relay2_on_get() {
    return HOMEKIT_BOOL(state2);
}


void relay2_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    state2 = value.bool_value;
    relay2_write(state2);
}


// ------------------------------------------------   Configuracion del server de Homekit   ------------------------------------------

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Cuarto"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Estonian Port"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "ASD123"),
            HOMEKIT_CHARACTERISTIC(MODEL, "C.U.C.A"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, relay1_identify),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Relay 1"),
            HOMEKIT_CHARACTERISTIC(ON, false,
            .getter=relay1_on_get,
            .setter=relay1_on_set
            ),
            NULL
        }),
        NULL
    }),
    HOMEKIT_ACCESSORY(.id=2, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Cuarto"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Estonian Port"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "ASD123"),
            HOMEKIT_CHARACTERISTIC(MODEL, "C.U.C.A"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, relay2_identify),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Relay 2"),
            HOMEKIT_CHARACTERISTIC(ON, false,
            .getter=relay2_on_get,
            .setter=relay2_on_set
            ),
            NULL
        }),
        NULL
    }),
    
    NULL
};


homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};


// -----------------------------------------------------------   MAIN   ----------------------------------------------------------

void user_init(void) {
    uart_set_baud(0, 115200);

    wifi_init();
    relay1_init();
    homekit_server_init(&config);
}
