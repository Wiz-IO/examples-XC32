#include <Arduino.h>
void coremark_run();

void setup()
{
    pinMode(0, OUTPUT);
    pinMode(1, OUTPUT);
    pinMode(2, OUTPUT);
    Serial.begin(115200);
    Serial.println("PIC32MZ 2023 Georgi Angelov");
    coremark_run();
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

/*
PIC32MZ 2023 Georgi Angelov
CoreMark Performance Benchmark

CoreMark measures how quickly your processor can manage linked
lists, compute matrix multiply, and execute state machine code.

Iterations/Sec is the main benchmark result, higher numbers are better
Running.... (usually requires 12 to 20 seconds)

2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 13000
Total time (secs): 13.00
Iterations/Sec   : 461.54
Iterations       : 6000
Compiler version : GCC4.8.3 MPLAB XC32 Compiler v2.10
Compiler flags   : (flags unknown)
Memory location  : STACK
seedcrc          : 0xE9F5
[0]crclist       : 0xE714
[0]crcmatrix     : 0x1FD7
[0]crcstate      : 0x8E3A
[0]crcfinal      : 0xA14C
Correct operation validated. See README.md for run and reporting rules.
CoreMark 1.0 : 461.54 / GCC4.8.3 MPLAB XC32 Compiler v2.10 (flags unknown) / STACK
*/