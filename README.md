# Joystick_Pico Driver

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%20Pico-brightgreen.svg)
![Version](https://img.shields.io/badge/version-1.0.0-orange.svg)

English | [中文](README.zh.md)

This is a driver for the Raspberry Pi Pico platform that supports both joystick module and ST7789 display. The driver provides comprehensive joystick control, button detection, direction recognition, RGB status indication, and display support.

## Features

### Joystick Module
- I2C communication (default 100kHz, configurable)
- 12-bit and 8-bit ADC value reading
- Enhanced direction detection (up, down, left, right, center)
- Advanced debouncing and stability algorithms
- Button state detection
- RGB LED status indication
- Calibration capabilities
- Complete error handling
- Direction stability enhancements

### ST7789 Display
- SPI communication interface
- Support for multiple resolutions
- Built-in display buffer
- Basic graphics drawing support
- Text display support
- Image display support

## Hardware Requirements

- Raspberry Pi Pico
- Joystick module (I2C interface)
- ST7789 display (optional)
- Connection cables (SDA, SCL, VCC, GND)

## Pin Connections

### Joystick Module
| Pico Pin | Joystick Pin | Description |
|----------|--------------|-------------|
| GPIO6    | SDA          | I2C data line |
| GPIO7    | SCL          | I2C clock line |
| 3V3      | VCC          | Power positive |
| GND      | GND          | Power negative |

### ST7789 Display
| Pico Pin | Display Pin | Description |
|----------|-------------|-------------|
| GPIO16   | SCLK        | SPI clock line |
| GPIO17   | MOSI        | SPI data line |
| GPIO18   | CS          | Chip select |
| GPIO19   | DC          | Data/Command select |
| GPIO20   | RST         | Reset signal |
| 3V3      | VCC         | Power positive |
| GND      | GND         | Power negative |

## Quick Start

1. Include the header and create an instance
```cpp
#include "joystick.hpp"
Joystick joystick;
```

2. Initialize the device
```cpp
// Using i2c1, Address 0x63, SDA Pin 6, SCL Pin 7
joystick.begin(i2c1, 0x63, 6, 7); 
```

3. Main loop example
```cpp
while (true) {
    // Read joystick data
    uint16_t adc_x, adc_y;
    joystick.get_joy_adc_16bits_value_xy(&adc_x, &adc_y);
    
    // Read offset values (calibrated position)
    int16_t offset_x = joystick.get_joy_adc_12bits_offset_value_x();
    int16_t offset_y = joystick.get_joy_adc_12bits_offset_value_y();
    
    // Read button state (0=pressed, 1=not pressed)
    uint8_t button = joystick.get_button_value();
    
    // Process direction
    // ... direction determination code ...
    
    // Set LED based on operation state
    joystick.set_rgb_color(0x0000FF); // Blue LED
    
    sleep_ms(20);
}
```

## Status LED Usage

- Green: Device working normally/initialization successful
- Blue: Active joystick operation
- Off: No operation/idle state

## API Reference

### Initialization
```cpp
bool begin(i2c_inst_t *i2c_port, uint8_t addr = 0x63, uint sda_pin = 21, uint scl_pin = 22, uint32_t speed = 400000UL);
```

### Data Reading
```cpp
uint16_t get_joy_adc_value_x(adc_mode_t adc_bits);
uint16_t get_joy_adc_value_y(adc_mode_t adc_bits);
void get_joy_adc_16bits_value_xy(uint16_t *adc_x, uint16_t *adc_y);
void get_joy_adc_8bits_value_xy(uint8_t *adc_x, uint8_t *adc_y);
uint8_t get_button_value(void);
```

### Offset (Calibrated) Values
```cpp
int16_t get_joy_adc_12bits_offset_value_x(void);
int16_t get_joy_adc_12bits_offset_value_y(void);
int8_t get_joy_adc_8bits_offset_value_x(void);
int8_t get_joy_adc_8bits_offset_value_y(void);
```

### Status Control
```cpp
void set_rgb_color(uint32_t color);
uint32_t get_rgb_color(void);
```

### Calibration Functions
```cpp
void set_joy_adc_value_cal(uint16_t x_neg_min, uint16_t x_neg_max, uint16_t x_pos_min,
                          uint16_t x_pos_max, uint16_t y_neg_min, uint16_t y_neg_max,
                          uint16_t y_pos_min, uint16_t y_pos_max);
                          
void get_joy_adc_value_cal(uint16_t *x_neg_min, uint16_t *x_neg_max, uint16_t *x_pos_min,
                          uint16_t *x_pos_max, uint16_t *y_neg_min, uint16_t *y_neg_max,
                          uint16_t *y_pos_min, uint16_t *y_pos_max);
```

### Version Information
```cpp
uint8_t get_firmware_version(void);
uint8_t get_bootloader_version(void);
uint8_t get_i2c_address(void);
uint8_t set_i2c_address(uint8_t new_addr);
```

## Enhanced Direction Detection & Debouncing

The latest version includes advanced debouncing and stability features:

- **Direction detection with enhanced stability:**
  - Higher threshold for detecting valid movement (1800 vs previous 1500)
  - Improved direction ratio calculation for more reliable detection
  - Special handling for "up" direction with customized sensitivity
  - Deadzone implementation to reduce jitter
  
- **Advanced debouncing algorithm:**
  - Requires 3 consecutive same readings to consider a direction stable
  - Needs 5 stable center readings to confirm true joystick release
  - Gradual stability counter reduction instead of immediate reset
  - Separate tracking for active direction and release state

- **Continuous operation support:**
  - Allows repeated output when holding a direction
  - Controls output frequency with configurable interval (250ms)
  - Immediate output on direction change

## Configuration

Key parameters in `main.cpp`:
```cpp
#define JOYSTICK_THRESHOLD 1800  // Increased threshold to reduce false triggers
#define LOOP_DELAY_MS 20         // Loop delay time (milliseconds)
#define PRINT_INTERVAL_MS 250    // Repeat print interval (milliseconds)
#define DIRECTION_RATIO 1.5      // Direction determination ratio
```

## Building and Flashing

1. Set up the Raspberry Pi Pico SDK
2. Run the `build_pico.bat` script to build
3. Copy the generated `.uf2` file to the Pico when in BOOTSEL mode

## Example Code

The project provides two main examples demonstrating different use cases:

### 1. Basic Joystick Test Example (`examples/joystick_test.cpp`)

This example demonstrates the basic functionality of the joystick, including:
- Direction detection (up, down, left, right, center)
- Button state detection
- LED status indication
- Advanced debouncing algorithm

Key features:
- Uses enhanced direction detection algorithm for improved stability
- Implements intelligent debouncing mechanism
- Supports continuous operation and immediate response
- Indicates operation state through LED

### 2. Joystick with Display Integration Example (`examples/joystick_st7789.cpp`)

This example implements an interesting physics-based collision game, demonstrating how to integrate the joystick with the ST7789 display.

#### Game Rules

1. **Game Objective**:
   - Eliminate as many red blocks as possible within the time limit (20 seconds)
   - Control the green dot using the joystick
   - Eliminate red blocks through collisions

2. **Game Elements**:
   - Green dot: Player-controlled character
   - Red blocks: Targets to be eliminated
   - Blue lines: Upper and lower boundaries
   - Timer: Shows remaining time
   - Remaining blocks counter: Shows number of blocks to be eliminated

3. **Game Mechanics**:
   - Each red block requires two collisions to disappear
   - Dots bounce off boundaries
   - Dots change direction when hitting red blocks
   - Final score is displayed when time runs out

4. **Control Method**:
   - Use joystick to control the green dot's movement
   - Joystick direction determines dot's movement direction
   - Joystick movement amplitude determines dot's speed

5. **Scoring Rules**:
   - 1 point for each eliminated red block
   - Final score = Number of eliminated blocks

#### Technical Implementation

1. **Physics System**:
   - Implements basic physics collision detection
   - Supports multiple dots moving simultaneously
   - Implements boundary collision and bounce effects

2. **Display System**:
   - Real-time game screen updates
   - Displays timer and score
   - Supports dynamic graphics drawing

3. **Control System**:
   - Precise control using joystick
   - Implements smooth movement control
   - Supports multi-directional simultaneous input

4. **Game State Management**:
   - Tracks state of all game elements
   - Manages collision detection and response
   - Handles game start and end logic

### Example Program Feature Comparison

| Feature | Basic Test Example | Display Integration Example |
|---------|-------------------|----------------------------|
| Direction Detection | ✓ | ✓ |
| Button Detection | ✓ | ✓ |
| LED Indication | ✓ | - |
| Display Support | - | ✓ |
| Physics Simulation | - | ✓ |
| Collision Detection | - | ✓ |
| Game Logic | - | ✓ |
| Real-time Display | - | ✓ |

## Notes

1. Green LED flash indicates successful initialization
2. Blue LED shows active operation, automatically turning off upon release
3. Configure parameters for your specific hardware sensitivity
4. The debouncing algorithm balances responsiveness with stability
5. Ensure adequate power supply when using the display

## License

MIT License

## Contributions

Issues and Pull Requests are welcome to improve this project. 