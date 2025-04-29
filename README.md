# M5Stack Unit Joystick2 Driver

This is a driver for controlling the M5Stack Unit Joystick2 module, supporting the Raspberry Pi Pico platform. The driver provides complete joystick control, button detection, and status indication functions.

## Features

- I2C communication control (400kHz)
- 12-bit ADC value reading
- Direction detection (up, down, left, right, center)
- Button state detection
- RGB status light control
- Auto-calibration function
- Complete error handling

## Hardware Requirements

- Raspberry Pi Pico
- M5Stack Unit Joystick2 module
- Connection cables (SDA, SCL, VCC, GND)

## Pin Connections

| Pico Pin | Joystick2 Pin | Description |
|----------|--------------|------|
| GPIO6    | SDA          | I2C data line |
| GPIO7    | SCL          | I2C clock line |
| 3V3      | VCC          | Power positive |
| GND      | GND          | Power negative |

## Quick Start

1. Initialize the device
```c
joystick2_init(i2c0, 6, 7);  // Using I2C0, SDA=6, SCL=7
```

2. Main loop example
```c
while (true) {
    // Update status (button detection and LED control)
    joystick2_update();
    
    // Read joystick data
    uint16_t x, y;
    if (joystick2_read_xy(&x, &y) == JOYSTICK2_OK) {
        // Process joystick data
    }
    
    sleep_ms(10);
}
```

## Status LED Indications

- Green steady light: Device working normally
- Red steady light: Device not found or communication error
- Green flashing light: Button pressed (50ms on, 200ms off)

## API Reference

### Initialization
```c
void joystick2_init(i2c_inst_t *i2c_instance, uint sda_pin, uint scl_pin);
```

### Data Reading
```c
joystick2_error_t joystick2_read_xy(uint16_t *x, uint16_t *y);
uint8_t joystick2_get_button(void);
```

### Status Control
```c
void joystick2_set_rgb(uint32_t color);
void joystick2_update(void);
```

### Calibration Functions
```c
joystick2_error_t joystick2_get_calibration(joystick2_calibration_t *cal);
joystick2_error_t joystick2_set_calibration(const joystick2_calibration_t *cal);
joystick2_error_t joystick2_calibrate(void);
```

### Version Information
```c
uint8_t joystick2_get_firmware_version(void);
uint8_t joystick2_get_bootloader_version(void);
```

## Error Handling

The driver uses the following error codes:
```c
typedef enum {
    JOYSTICK2_OK = 0,                   // Operation successful
    JOYSTICK2_ERROR_I2C = -1,           // I2C communication error
    JOYSTICK2_ERROR_INVALID_PARAM = -2, // Parameter error
    JOYSTICK2_ERROR_NOT_INITIALIZED = -3,// Not initialized
    JOYSTICK2_ERROR_CALIBRATION = -4    // Calibration error
} joystick2_error_t;
```

## Calibration Instructions

1. Automatic calibration
```c
joystick2_error_t ret = joystick2_calibrate();
if (ret == JOYSTICK2_OK) {
    // Calibration successful
}
```

2. Manual calibration
```c
joystick2_calibration_t cal;
// Set calibration parameters
cal.x_min = 0;
cal.x_max = 4095;
cal.y_min = 0;
cal.y_max = 4095;
joystick2_set_calibration(&cal);
```

## Notes

1. Check the status LED after initialization to ensure the device is working properly
2. It is recommended to calibrate before use for more accurate data
3. The direction judgment threshold (ADC_THRESHOLD) can be adjusted according to actual needs
4. Ensure there are no address conflicts on the I2C bus (default address 0x63)

## Example Code

For complete example code, please refer to the `main.c` file.

## License

MIT License

## Contributions

Issues and Pull Requests are welcome to improve this project. 