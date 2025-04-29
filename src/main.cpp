#include <stdio.h>
#include <cstdlib>  // For abs() function
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "joystick.hpp"

// --- Configuration ---
#define I2C_PORT i2c1        // Use i2c1 bus
#define I2C_SDA_PIN 6        // I2C SDA pin
#define I2C_SCL_PIN 7        // I2C SCL pin
#define JOYSTICK_ADDR 0x63   // Joystick Unit I2C address
#define I2C_SPEED 100 * 1000 // 100 kHz

// LED color definitions
#define LED_OFF  0x000000    // Black (off)
#define LED_RED  0xFF0000    // Red
#define LED_GREEN 0x00FF00   // Green
#define LED_BLUE 0x0000FF    // Blue

// Operation detection thresholds
#define JOYSTICK_THRESHOLD 1500  // Joystick center offset threshold
#define LOOP_DELAY_MS 20        // Loop delay time (milliseconds)
#define PRINT_INTERVAL_MS 100   // Repeat print interval (milliseconds)
#define DIRECTION_RATIO 1.5     // Direction determination ratio, ensures one direction is significantly greater than the other

// ---------------------

// Create Joystick instance
Joystick joystick;

// Status tracking variables
static int16_t last_offset_x = 0;
static int16_t last_offset_y = 0;
static bool last_button_state = true; // 1 = not pressed
static bool is_active = false;        // Whether there is operation
static absolute_time_t last_print_time = {0}; // Last print time
static int last_direction = 0;  // Last direction: 0=none, 1=up, 2=down, 3=left, 4=right, 5=mid

void setup() {
    // Initialize serial
    stdio_init_all();
    
    // Allow time for USB serial connection to establish
    sleep_ms(2000);
    
    printf("Initializing Joystick Unit...\n");

    // Initialize joystick module
    if (joystick.begin(I2C_PORT, JOYSTICK_ADDR, I2C_SDA_PIN, I2C_SCL_PIN, I2C_SPEED)) {
        printf("Joystick Unit initialized successfully!\n");
        
        // Get firmware and bootloader versions
        uint8_t bootloader_ver = joystick.get_bootloader_version();
        uint8_t firmware_ver = joystick.get_firmware_version();
        printf("Bootloader version: %d, Firmware version: %d\n", bootloader_ver, firmware_ver);
        
        // Initialization successful, flash green LED once then turn off
        joystick.set_rgb_color(LED_GREEN);
        sleep_ms(250);
        joystick.set_rgb_color(LED_OFF);
        printf("Green LED flash indicates successful initialization\n");
        
        // Initialize last print time
        last_print_time = get_absolute_time();
    } else {
        printf("Error: Joystick Unit not found or initialization failed!\n");
        // Infinite loop on failure
        while (true) { 
            sleep_ms(1000);
        }
    }
}

// Print operation information and update last print time
void print_operation(int direction) {
    // If it's a new direction or time since last print exceeds the set interval
    bool is_time_to_print = absolute_time_diff_us(last_print_time, get_absolute_time()) > PRINT_INTERVAL_MS * 1000;
    
    // Only print when direction changes or print interval is reached
    if (direction != last_direction || (is_time_to_print && direction > 0)) {
        last_direction = direction;
        last_print_time = get_absolute_time();
        
        switch (direction) {
            case 1: printf("up\n"); break;
            case 2: printf("down\n"); break;
            case 3: printf("left\n"); break;
            case 4: printf("right\n"); break;
            case 5: printf("mid\n"); break;
            default: break;
        }
    }
}

// Determine joystick direction using stricter conditions
int determine_joystick_direction(int16_t offset_x, int16_t offset_y) {
    int abs_x = abs(offset_x);
    int abs_y = abs(offset_y);
    
    // If both values are below threshold, assume no operation
    if (abs_x < JOYSTICK_THRESHOLD && abs_y < JOYSTICK_THRESHOLD) {
        return 0;
    }
    
    // 为"上"方向单独使用更敏感的判断逻辑
    if (offset_y < -JOYSTICK_THRESHOLD && abs_y > abs_x * 1.5) {
        return 1; // Up - 使用更低的比例要求
    }
    
    // Only detect up/down when y-direction is significantly greater than x-direction
    if (abs_y > abs_x * DIRECTION_RATIO) {
        if (offset_y < 0) return 1; // Up
        else return 2;              // Down
    }
    
    // Only detect left/right when x-direction is significantly greater than y-direction
    if (abs_x > abs_y * DIRECTION_RATIO) {
        if (offset_x < 0) return 3; // Left
        else return 4;              // Right
    }
    
    // 防止不明确方向时保持上次方向状态，改为返回0
    return 0;
}

void loop() {
    // Use separate variables to track if operation occurred
    bool operation_detected = false;
    int current_direction = 0; // 0=none, 1=up, 2=down, 3=left, 4=right, 5=mid
    
    // Declare joystick-related variables
    uint16_t adc_x = 0, adc_y = 0;
    int16_t offset_x = 0, offset_y = 0;
    
    // Priority read button status (0=pressed, 1=not pressed)
    uint8_t button_state = joystick.get_button_value();
    
    // Read joystick data (regardless of button press to ensure state updates are available)
    joystick.get_joy_adc_16bits_value_xy(&adc_x, &adc_y);
    offset_x = joystick.get_joy_adc_12bits_offset_value_x();
    offset_y = joystick.get_joy_adc_12bits_offset_value_y();
    
    // Priority handle button status - immediate response
    if (button_state == 0) { // Button pressed
        operation_detected = true;
        current_direction = 5; // mid
    } 
    // Only detect joystick direction when button is not pressed
    else {
        // Use stricter direction determination logic
        current_direction = determine_joystick_direction(offset_x, offset_y);
        
        // Detect joystick movement
        if (current_direction > 0) {
            operation_detected = true;
        }
    }
    
    // If there is operation, print current direction
    if (current_direction > 0) {
        print_operation(current_direction);
    } else {
        // When returning to center position, reset direction state
        last_direction = 0;
    }
    
    // Immediate response to operation state changes
    // When operation occurs, immediately light up blue LED
    if (operation_detected && !is_active) {
        is_active = true;
        joystick.set_rgb_color(LED_BLUE);
    }
    // When all operations end, immediately turn off blue LED
    else if (!operation_detected && is_active) {
        is_active = false;
        joystick.set_rgb_color(LED_OFF);
    }
    
    // Update previous states
    last_button_state = button_state;
    last_offset_x = offset_x;
    last_offset_y = offset_y;
    
    // Reduced loop delay to 20ms to improve response speed
    sleep_ms(LOOP_DELAY_MS);
}

int main() {
    setup();
    while (true) {
        loop();
    }
    return 0;
} 