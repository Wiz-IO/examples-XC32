#include <Arduino.h>

void setup()
{
    pinMode(0, OUTPUT);
    pinMode(1, OUTPUT);
    pinMode(2, OUTPUT);
    Serial.begin(115200);
    Serial.debug(true); // enable printf for this serial
    Serial.println("PIC32MZ 2023 Georgi Angelov");
    printf("SETUP\n");
}

void loop()
{
    digitalToggle(LED_ORANGE);
    delay(100);
    digitalToggle(LED_GREEN);
    delay(100);
    digitalToggle(LED_RED);
    delay(100);
}
