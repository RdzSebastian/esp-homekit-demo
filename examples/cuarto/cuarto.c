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
    gpio_write(Relay1, on ? 1 : 0);
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
    gpio_write(Relay2, on ? 1 : 0);
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


// ----------------------------------------------------------   Logica boton 1   -----------------------------------------------------

typedef struct _button {
    uint8_t gpio_num;
    button_callback_fn callback;

    uint16_t debounce_time;
    uint16_t long_press_time;
    uint16_t double_press_time;

    uint8_t press_count;
    ETSTimer press_timer;
    uint32_t last_press_time;
    uint32_t last_event_time;

    struct _button *next;
} button_t;

button_t *buttons = NULL;


static button_t *button_find_by_gpio(const uint8_t gpio_num) {
    button_t *button = buttons;
    while (button && button->gpio_num != gpio_num)
        button = button->next;

    return button;
}

void button1_identify(homekit_value_t _value) {
    printf("Button identify\n");
}


int button_create(const uint8_t gpio_num, button_callback_fn callback) {
    button_t *button = button_find_by_gpio(gpio_num);
    if (button)
        return -1;

    button = malloc(sizeof(button_t));
    memset(button, 0, sizeof(*button));
    button->gpio_num = gpio_num;
    button->callback = callback;

    // times in milliseconds
    button->debounce_time = 50;
    button->long_press_time = 1000;
    button->double_press_time = 500;

    button->next = buttons;
    buttons = button;

    gpio_set_pullup(button->gpio_num, true, true);
    gpio_set_interrupt(button->gpio_num, GPIO_INTTYPE_EDGE_ANY, button_intr_callback);

    sdk_os_timer_disarm(&button->press_timer);
    sdk_os_timer_setfn(&button->press_timer, button_timer_callback, button);

    return 0;
}


// ----------------------------------------------------------   Logica boton 2   -----------------------------------------------------

void button2_identify(homekit_value_t _value) {
    printf("Button identify\n");
}


void button2_init() {
    gpio_enable(button2, GPIO_INPUT);
    relay2_write(state2);
}


void button2_on_set(homekit_value_t value) {

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
            HOMEKIT_CHARACTERISTIC(NAME, "Luz"),
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
            HOMEKIT_CHARACTERISTIC(NAME, "Luz"),
            HOMEKIT_CHARACTERISTIC(ON, false,
            .getter=relay2_on_get,
            .setter=relay2_on_set
            ),
            NULL
        }),
        NULL
    }),
    HOMEKIT_ACCESSORY(.id=3, .category=homekit_accessory_category_programmable_switch, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Cuarto"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Estonian Port"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "ASD123"),
            HOMEKIT_CHARACTERISTIC(MODEL, "C.U.C.A"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, button1_identify),
            NULL
        }),
        HOMEKIT_SERVICE(STATELESS_PROGRAMMABLE_SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            &button_event,
            NULL
        }),
        NULL
    }),
    HOMEKIT_ACCESSORY(.id=4, .category=homekit_accessory_category_programmable_switch, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Cuarto"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Estonian Port"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "ASD123"),
            HOMEKIT_CHARACTERISTIC(MODEL, "C.U.C.A"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, button2_identify),
            NULL
        }),
        HOMEKIT_SERVICE(STATELESS_PROGRAMMABLE_SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            &button_event,
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
    relay2_init();

    if (button_create(button1, button_callback)) {
        printf("Failed to initialize button\n");
    }
    
    if (button_create(button2, button_callback)) {
        printf("Failed to initialize button\n");
    }

    homekit_server_init(&config);
}