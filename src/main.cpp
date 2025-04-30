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
#define JOYSTICK_THRESHOLD 1800  // Increased joystick threshold to reduce false triggers
#define LOOP_DELAY_MS 20        // Loop delay time (milliseconds)
#define PRINT_INTERVAL_MS 250   // Repeat print interval (milliseconds)
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
    // If it's a new direction or transition from no direction to having direction, print immediately
    // If same direction is held, print at time intervals
    bool is_time_to_print = absolute_time_diff_us(last_print_time, get_absolute_time()) > PRINT_INTERVAL_MS * 1000;
    
    // Print when direction changes or time interval is reached
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
    
    // Use higher threshold to detect valid movement
    if (abs_x < JOYSTICK_THRESHOLD && abs_y < JOYSTICK_THRESHOLD) {
        return 0;
    }
    
    // Introduce deadzone concept to reduce jitter
    if (abs_x < JOYSTICK_THRESHOLD * 0.8 && abs_y < JOYSTICK_THRESHOLD * 0.8) {
        return 0;
    }
    
    // Use more sensitive detection logic for "up" direction
    if (offset_y < -JOYSTICK_THRESHOLD * 1.1 && abs_y > abs_x * 1.3) {
        return 1; // Up - using lower ratio requirement
    }
    
    // Use higher direction ratio for up/down detection to improve stability
    if (abs_y > abs_x * (DIRECTION_RATIO + 0.2)) {
        if (offset_y < -JOYSTICK_THRESHOLD) return 1; // Up 
        else if (offset_y > JOYSTICK_THRESHOLD) return 2; // Down
        else return 0; // Unclear direction
    }
    
    // Use higher direction ratio for left/right detection to improve stability
    if (abs_x > abs_y * (DIRECTION_RATIO + 0.2)) {
        if (offset_x < -JOYSTICK_THRESHOLD) return 3; // Left
        else if (offset_x > JOYSTICK_THRESHOLD) return 4; // Right
        else return 0; // Unclear direction
    }
    
    // Return 0 when direction is unclear
    return 0;
}

void loop() {
    // Use separate variables to track if operation occurred
    bool operation_detected = false;
    int current_direction = 0; // 0=none, 1=up, 2=down, 3=left, 4=right, 5=mid
    static int previous_raw_direction = 0; // Previous direction for debouncing
    static uint8_t stable_count = 0; // Stability counter
    static uint8_t release_count = 0; // Release counter to confirm joystick returned to center
    
    // Declare joystick-related variables
    uint16_t adc_x = 0, adc_y = 0;
    int16_t offset_x = 0, offset_y = 0;
    
    // Priority read button status (0=pressed, 1=not pressed)
    uint8_t button_state = joystick.get_button_value();
    
    // Read joystick data (regardless of button press to ensure state updates are available)
    joystick.get_joy_adc_16bits_value_xy(&adc_x, &adc_y);
    offset_x = joystick.get_joy_adc_12bits_offset_value_x();
    offset_y = joystick.get_joy_adc_12bits_offset_value_y();
    
    // Get raw direction signal
    int raw_direction = 0;
    
    // Priority handle button status - immediate response
    if (button_state == 0) { // Button pressed
        raw_direction = 5; // mid button
    } 
    // Only detect joystick direction when button is not pressed
    else {
        // Use stricter direction determination logic
        raw_direction = determine_joystick_direction(offset_x, offset_y);
    }
    
    // Enhanced debouncing processing
    if (raw_direction == previous_raw_direction) {
        // Direction is stable, increase stability count
        if (stable_count < 3) { // Need at least 3 consecutive same readings to consider stable
            stable_count++;
        }
        
        // If in center position (0), increase release count
        if (raw_direction == 0) {
            if (release_count < 5) { // Need 5 stable center readings to confirm true release
                release_count++;
            }
        } else {
            release_count = 0; // Non-center position, reset release count
        }
    } else {
        // Direction changed, partially reset stability count
        if (stable_count > 0) {
            stable_count--;
        }
        previous_raw_direction = raw_direction;
        
        // If new direction is center, start release counting
        if (raw_direction == 0) {
            release_count = 1;
        } else {
            release_count = 0;
        }
    }
    
    // Only process when direction is stable
    if (stable_count >= 3) {
        // Button debouncing - avoid jitter triggering
        if (raw_direction != 0) { // Valid direction
            current_direction = raw_direction;
            operation_detected = true;
        }
    }
    
    // Use stricter release detection when joystick returns to center
    bool joystick_released = (release_count >= 5);
    
    // If there is operation, print current direction
    if (current_direction > 0) {
        print_operation(current_direction);
    } else if (joystick_released) {
        // Only reset direction state when joystick is confirmed to be stable at center
        last_direction = 0;
    }
    
    // Immediate response to operation state changes
    // When operation occurs, immediately light up blue LED
    if (operation_detected && !is_active) {
        is_active = true;
        joystick.set_rgb_color(LED_BLUE);
    }
    // When all operations end, immediately turn off blue LED
    else if (!operation_detected && is_active && joystick_released) {
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