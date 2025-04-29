#include <stdio.h>
#include <cstdlib>  // 添加此行以使用 abs() 函数
#include "pico/stdlib.h"
#include "hardware/i2c.h"
// #include "PicoTwoWire.hpp"       // Removed compatibility layer
#include "m5_unit_joystick.hpp" // Include the (modified) joystick library header

// --- Configuration ---
// I2C Instance (e.g., i2c0 or i2c1)
#define I2C_PORT i2c1
// I2C Pins (Check your Pico board's pinout)
#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7
// I2C Address of the Joystick Unit
#define JOYSTICK_ADDR 0x63 // Use the address defined in the library header (0x63)
// I2C Speed
#define I2C_SPEED 400 * 1000 // 400 kHz
// LED 颜色定义
#define LED_OFF  0x000000    // 黑色 (关闭)
#define LED_RED  0xFF0000    // 红色
#define LED_GREEN 0x00FF00   // 绿色
#define LED_BLUE 0x0000FF    // 蓝色
// 操作检测阈值
#define JOYSTICK_THRESHOLD 1500  // 提高摇杆中位偏移阈值
#define LOOP_DELAY_MS 20        // 循环延迟时间（毫秒）
#define PRINT_INTERVAL_MS 500   // 重复打印间隔（毫秒）
#define DIRECTION_RATIO 1.5     // 方向判断比率，确保一个方向显著大于另一方向
// ---------------------

// Create an instance of the compatibility layer
// PicoTwoWire picoWire(I2C_PORT); // Removed compatibility layer instance

// Create an instance of the Joystick Unit driver
M5UnitJoystick2 joystick2;

// 上一次摇杆状态
static int16_t last_offset_x = 0;
static int16_t last_offset_y = 0;
static bool last_button_state = true; // 1 = 未按下
static bool is_active = false;        // 是否有操作
static absolute_time_t last_print_time = {0}; // 上次打印时间
static int last_direction = 0;  // 上次方向: 0=无, 1=up, 2=down, 3=left, 4=right, 5=mid

// Equivalent to Arduino setup()
void setup() {
    // 初始化串口（用于输出调试信息）
    stdio_init_all();
    
    // 留出时间让 USB 串口连接建立
    sleep_ms(2000);
    
    printf("初始化 M5Stack Joystick Unit...\n");

    // 初始化摇杆模块
    if (joystick2.begin(I2C_PORT, JOYSTICK_ADDR, I2C_SDA_PIN, I2C_SCL_PIN, I2C_SPEED)) {
        printf("M5Stack Joystick Unit 初始化成功!\n");
        
        // 获取固件和引导程序版本
        uint8_t bootloader_ver = joystick2.get_bootloader_version();
        uint8_t firmware_ver = joystick2.get_firmware_version();
        printf("Bootloader版本: %d, 固件版本: %d\n", bootloader_ver, firmware_ver);
        
        // 初始化成功，绿灯闪一下然后关闭
        joystick2.set_rgb_color(LED_GREEN);
        sleep_ms(250);
        joystick2.set_rgb_color(LED_OFF);
        printf("绿灯闪烁表示初始化成功\n");
        
        // 初始化最后打印时间
        last_print_time = get_absolute_time();
    } else {
        printf("错误: M5Stack Joystick Unit 未找到或初始化失败!\n");
        // 如果失败则循环等待
        while (true) { 
            sleep_ms(1000);
        }
    }
}

// 打印操作信息并更新上次打印时间
void print_operation(int direction) {
    // 如果是新的方向或者距离上次打印已经超过了设定的间隔
    bool is_time_to_print = absolute_time_diff_us(last_print_time, get_absolute_time()) > PRINT_INTERVAL_MS * 1000;
    
    // 只有在方向变化或达到打印间隔时才打印
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

// 判断摇杆方向，采用更严格的条件
int determine_joystick_direction(int16_t offset_x, int16_t offset_y) {
    int abs_x = abs(offset_x);
    int abs_y = abs(offset_y);
    
    // 如果两个值都低于阈值，则认为没有操作
    if (abs_x < JOYSTICK_THRESHOLD && abs_y < JOYSTICK_THRESHOLD) {
        return 0;
    }
    
    // 只有当y方向明显大于x方向时才判断为上下
    if (abs_y > abs_x * DIRECTION_RATIO) {
        if (offset_y < 0) return 1; // 上
        else return 2;              // 下
    }
    
    // 只有当x方向明显大于y方向时才判断为左右
    if (abs_x > abs_y * DIRECTION_RATIO) {
        if (offset_x < 0) return 3; // 左
        else return 4;              // 右
    }
    
    // 如果没有明显的方向，保持上一次的方向以增加稳定性
    return last_direction;
}

//  loop()
void loop() {
    // 使用单独变量记录是否有操作发生
    bool operation_detected = false;
    int current_direction = 0; // 0=无, 1=up, 2=down, 3=left, 4=right, 5=mid
    
    // 优先读取按键状态 (0=按下, 1=未按下)
    uint8_t button_state = joystick2.get_button_value();
    
    // 优先处理按钮状态 - 立即响应
    if (button_state == 0) { // 按钮按下
        operation_detected = true;
        current_direction = 5; // mid
    }
    
    // 只在没有按钮按下时检测摇杆方向
    if (current_direction == 0) {
        // 读取摇杆 X/Y 轴 ADC 值和偏移值
        uint16_t adc_x, adc_y;
        joystick2.get_joy_adc_16bits_value_xy(&adc_x, &adc_y);
        int16_t offset_x = joystick2.get_joy_adc_12bits_offset_value_x();
        int16_t offset_y = joystick2.get_joy_adc_12bits_offset_value_y();
        
        // 使用更严格的方向判断逻辑
        current_direction = determine_joystick_direction(offset_x, offset_y);
        
        // 检测摇杆移动
        if (current_direction > 0) {
            operation_detected = true;
        }
    }
    
    // 如果有操作，打印当前方向
    if (current_direction > 0) {
        print_operation(current_direction);
    } else {
        // 当回到中心位置时，重置方向状态
        last_direction = 0;
    }
    
    // 立即响应操作状态变化
    // 当有操作发生时，立即点亮蓝灯
    if (operation_detected && !is_active) {
        is_active = true;
        joystick2.set_rgb_color(LED_BLUE);
    }
    // 当所有操作结束时，立即关闭蓝灯
    else if (!operation_detected && is_active) {
        is_active = false;
        joystick2.set_rgb_color(LED_OFF);
    }
    
    // 更新上一次的状态
    last_button_state = button_state;
    last_offset_x = offset_x;
    last_offset_y = offset_y;
    
    // 循环延时减少为20ms以提高响应速度
    sleep_ms(LOOP_DELAY_MS);
}

int main() {
    // PICO_DEFAULT_LED_PIN is defined in pico/stdlib.h if you have one
    // #ifdef PICO_DEFAULT_LED_PIN
    // gpio_init(PICO_DEFAULT_LED_PIN);
    // gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    // #endif

    setup();
    while (true) {
        loop();
    }
    return 0;
} 