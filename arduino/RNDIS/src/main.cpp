#include <Arduino.h>

extern "C" int rndis_init(void);
extern "C" void rndis_task(void);

void blink(int period)
{
    static uint32_t begin = 0;
    if (millis() - begin > period)
    {
        begin = millis();
        digitalToggle(LED);
    }
}

void setup()
{
    pinMode(LED, OUTPUT);
    Serial.begin(115200);
    Serial.debug(1);
    printf("PIC32MZ RNDIS 2023 WizIO\n");
    rndis_init();
    printf("SETUP DONE\n");
}

void loop()
{
    rndis_task();
    blink(100);
}
