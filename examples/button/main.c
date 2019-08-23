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
#include "button.h"

#define Relay1    16    //D0

#define button1   14    //D5

#ifndef BUTTON_PIN
#error BUTTON_PIN is not specified
#endif




static void wifi_init() {
    struct sdk_station_config wifi_config = {
        .ssid = "Cuarto",
        .password = "estudios22",
    };

    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&wifi_config);
    sdk_wifi_station_connect();
}


void button_identify(homekit_value_t _value) {
    printf("Button identify\n");
}


homekit_characteristic_t button_event = HOMEKIT_CHARACTERISTIC_(PROGRAMMABLE_SWITCH_EVENT, 0);


void button_callback(uint8_t gpio, button_event_t event) {
    switch (event) {
        case button_event_single_press:
            printf("single press\n");
            relay1_write(!state1);
            break;
        default:
            printf("unknown button event: %d\n", event);
    }
}

void relay1_write(bool on) {
    gpio_write(Relay1, on ? 1 : 0);
}


homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(
        .id=1,
        .category=homekit_accessory_category_programmable_switch,
        .services=(homekit_service_t*[]) {
            HOMEKIT_SERVICE(
                ACCESSORY_INFORMATION,
                .characteristics=(homekit_characteristic_t*[]) {
                    HOMEKIT_CHARACTERISTIC(NAME, "Button"),
                    HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Estonianport"),
                    HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "019111111123"),
                    HOMEKIT_CHARACTERISTIC(MODEL, "MyButton"),
                    HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
                    HOMEKIT_CHARACTERISTIC(IDENTIFY, button_identify),
                    NULL
                },
            ),
            HOMEKIT_SERVICE(
                STATELESS_PROGRAMMABLE_SWITCH,
                .primary=true,
                .characteristics=(homekit_characteristic_t*[]) {
                    HOMEKIT_CHARACTERISTIC(NAME, "Button"),
                    &button_event,
                    NULL
                },
            ),
            NULL
        },
    ),
    NULL
};


homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};


void user_init(void) {
    uart_set_baud(0, 115200);

    wifi_init();
    if (button_create(button1, button_callback)) {
        printf("Failed to initialize button\n");
    }
    homekit_server_init(&config);
}

