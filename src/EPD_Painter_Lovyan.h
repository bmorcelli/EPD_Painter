#pragma once

#include <string.h>
#include <esp_heap_caps.h>
#include "EPD_Painter.h"

// Check if LovyanGFX is available in the include path
#if __has_include(<LovyanGFX.hpp>)
#include <LovyanGFX.hpp>

// =============================================================================
// EPD_PainterLovyan
//
// Exposes the LovyanGFX drawing API (using an 8bpp grayscale LGFX_Sprite),
// and drives the eInk panel via an EPD_Painter instance.
//
// Usage:
//   EPD_Painter::Config config = { ... };
//   EPD_PainterLovyan gfx(config);
//
//   gfx.begin();
//   gfx.fillScreen(0xFF); // White
//   gfx.setCursor(10, 10);
//   gfx.print("Hello LovyanGFX!");
//   gfx.paint();
// =============================================================================
class EPD_PainterLovyan : public lgfx::LGFX_Sprite
{
public:
    // -------------------------------------------------------------------------
    // Constructor — allocates the framebuffer and wires everything together.
    // -------------------------------------------------------------------------
    explicit EPD_PainterLovyan(const EPD_Painter::Config &config, bool portrait = false)
        : lgfx::LGFX_Sprite(), _painter(config, portrait)
    {
        // Compute logical width and height based on rotation
        _logical_w = (config.rotation == EPD_Painter::Rotation::ROTATION_CW || portrait) ? config.height : config.width;
        _logical_h = (config.rotation == EPD_Painter::Rotation::ROTATION_CW || portrait) ? config.width : config.height;
    }

    ~EPD_PainterLovyan()
    {
        this->deleteSprite();
        if (_framebuffer) {
            heap_caps_free(_framebuffer);
            _framebuffer = nullptr;
        }
    }

    // -------------------------------------------------------------------------
    // begin() — allocates framebuffer, inits EPD_Painter hardware.
    // Call after Serial / I2C setup, before any drawing.
    // -------------------------------------------------------------------------
    bool begin()
    {
        this->setColorDepth(8); // 8bpp grayscale
        this->setPsram(true);   // Try allocating in PSRAM if available

        // Use external custom buffer using setBuffer instead of createSprite
        const size_t buf_size = (size_t)_painter._config.width * (size_t)_painter._config.height;
        _framebuffer = static_cast<uint8_t *>(
            heap_caps_aligned_alloc(16, buf_size, MALLOC_CAP_SPIRAM));

        if (!_framebuffer) {
            EPD_PRINT("Failed to allocate framebuffer in PSRAM\n");
            return false;
        }

        memset(_framebuffer, 0x00, buf_size);

        // LovyanGFX expects the sprite buffer to be set:
        this->setBuffer(_framebuffer, _logical_w, _logical_h, 8);

        _painter.setInterlaceMode(true);
        return _painter.begin();
    }

    // -------------------------------------------------------------------------
    // end() — mirrors EPD_Painter::end()
    // -------------------------------------------------------------------------
    bool end() { return _painter.end(); }

    // -------------------------------------------------------------------------
    // Rendering
    // -------------------------------------------------------------------------
    void paint() { _painter.paint((uint8_t*)this->getBuffer()); }
    void paintLater() { _painter.paintLater((uint8_t*)this->getBuffer()); }

    void clear(const EPD_Painter::Rect *rects = nullptr, int num_rects = 0, EPD_Painter::ClearMode mode = EPD_Painter::ClearMode::HARD) { _painter.clear(rects, num_rects, mode); }
    int computeDirtyRects(EPD_Painter::Rect *out, int max, int tolerance = 0) const { return _painter.computeDirtyRects(out, max, tolerance); }
    void clearDirtyAreas(int tolerance = 0, EPD_Painter::ClearMode mode = EPD_Painter::ClearMode::SOFT) { _painter.clearDirtyAreas((uint8_t*)this->getBuffer(), tolerance, mode); }
    void fxClear() { _painter.fxClear(); }
    void dither() { EPD_Painter::dither((uint8_t*)this->getBuffer(), _painter._config.width, _painter._config.height); }

    uint8_t* packBuffer() { return _painter.packBuffer((uint8_t*)this->getBuffer()); }

    // -------------------------------------------------------------------------
    // Quality
    // -------------------------------------------------------------------------
    void setQuality(EPD_Painter::Quality q) { _painter.setQuality(q); }

    const EPD_Painter::Config& getConfig() { return _painter.getConfig(); }
    const EPD_Painter::Config* getPreset() const { return _painter.getPreset(); }

    // -------------------------------------------------------------------------
    // Access to the underlying driver if needed
    // -------------------------------------------------------------------------
    EPD_Painter &driver() { return _painter; }

    void setAutoShutdown(bool v) { _painter.setAutoShutdown(v); }
    EPD_PainterShutdown *shutdown() { return _painter.shutdown(); }

private:
    uint8_t *_framebuffer = nullptr;
    EPD_Painter _painter;
    int _logical_w;
    int _logical_h;
};

#endif // __has_include(<LovyanGFX.hpp>)
