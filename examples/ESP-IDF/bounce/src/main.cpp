#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "EPD_Painter_Lovyan.h"

// Instantiate the LovyanGFX wrapper with AUTO board detection
EPD_PainterLovyan epd(EPD_PAINTER_PRESET);

struct Hello {
  float x, y;
  float vx, vy;
  int color;      // 1, 2, or 3
  int fontSize;
};

// Approximate text dimensions for "HELLO" at given font scale
const int TEXT_W = 200;  // approximate pixel width of "Hello World"
const int TEXT_H = 40;   // approximate pixel height

Hello hellos[3];

void initHellos() {
  int w = epd.width();
  int h = epd.height();

  hellos[0] = { (float)(w / 4),       (float)(h / 4),       14.5f,  9.3f, 1, 10 };
  hellos[1] = { (float)(w / 2),       (float)(h / 2),      -9.8f,  14.1f, 2, 10 };
  hellos[2] = { (float)(3 * w / 4),   (float)(3 * h / 4),   14.0f, -9.0f, 3, 10 };
}

void updateHello(Hello &h) {
  int w = epd.width();
  int ht = epd.height();

  h.x += h.vx;
  h.y += h.vy;

  // Bounce off edges (x bounds)
  if (h.x < 0) {
    h.x = 0;
    h.vx = -h.vx;
  } else if (h.x + TEXT_W > w) {
    h.x = w - TEXT_W;
    h.vx = -h.vx;
  }

  // Bounce off edges (y bounds)
  if (h.y < 0) {
    h.y = 0;
    h.vy = -h.vy;
  } else if (h.y + TEXT_H > ht) {
    h.y = ht - TEXT_H;
    h.vy = -h.vy;
  }
}

extern "C" void app_main() {
  printf("Starting EPD LovyanGFX Bounce Example...\n");

  if (!epd.begin()) {
    printf("EPD init failed\n");
    while (1) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  epd.setAutoShutdown(false);
  epd.clear();
  epd.clear();

  initHellos();
  epd.setTextSize(3);

  while (1) {
    epd.fillScreen(0); // White background
    for (int i = 0; i < 3; i++) {
      updateHello(hellos[i]);
      epd.setTextColor(hellos[i].color);
      epd.setCursor((int)hellos[i].x, (int)hellos[i].y);
      epd.print("Hello World");
    }
    epd.paint();
    epd.paint();

    // Give some time to background tasks
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
