#pragma once

#ifndef ARDUINO
#include <stdint.h>
#include <vector>
#include "driver/i2c.h"

class TwoWire {
private:
    i2c_port_t _port;
    uint8_t _address;
    std::vector<uint8_t> _tx_buffer;
    std::vector<uint8_t> _rx_buffer;
    size_t _rx_index;

public:
    TwoWire(uint8_t bus_num) : _port((i2c_port_t)bus_num), _address(0), _rx_index(0) {}
    ~TwoWire() {
        i2c_driver_delete(_port);
    }

    void* getBus() const {
        return (void*)(uintptr_t)_port;
    }

    void begin(int sda, int scl, uint32_t frequency=100000) {
        i2c_config_t conf = {};
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = sda;
        conf.scl_io_num = scl;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = frequency;
        // i2c_param_config doesn't fail unless args are invalid, we can ignore or log.
        i2c_param_config(_port, &conf);
        // Install driver if not already installed.
        // If it fails with ESP_ERR_INVALID_STATE (already installed), it's fine.
        i2c_driver_install(_port, conf.mode, 0, 0, 0);
    }

    void end() {
        i2c_driver_delete(_port);
    }

    void beginTransmission(uint8_t address) {
        _address = address;
        _tx_buffer.clear();
    }

    void write(uint8_t data) {
        _tx_buffer.push_back(data);
    }

    uint8_t endTransmission(bool sendStop = true) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        // Write operations send address + write bit.
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
        return 2; // NACK on address or other error
    }

    uint8_t requestFrom(uint8_t address, uint8_t quantity) {
        _rx_buffer.clear();
        _rx_index = 0;
        if (quantity == 0) return 0;

        _rx_buffer.resize(quantity);
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
