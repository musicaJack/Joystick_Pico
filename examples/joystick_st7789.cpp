#include <stdio.h>
#include <cstdlib>
#include "pico/stdlib.h"
#include "joystick.hpp"
#include "joystick/joystick_config.hpp"
#include "st7789/st7789.hpp"

// 定义方块大小和移动步长
#define BLOCK_SIZE 20
#define MOVE_STEP 5

// 定义屏幕尺寸
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// 定义本次显示的颜色
#define TEXT_COLOR st7789::WHITE
#define BG_COLOR st7789::BLACK
#define BLOCK_COLOR st7789::BLUE
#define STAMP_COLOR st7789::RED  // 新增：盖章方块的颜色

// 定义最大盖章数量
#define MAX_STAMPS 50

// 定义方块位置结构
struct BlockPosition {
    int16_t x;
    int16_t y;
};

// 定义盖章位置数组
struct StampPositions {
    BlockPosition positions[MAX_STAMPS];
    uint8_t count;
};

// 绘制方块
void drawBlock(st7789::ST7789& lcd, const BlockPosition& pos, bool is_stamp = false) {
    lcd.fillRect(pos.x, pos.y, BLOCK_SIZE, BLOCK_SIZE, is_stamp ? STAMP_COLOR : BLOCK_COLOR);
}

// 清除方块
void clearBlock(st7789::ST7789& lcd, const BlockPosition& pos) {
    lcd.fillRect(pos.x, pos.y, BLOCK_SIZE, BLOCK_SIZE, BG_COLOR);
}

// 绘制所有盖章方块
void drawAllStamps(st7789::ST7789& lcd, const StampPositions& stamps) {
    for (uint8_t i = 0; i < stamps.count; i++) {
        drawBlock(lcd, stamps.positions[i], true);
    }
}

// 确定摇杆方向
int determine_joystick_direction(int16_t x, int16_t y) {
    int16_t abs_x = abs(x);
    int16_t abs_y = abs(y);
    
    if (abs_y > abs_x * (JOYSTICK_DIRECTION_RATIO + 0.2)) {
        return (y < 0) ? 1 : 2;  // 1=up, 2=down
    }
    
    if (abs_x > abs_y * (JOYSTICK_DIRECTION_RATIO + 0.2)) {
        return (x < 0) ? 3 : 4;  // 3=left, 4=right
    }
    
    return 0;  // center
}

int main() {
    stdio_init_all();
    printf("Joystick and ST7789 LCD Integration Demo\n");
    
    // 初始化显示屏
    st7789::ST7789 lcd;
    st7789::Config lcd_config;
    lcd_config.spi_inst = spi0;
    lcd_config.pin_din = 19;    // MOSI
    lcd_config.pin_sck = 18;    // SCK
    lcd_config.pin_cs = 17;     // CS
    lcd_config.pin_dc = 20;     // DC
    lcd_config.pin_reset = 15;  // RESET
    lcd_config.pin_bl = 10;     // Backlight
    lcd_config.width = SCREEN_WIDTH;
    lcd_config.height = SCREEN_HEIGHT;
    lcd_config.rotation = st7789::ROTATION_0;
    
    if (!lcd.begin(lcd_config)) {
        printf("LCD initialization failed!\n");
        return -1;
    }
    
    // 屏幕旋转180度
    lcd.setRotation(st7789::ROTATION_180);
    
    // 初始化摇杆
    Joystick joystick;
    if (!joystick.begin(JOYSTICK_I2C_PORT, JOYSTICK_I2C_ADDR, 
                       JOYSTICK_I2C_SDA_PIN, JOYSTICK_I2C_SCL_PIN, 
                       JOYSTICK_I2C_SPEED)) {
        printf("Joystick initialization failed!\n");
        return -1;
    }
    
    printf("Initialization successful!\n");
    
    // 清屏并显示启动提示
    lcd.clearScreen(BG_COLOR);
    lcd.drawString(0, 10,"Press MID BTN start",TEXT_COLOR,BG_COLOR,2);
    
    // 等待用户按下中间键
    bool started = false;
    while (!started) {
        if (joystick.get_button_value() == 0) {  // 0表示按下
            started = true;
            // 立即清屏
            lcd.clearScreen(BG_COLOR);
            sleep_ms(200);  // 消抖
        }
        sleep_ms(JOYSTICK_LOOP_DELAY_MS);
    }
    
    // 初始化方块位置（屏幕中央）
    BlockPosition block_pos = {
        (SCREEN_WIDTH - BLOCK_SIZE) / 2,
        (SCREEN_HEIGHT - BLOCK_SIZE) / 2
    };
    
    // 绘制初始方块
    drawBlock(lcd, block_pos);
    
    // 等待一小段时间，确保摇杆稳定
    sleep_ms(500);
    
    // 状态变量
    static int previous_raw_direction = 0;
    static uint8_t stable_count = 0;
    static StampPositions stamps = {0};  // 初始化盖章位置数组
    
    // 主循环
    while (true) {
        // 检查MID按钮状态
        if (joystick.get_button_value() == 0) {  // 0表示按下
            if (!started) {
                started = true;
                // 立即清屏
                lcd.clearScreen(BG_COLOR);
                printf("Program started\n");
            } else if (stamps.count < MAX_STAMPS) {  // 确保不超过最大盖章数量
                // 保存当前方块位置
                stamps.positions[stamps.count] = block_pos;
                stamps.count++;
                // 在保存的位置绘制一个永久红色方块（盖章）
                drawBlock(lcd, block_pos, true);
                // 打印盖章数量
                printf("mid(%d)\n", stamps.count);
            } else {
                printf("Reached maximum stamps limit (%d)\n", MAX_STAMPS);
            }
            sleep_ms(200);  // 消抖
        }

        // 读取摇杆数据
        int16_t offset_x = joystick.get_joy_adc_12bits_offset_value_x();
        int16_t offset_y = joystick.get_joy_adc_12bits_offset_value_y();
        
        // 获取方向
        int raw_direction = determine_joystick_direction(offset_x, offset_y);
        
        // 防抖动处理
        if (raw_direction == previous_raw_direction) {
            if (stable_count < 3) {
                stable_count++;
            }
        } else {
            if (stable_count > 0) {
                stable_count--;
            }
            previous_raw_direction = raw_direction;
        }
        
        // 只有当方向稳定时才移动
        if (stable_count >= 3) {
            // 保存旧位置
            BlockPosition old_pos = block_pos;
            
            // 根据方向移动方块
            switch (raw_direction) {
                case 1:  // 上
                    block_pos.y = (block_pos.y - MOVE_STEP < 0) ? 0 : block_pos.y - MOVE_STEP;
                    break;
                case 2:  // 下
                    block_pos.y = (block_pos.y + MOVE_STEP > SCREEN_HEIGHT - BLOCK_SIZE) ? 
                                 SCREEN_HEIGHT - BLOCK_SIZE : block_pos.y + MOVE_STEP;
                    break;
                case 3:  // 左
                    block_pos.x = (block_pos.x - MOVE_STEP < 0) ? 0 : block_pos.x - MOVE_STEP;
                    break;
                case 4:  // 右
                    block_pos.x = (block_pos.x + MOVE_STEP > SCREEN_WIDTH - BLOCK_SIZE) ? 
                                 SCREEN_WIDTH - BLOCK_SIZE : block_pos.x + MOVE_STEP;
                    break;
            }
            
            // 如果位置改变，重绘方块
            if (old_pos.x != block_pos.x || old_pos.y != block_pos.y) {
                // 清除旧方块
                clearBlock(lcd, old_pos);
                // 绘制新方块（非盖章方块）
                drawBlock(lcd, block_pos, false);
                // 重新绘制所有盖章方块
                drawAllStamps(lcd, stamps);
            }
        }
        
        sleep_ms(JOYSTICK_LOOP_DELAY_MS);
    }
    
    return 0;
} 