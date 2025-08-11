/**
 * Prezenter
*/

#include <Arduino.h>
#include <BleKeyboard.h>

const int FORWARD_BUTTON = 25;
const int BACKWARD_BUTTON = 27;
const int BUTTON_STATE_LED = 21;
const int BATTERY_PIN = 35;

static const BaseType_t app_cpu = 1;
BleKeyboard bleKeyboard("Prezenter", "Prezenter", 80);
static const uint8_t key_queue_len = 20;
static QueueHandle_t keyQueue;

void readButtonPress(void *parameters) {
    pinMode(FORWARD_BUTTON, INPUT_PULLDOWN);
    pinMode(BACKWARD_BUTTON, INPUT_PULLDOWN);
    bool f_last_state = LOW;
    bool b_last_state = LOW;

    while(true) {
        
        bool f_current_state = digitalRead(FORWARD_BUTTON);
        bool b_current_state = digitalRead(BACKWARD_BUTTON);
        if (f_last_state == LOW && f_current_state == HIGH) {
            uint8_t key = KEY_DOWN_ARROW;
            xQueueSend(keyQueue, &key, pdMS_TO_TICKS(0));
        } else if (b_last_state == LOW && b_current_state == HIGH) {
            uint8_t key = KEY_UP_ARROW;
            xQueueSend(keyQueue, &key, pdMS_TO_TICKS(0));
        }
        f_last_state = f_current_state;
        b_last_state = b_current_state;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

}

void sendKeyPress(void *parameters) {
    uint8_t key;
    while(true) {
        if (xQueueReceive(keyQueue, &key, pdMS_TO_TICKS(0))) {
            // Serial.printf("Key pressed: %d\n", key);
            digitalWrite(BUTTON_STATE_LED, HIGH);
            if (bleKeyboard.isConnected()) {
                bleKeyboard.write(key);
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
            digitalWrite(BUTTON_STATE_LED, LOW);
        }
    }
}

void setBatteryLevel(void *parameters) {
    const float R1 = 10000;
    const float R2 = 10000;
    const float MAX_BATTERY_VOLTAGE = 3.0;
    const float MIN_BATTERY_VOLTAGE = 2.4;
    while(true) {
        int adcValue = analogRead(BATTERY_PIN);
        float vMeasured = (adcValue / 4095.0) * 3.3;
        float batteryVoltage = vMeasured * ((R1 + R2) / R2);
        int batteryLevel = (int)(100.0 * 
            (batteryVoltage - MIN_BATTERY_VOLTAGE) / (MAX_BATTERY_VOLTAGE - MIN_BATTERY_VOLTAGE)
        );
        batteryLevel = constrain(batteryLevel, 0, 100);
        bleKeyboard.setBatteryLevel(batteryLevel);

        // Serial.printf("Battery Level: %.2fV -> %d%%\n", batteryVoltage, batteryLevel);
        vTaskDelay( 5000 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(9600);
    // vTaskDelay(200 / portTICK_PERIOD_MS);
    // Serial.println();
    // Serial.println("---Prezenter---");

    pinMode(BUTTON_STATE_LED, OUTPUT);
    analogReadResolution(12);
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
    // xTaskCreate(
    //     setBatteryLevel,
    //     "Set Battery Level",
    //     4096,
    //     NULL,
    //     1,
    //     NULL
    // );
    vTaskDelete(NULL);
}

void loop() {}
