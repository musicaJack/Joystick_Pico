#include <stdio.h>
#include <cstdlib>  // For abs() function
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "joystick.hpp"
#include "joystick/joystick_config.hpp"

// Direction constants
#define DIRECTION_UP     1
#define DIRECTION_DOWN   2
#define DIRECTION_LEFT   3
#define DIRECTION_RIGHT  4
#define DIRECTION_CENTER 5

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
    stdio_init_all();
    printf("Joystick Test Program\n");
    
    // Initialize joystick
    if (joystick.begin(JOYSTICK_I2C_PORT, JOYSTICK_I2C_ADDR, 
                      JOYSTICK_I2C_SDA_PIN, JOYSTICK_I2C_SCL_PIN, 
                      JOYSTICK_I2C_SPEED)) {
        printf("Joystick initialization successful!\n");
        // Set LED to green to indicate successful initialization
        joystick.set_rgb_color(JOYSTICK_LED_GREEN);
        sleep_ms(1000);
        joystick.set_rgb_color(JOYSTICK_LED_OFF);
    } else {
        printf("Joystick initialization failed!\n");
    }
}

// Print operation information and update last print time
void print_operation(int direction) {
    static absolute_time_t last_print_time = get_absolute_time();
    bool is_time_to_print = absolute_time_diff_us(last_print_time, get_absolute_time()) > JOYSTICK_PRINT_INTERVAL_MS * 1000;
    
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
int determine_joystick_direction(int16_t x, int16_t y) {
    int16_t abs_x = abs(x);
    int16_t abs_y = abs(y);
    
    if (abs_y > abs_x * (JOYSTICK_DIRECTION_RATIO + 0.2)) {
        return (y < 0) ? DIRECTION_UP : DIRECTION_DOWN;
    }
    
    if (abs_x > abs_y * (JOYSTICK_DIRECTION_RATIO + 0.2)) {
        return (x < 0) ? DIRECTION_LEFT : DIRECTION_RIGHT;
    }
    
    return DIRECTION_CENTER;
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
        joystick.set_rgb_color(JOYSTICK_LED_BLUE);
    }
    // When all operations end, immediately turn off blue LED
    else if (!operation_detected && is_active && joystick_released) {
        is_active = false;
        joystick.set_rgb_color(JOYSTICK_LED_OFF);
    }
    
    // Update previous states
    last_button_state = button_state;
    last_offset_x = offset_x;
    last_offset_y = offset_y;
    
    // Reduced loop delay to 20ms to improve response speed
    sleep_ms(JOYSTICK_LOOP_DELAY_MS);
}

int main() {
    setup();
    while (true) {
        loop();
    }
    return 0;
} 