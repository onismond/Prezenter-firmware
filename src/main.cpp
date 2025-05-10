/**
 * Prezenter
*/

#include <Arduino.h>
#include <BleKeyboard.h>

const int btnForward = 25;
const int btnBackward = 26;
const int redLED = 14;

static const BaseType_t app_cpu = 1;
BleKeyboard bleKeyboard("Prezenter", "Bilo Technologies", 100);
static const uint8_t key_queue_len = 10;
static QueueHandle_t keyQueue;

void readButtonPress(void *parameters) {
    pinMode(btnForward, INPUT);
    pinMode(btnBackward, INPUT);
    bool f_last_state = LOW;
    bool b_last_state = LOW;

    while(true) {
        
        bool f_current_state = digitalRead(btnForward);
        bool b_current_state = digitalRead(btnBackward);
        if (f_last_state == LOW && f_current_state == HIGH) {
            uint8_t key = KEY_DOWN_ARROW;
            xQueueSend(keyQueue, &key, portMAX_DELAY);
        } else if (b_last_state == LOW && b_current_state == HIGH) {
            uint8_t key = KEY_UP_ARROW;
            xQueueSend(keyQueue, &key, portMAX_DELAY);
        }
        f_last_state = f_current_state;
        b_last_state = b_current_state;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

}

void sendKeyPress(void *parameters) {
    uint8_t key;
    while(true) {
        if (xQueueReceive(keyQueue, &key, portMAX_DELAY)) {
            Serial.printf("Key pressed: %d\n", key);
            digitalWrite(redLED, HIGH);
            if (bleKeyboard.isConnected()) {
                bleKeyboard.write(key);
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
            digitalWrite(redLED, LOW);
        }
    }
}

void setBatteryLevel(void *parameters) {
    while(true) {
        Serial.println("Battery Level");
        vTaskDelay( 10000 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(9600);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---Prezenter---");

    pinMode(redLED, OUTPUT);
    bleKeyboard.begin();
    keyQueue = xQueueCreate(key_queue_len, sizeof(uint8_t));
    xTaskCreatePinnedToCore(
        readButtonPress,
        "Read Button Press",
        2048,
        NULL,
        10,
        NULL,
        app_cpu
    );
    xTaskCreatePinnedToCore(
        sendKeyPress,
        "Send Key Press",
        4096,
        NULL,
        10,
        NULL,
        app_cpu
    );
    xTaskCreate(
        setBatteryLevel,
        "Set Battery Level",
        4096,
        NULL,
        1,
        NULL
    );
    vTaskDelete(NULL);
}

void loop() {}
