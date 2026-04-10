#pragma once

// ---- HAL compatibility: Arduino-ESP32 vs pure ESP-IDF ----
//
// Include this header instead of esp32-hal.h / Arduino.h.
// All EPD_* macros resolve to the correct platform API.

#ifdef ARDUINO
  #include <Arduino.h>
  #include <Wire.h>
  #define EPD_DELAY_MS(ms)    delay(ms)
  #define EPD_DELAY_US(us)    delayMicroseconds(us)
  #define EPD_PIN_OUTPUT(p)   pinMode((p), OUTPUT)
  #define EPD_PIN_INPUT(p)    pinMode((p), INPUT)
  #define EPD_PIN_HIGH(p)     digitalWrite((p), HIGH)
  #define EPD_PIN_LOW(p)      digitalWrite((p), LOW)
  #define EPD_PIN_READ(p)     digitalRead((p))
  #define EPD_YIELD()         yield()
  #define EPD_PRINT(...)      Serial.printf(__VA_ARGS__)
#else
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
  #include "driver/gpio.h"
  #include "esp_rom_sys.h"
  #include "epd_i2c_wrapper.h"
  #include <stdio.h>
  #define EPD_DELAY_MS(ms)    vTaskDelay(pdMS_TO_TICKS(ms))
  #define EPD_DELAY_US(us)    esp_rom_delay_us(us)
  #define EPD_PIN_OUTPUT(p)   gpio_set_direction((gpio_num_t)(p), GPIO_MODE_OUTPUT)
  #define EPD_PIN_INPUT(p)    gpio_set_direction((gpio_num_t)(p), GPIO_MODE_INPUT)
  #define EPD_PIN_HIGH(p)     gpio_set_level((gpio_num_t)(p), 1)
  #define EPD_PIN_LOW(p)      gpio_set_level((gpio_num_t)(p), 0)
  #define EPD_PIN_READ(p)     gpio_get_level((gpio_num_t)(p))
  #define EPD_YIELD()         taskYIELD()
  #define EPD_PRINT(...)      printf(__VA_ARGS__)
#endif
