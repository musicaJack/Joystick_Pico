#include <stdio.h>
#include <cstdlib>
#include <random>
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
#define TEXT_COLOR st7789::WHITE  // 文字颜色(白色) 
#define BG_COLOR st7789::BLACK     // 背景颜色(黑色)
#define BLOCK_COLOR st7789::BLUE  // 方块颜色(蓝色)
#define STAMP_COLOR st7789::RED  // 盖章方块的颜色(红色)
#define DOT_COLOR st7789::GREEN  // 游走圆点的颜色(绿色)

// 定义最大盖章数量
#define MAX_STAMPS 50

// 定义最大小球数量
#define MAX_DOTS 10

// 定义方块位置结构
struct BlockPosition {
    int16_t x;
    int16_t y;
};

// 定义游走圆点结构
struct WanderingDot {
    BlockPosition pos;
    int16_t speed_x;
    int16_t speed_y;
    bool active;
};

// 定义小球数组
struct WanderingDots {
    WanderingDot dots[MAX_DOTS];
    uint8_t count;
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

// 绘制圆点
void drawDot(st7789::ST7789& lcd, const BlockPosition& pos) {
    lcd.fillCircle(pos.x + BLOCK_SIZE/2, pos.y + BLOCK_SIZE/2, BLOCK_SIZE/2, DOT_COLOR);
}

// 清除圆点
void clearDot(st7789::ST7789& lcd, const BlockPosition& pos) {
    lcd.fillCircle(pos.x + BLOCK_SIZE/2, pos.y + BLOCK_SIZE/2, BLOCK_SIZE/2, BG_COLOR);
}

// 检查位置是否与任何stamp重叠，并返回碰撞方向
int checkCollisionDirection(const BlockPosition& pos, const StampPositions& stamps) {
    for (uint8_t i = 0; i < stamps.count; i++) {
        if (abs(pos.x - stamps.positions[i].x) < BLOCK_SIZE &&
            abs(pos.y - stamps.positions[i].y) < BLOCK_SIZE) {
            // 计算碰撞方向
            int dx = pos.x - stamps.positions[i].x;
            int dy = pos.y - stamps.positions[i].y;
            
            // 确定主要碰撞方向
            if (abs(dx) > abs(dy)) {
                return (dx > 0) ? 1 : 2;  // 1=右碰撞, 2=左碰撞
            } else {
                return (dy > 0) ? 3 : 4;  // 3=下碰撞, 4=上碰撞
            }
        }
    }
    return 0;  // 无碰撞
}

// 生成随机速度
void generateRandomSpeed(WanderingDot& dot) {
    // 生成-3到3之间的随机速度，但确保不会太慢
    do {
        dot.speed_x = static_cast<int16_t>((rand() % 7) - 3);  // -3到3
        dot.speed_y = static_cast<int16_t>((rand() % 7) - 3);  // -3到3
    } while (abs(dot.speed_x) < 2 && abs(dot.speed_y) < 2);  // 确保至少一个方向的速度足够大
}

// 更新游走圆点的位置
void updateWanderingDot(WanderingDot& dot, const StampPositions& stamps) {
    if (!dot.active) return;

    // 保存旧位置
    BlockPosition old_pos = dot.pos;
    
    // 更新位置
    dot.pos.x += dot.speed_x;
    dot.pos.y += dot.speed_y;
    
    // 边界检查和反弹
    if (dot.pos.x < 0 || dot.pos.x > SCREEN_WIDTH - BLOCK_SIZE) {
        dot.pos.x = (dot.pos.x < 0) ? 0 : SCREEN_WIDTH - BLOCK_SIZE;
        generateRandomSpeed(dot);
    }
    if (dot.pos.y < 0 || dot.pos.y > SCREEN_HEIGHT - BLOCK_SIZE) {
        dot.pos.y = (dot.pos.y < 0) ? 0 : SCREEN_HEIGHT - BLOCK_SIZE;
        generateRandomSpeed(dot);
    }
    
    // 检查是否与stamp重叠
    if (checkCollisionDirection(dot.pos, stamps) != 0) {
        // 如果发生碰撞，立即生成新的随机速度
        generateRandomSpeed(dot);
        // 回退到碰撞前的位置
        dot.pos = old_pos;
    }
}

// 更新所有小球的位置
void updateAllDots(WanderingDots& dots, const StampPositions& stamps) {
    for (uint8_t i = 0; i < dots.count; i++) {
        updateWanderingDot(dots.dots[i], stamps);
    }
}

// 绘制所有小球
void drawAllDots(st7789::ST7789& lcd, const WanderingDots& dots) {
    for (uint8_t i = 0; i < dots.count; i++) {
        if (dots.dots[i].active) {
            drawDot(lcd, dots.dots[i].pos);
        }
    }
}

// 清除所有小球
void clearAllDots(st7789::ST7789& lcd, const WanderingDots& dots) {
    for (uint8_t i = 0; i < dots.count; i++) {
        if (dots.dots[i].active) {
            clearDot(lcd, dots.dots[i].pos);
        }
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
    
    // 初始化随机数生成器
    srand(to_ms_since_boot(get_absolute_time()));
    
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
    
    // 初始化小球数组
    WanderingDots wandering_dots = {0};
    
    // 主循环
    while (true) {
        // 检查MID按钮状态
        static uint32_t button_press_start_time = 0;
        static bool button_pressed = false;
        static bool long_press_triggered = false;
        
        if (joystick.get_button_value() == 0) {  // 按钮按下
            uint32_t current_time = to_ms_since_boot(get_absolute_time());
            
            if (!button_pressed) {  // 按钮刚被按下
                button_pressed = true;
                button_press_start_time = current_time;
                long_press_triggered = false;
                
                // 单击：绘制stamp
                if (stamps.count < MAX_STAMPS) {
                    stamps.positions[stamps.count] = block_pos;
                    stamps.count++;
                    drawBlock(lcd, block_pos, true);
                    printf("mid(%d)\n", stamps.count);
                } else {
                    printf("Reached maximum stamps limit (%d)\n", MAX_STAMPS);
                }
            } else if (!long_press_triggered && (current_time - button_press_start_time >= 3000)) {
                // 长按3秒触发
                long_press_triggered = true;
                
                // 添加新的小球
                if (wandering_dots.count < MAX_DOTS) {
                    WanderingDot new_dot = {
                        {0, 0},  // 初始位置（左上角）
                        static_cast<int16_t>((rand() % 5) - 2),  // 随机初始X速度
                        static_cast<int16_t>((rand() % 5) - 2),  // 随机初始Y速度
                        true     // 激活状态
                    };
                    wandering_dots.dots[wandering_dots.count] = new_dot;
                    wandering_dots.count++;
                    drawDot(lcd, new_dot.pos);
                }
            }
        } else {
            button_pressed = false;
            long_press_triggered = false;
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

        // 更新和绘制所有小球
        clearAllDots(lcd, wandering_dots);
        updateAllDots(wandering_dots, stamps);
        drawAllDots(lcd, wandering_dots);

        sleep_ms(JOYSTICK_LOOP_DELAY_MS);
    }
    
    return 0;
} 