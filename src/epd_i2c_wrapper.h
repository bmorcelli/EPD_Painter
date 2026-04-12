#pragma once

#ifndef ARDUINO
#include <stdint.h>
#include <vector>

#if defined(CONFIG_EPD_PAINTER_ESP_IDF_OLD_API)
  #include "driver/i2c.h"
#else
  #include "driver/i2c_master.h"
#endif

class TwoWire {
private:
#if defined(CONFIG_EPD_PAINTER_ESP_IDF_OLD_API)
    i2c_port_t _port;
#else
    i2c_master_bus_handle_t _bus_handle;
    i2c_master_dev_handle_t _dev_handle;
    uint32_t _frequency;
#endif
    uint8_t _address;
    std::vector<uint8_t> _tx_buffer;
    std::vector<uint8_t> _rx_buffer;
    size_t _rx_index;

public:
#if defined(CONFIG_EPD_PAINTER_ESP_IDF_OLD_API)
    TwoWire(uint8_t bus_num) : _port((i2c_port_t)bus_num), _address(0), _rx_index(0) {}
#else
    TwoWire(uint8_t bus_num) : _bus_handle(nullptr), _dev_handle(nullptr), _frequency(100000), _address(0), _rx_index(0) {}
#endif

    ~TwoWire() {
        end();
    }

    void* getBus() const {
#if defined(CONFIG_EPD_PAINTER_ESP_IDF_OLD_API)
        return (void*)(uintptr_t)_port;
#else
        return (void*)_bus_handle;
#endif
    }

    void begin(int sda, int scl, uint32_t frequency=100000) {
#if defined(CONFIG_EPD_PAINTER_ESP_IDF_OLD_API)
        i2c_config_t conf = {};
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = sda;
        conf.scl_io_num = scl;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = frequency;
        i2c_param_config(_port, &conf);
        i2c_driver_install(_port, conf.mode, 0, 0, 0);
#else
        _frequency = frequency;
        i2c_master_bus_config_t bus_config = {};
        bus_config.i2c_port = -1; // Auto-select
        bus_config.sda_io_num = (gpio_num_t)sda;
        bus_config.scl_io_num = (gpio_num_t)scl;
        bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
        bus_config.glitch_ignore_cnt = 7;
        bus_config.flags.enable_internal_pullup = true;

        if (_bus_handle == nullptr) {
            i2c_new_master_bus(&bus_config, &_bus_handle);
        }
#endif
    }

    void end() {
#if defined(CONFIG_EPD_PAINTER_ESP_IDF_OLD_API)
        i2c_driver_delete(_port);
#else
        if (_dev_handle) {
            i2c_master_bus_rm_device(_dev_handle);
            _dev_handle = nullptr;
        }
        if (_bus_handle) {
            i2c_del_master_bus(_bus_handle);
            _bus_handle = nullptr;
        }
#endif
    }

    void beginTransmission(uint8_t address) {
        _address = address;
        _tx_buffer.clear();
    }

    void write(uint8_t data) {
        _tx_buffer.push_back(data);
    }

    uint8_t endTransmission(bool sendStop = true) {
#if defined(CONFIG_EPD_PAINTER_ESP_IDF_OLD_API)
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_WRITE, true);
        if (!_tx_buffer.empty()) {
            i2c_master_write(cmd, _tx_buffer.data(), _tx_buffer.size(), true);
        }
        if (sendStop) {
            i2c_master_stop(cmd);
        }
        esp_err_t err = i2c_master_cmd_begin(_port, cmd, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        if (err == ESP_OK) return 0; // Success
        return 2; // Error
#else
        if (!_bus_handle) return 2;

        if (_dev_handle) {
            i2c_master_bus_rm_device(_dev_handle);
            _dev_handle = nullptr;
        }

        i2c_device_config_t dev_cfg = {};
        dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        dev_cfg.device_address = _address;
        dev_cfg.scl_speed_hz = _frequency;

        if (i2c_master_bus_add_device(_bus_handle, &dev_cfg, &_dev_handle) != ESP_OK) {
            return 2;
        }

        esp_err_t err = ESP_OK;
        if (!_tx_buffer.empty()) {
            err = i2c_master_transmit(_dev_handle, _tx_buffer.data(), _tx_buffer.size(), -1);
        } else {
            // Empty transmission is a probe for device presence.
            err = i2c_master_probe(_bus_handle, _address, -1);
        }

        if (err == ESP_OK) return 0;
        return 2;
#endif
    }

    uint8_t requestFrom(uint8_t address, uint8_t quantity) {
        _rx_buffer.clear();
        _rx_index = 0;
        if (quantity == 0) return 0;
        _rx_buffer.resize(quantity);

#if defined(CONFIG_EPD_PAINTER_ESP_IDF_OLD_API)
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, true);
        if (quantity > 1) {
            i2c_master_read(cmd, _rx_buffer.data(), quantity - 1, I2C_MASTER_ACK);
        }
        i2c_master_read_byte(cmd, _rx_buffer.data() + quantity - 1, I2C_MASTER_NACK);
        i2c_master_stop(cmd);

        esp_err_t err = i2c_master_cmd_begin(_port, cmd, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
#else
        if (!_bus_handle) return 0;

        if (_dev_handle) {
            i2c_master_bus_rm_device(_dev_handle);
            _dev_handle = nullptr;
        }

        i2c_device_config_t dev_cfg = {};
        dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        dev_cfg.device_address = address;
        dev_cfg.scl_speed_hz = _frequency;

        if (i2c_master_bus_add_device(_bus_handle, &dev_cfg, &_dev_handle) != ESP_OK) {
            _rx_buffer.clear();
            return 0;
        }

        esp_err_t err = i2c_master_receive(_dev_handle, _rx_buffer.data(), quantity, -1);
#endif

        if (err == ESP_OK) {
            return quantity;
        } else {
            _rx_buffer.clear();
            return 0;
        }
    }

    int available() {
        return _rx_buffer.size() - _rx_index;
    }

    uint8_t read() {
        if (_rx_index < _rx_buffer.size()) {
            return _rx_buffer[_rx_index++];
        }
        return 0;
    }
};

#endif
