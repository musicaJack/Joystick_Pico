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

// 定义红线相关常量
#define LINE_WIDTH 5
#define TOP_LINE_Y 20
#define BOTTOM_LINE_Y (SCREEN_HEIGHT - 20 - LINE_WIDTH)
#define LINE_COLOR st7789::BLUE

// 定义游戏相关常量
#define GAME_TIME 20  // 游戏时间（秒）
#define MAX_STAMPS 50  // 最大盖章数量

// 定义本次显示的颜色
#define TEXT_COLOR st7789::WHITE  // 文字颜色(白色) 
#define BG_COLOR st7789::BLACK     // 背景颜色(黑色)
#define BLOCK_COLOR st7789::BLUE  // 方块颜色(蓝色)
#define STAMP_COLOR st7789::RED  // 盖章方块的颜色(红色)
#define IRON_BLOCK_COLOR st7789::GRAY  // 铁方块的颜色(铁灰色)
#define DOT_COLOR st7789::GREEN  // 游走圆点的颜色(绿色)
#define YELLOW_DOT_COLOR st7789::CYAN  // 小黄球的颜色(黄色)

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
    bool is_yellow;  // 是否为小黄球
};

// 定义小球数组
struct WanderingDots {
    WanderingDot dots[MAX_DOTS];
    uint8_t count;
};

// 定义盖章位置结构
struct StampPosition {
    BlockPosition pos;
    uint8_t hit_count;  // 碰撞次数
    bool is_iron;      // 是否为铁方块
};

// 定义盖章位置数组
struct StampPositions {
    StampPosition positions[MAX_STAMPS];
    uint8_t count;
};

// 函数前向声明
void drawRemainingStamps(st7789::ST7789& lcd, int remaining);

// 绘制方块
void drawBlock(st7789::ST7789& lcd, const BlockPosition& pos, bool is_stamp = false, bool is_iron = false) {
    if (is_iron) {
        lcd.fillRect(pos.x, pos.y, BLOCK_SIZE, BLOCK_SIZE, IRON_BLOCK_COLOR);
    } else {
        lcd.fillRect(pos.x, pos.y, BLOCK_SIZE, BLOCK_SIZE, is_stamp ? STAMP_COLOR : BLOCK_COLOR);
    }
}

// 清除方块
void clearBlock(st7789::ST7789& lcd, const BlockPosition& pos) {
    lcd.fillRect(pos.x, pos.y, BLOCK_SIZE, BLOCK_SIZE, BG_COLOR);
}

// 绘制所有盖章方块
void drawAllStamps(st7789::ST7789& lcd, const StampPositions& stamps) {
    for (uint8_t i = 0; i < stamps.count; i++) {
        drawBlock(lcd, stamps.positions[i].pos, true, stamps.positions[i].is_iron);
    }
}

// 绘制圆点
void drawDot(st7789::ST7789& lcd, const BlockPosition& pos, bool is_yellow = false) {
    lcd.fillCircle(pos.x + BLOCK_SIZE/2, pos.y + BLOCK_SIZE/2, BLOCK_SIZE/2, is_yellow ? YELLOW_DOT_COLOR : DOT_COLOR);
}

// 清除圆点
void clearDot(st7789::ST7789& lcd, const BlockPosition& pos) {
    lcd.fillCircle(pos.x + BLOCK_SIZE/2, pos.y + BLOCK_SIZE/2, BLOCK_SIZE/2, BG_COLOR);
}

// 检查位置是否与任何stamp重叠，并返回碰撞方向和索引
int checkCollisionDirection(const BlockPosition& pos, const StampPositions& stamps, uint8_t& hit_index) {
    for (uint8_t i = 0; i < stamps.count; i++) {
        if (abs(pos.x - stamps.positions[i].pos.x) < BLOCK_SIZE &&
            abs(pos.y - stamps.positions[i].pos.y) < BLOCK_SIZE) {
            // 计算碰撞方向
            int dx = pos.x - stamps.positions[i].pos.x;
            int dy = pos.y - stamps.positions[i].pos.y;
            
            hit_index = i;  // 记录碰撞的方块索引
            
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
void updateWanderingDot(WanderingDot& dot, StampPositions& stamps, st7789::ST7789& lcd) {
    if (!dot.active) return;

    // 保存旧位置
    BlockPosition old_pos = dot.pos;
    
    // 更新位置
    dot.pos.x += dot.speed_x;
    dot.pos.y += dot.speed_y;
    
    // 边界检查和反弹
    if (dot.pos.x < 0 || dot.pos.x > SCREEN_WIDTH - BLOCK_SIZE) {
        dot.pos.x = (dot.pos.x < 0) ? 0 : SCREEN_WIDTH - BLOCK_SIZE;
        dot.speed_x = -dot.speed_x;  // 反向速度
    }
    if (dot.pos.y < 0 || dot.pos.y > SCREEN_HEIGHT - BLOCK_SIZE) {
        dot.pos.y = (dot.pos.y < 0) ? 0 : SCREEN_HEIGHT - BLOCK_SIZE;
        dot.speed_y = -dot.speed_y;  // 反向速度
    }
    
    // 检查是否与stamp重叠
    uint8_t hit_index;
    if (checkCollisionDirection(dot.pos, stamps, hit_index) != 0) {
        // 回退到碰撞前的位置
        dot.pos = old_pos;
        
        // 如果是小黄球，直接移除方块
        if (dot.is_yellow) {
            // 如果是铁方块，需要6次碰撞
            if (stamps.positions[hit_index].is_iron) {
                stamps.positions[hit_index].hit_count++;
                if (stamps.positions[hit_index].hit_count >= 6) {
                    // 将最后一个方块移到当前位置
                    if (hit_index < stamps.count - 1) {
                        stamps.positions[hit_index] = stamps.positions[stamps.count - 1];
                    }
                    stamps.count--;
                    // 更新 stamps 显示
                    drawRemainingStamps(lcd, MAX_STAMPS - stamps.count);
                }
            } else {
                // 将最后一个方块移到当前位置
                if (hit_index < stamps.count - 1) {
                    stamps.positions[hit_index] = stamps.positions[stamps.count - 1];
                }
                stamps.count--;
                // 更新 stamps 显示
                drawRemainingStamps(lcd, MAX_STAMPS - stamps.count);
            }
        } else {
            // 增加碰撞计数
            stamps.positions[hit_index].hit_count++;
            
            // 如果是铁方块，需要8次碰撞
            if (stamps.positions[hit_index].is_iron) {
                if (stamps.positions[hit_index].hit_count >= 8) {
                    // 将最后一个方块移到当前位置
                    if (hit_index < stamps.count - 1) {
                        stamps.positions[hit_index] = stamps.positions[stamps.count - 1];
                    }
                    stamps.count--;
                    // 更新 stamps 显示
                    drawRemainingStamps(lcd, MAX_STAMPS - stamps.count);
                }
            } else {
                // 普通方块需要2次碰撞
                if (stamps.positions[hit_index].hit_count >= 2) {
                    // 将最后一个方块移到当前位置
                    if (hit_index < stamps.count - 1) {
                        stamps.positions[hit_index] = stamps.positions[stamps.count - 1];
                    }
                    stamps.count--;
                    // 更新 stamps 显示
                    drawRemainingStamps(lcd, MAX_STAMPS - stamps.count);
                }
            }
        }
        
        // 生成新的随机速度，但保持一定的速度大小
        do {
            dot.speed_x = static_cast<int16_t>((rand() % 7) - 3);  // -3到3
            dot.speed_y = static_cast<int16_t>((rand() % 7) - 3);  // -3到3
        } while (abs(dot.speed_x) < 2 || abs(dot.speed_y) < 2);  // 确保速度足够大
        
        // 确保速度不会太小
        if (abs(dot.speed_x) < 2) dot.speed_x = (dot.speed_x >= 0) ? 2 : -2;
        if (abs(dot.speed_y) < 2) dot.speed_y = (dot.speed_y >= 0) ? 2 : -2;
    }
}

// 更新所有小球的位置
void updateAllDots(WanderingDots& dots, StampPositions& stamps, st7789::ST7789& lcd) {
    for (uint8_t i = 0; i < dots.count; i++) {
        updateWanderingDot(dots.dots[i], stamps, lcd);
    }
}

// 绘制所有小球
void drawAllDots(st7789::ST7789& lcd, const WanderingDots& dots) {
    for (uint8_t i = 0; i < dots.count; i++) {
        if (dots.dots[i].active) {
            drawDot(lcd, dots.dots[i].pos, dots.dots[i].is_yellow);
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

// 绘制红线
void drawLines(st7789::ST7789& lcd) {
    // 绘制上方红线
    lcd.fillRect(0, TOP_LINE_Y, SCREEN_WIDTH, LINE_WIDTH, LINE_COLOR);
    
    // 绘制下方红线
    lcd.fillRect(0, BOTTOM_LINE_Y, SCREEN_WIDTH, LINE_WIDTH, LINE_COLOR);
}

// 检查小球是否碰到红线
bool checkLineCollision(const BlockPosition& pos) {
    int dot_center_y = pos.y + BLOCK_SIZE/2;
    return (dot_center_y <= TOP_LINE_Y + LINE_WIDTH) || 
           (dot_center_y >= BOTTOM_LINE_Y);
}

// 显示倒计时
void drawCountdown(st7789::ST7789& lcd, int remaining_seconds) {
    char time_str[10];
    snprintf(time_str, sizeof(time_str), "Time: %02d", remaining_seconds);
    lcd.drawString(2, 2, time_str, TEXT_COLOR, BG_COLOR, 2);
}

// 检查位置是否已被占用
bool isPositionOccupied(const BlockPosition& pos, const StampPositions& stamps) {
    for (uint8_t i = 0; i < stamps.count; i++) {
        if (abs(pos.x - stamps.positions[i].pos.x) < BLOCK_SIZE &&
            abs(pos.y - stamps.positions[i].pos.y) < BLOCK_SIZE) {
            return true;
        }
    }
    return false;
}

// 检查位置是否在有效区域内（两条红线之间）
bool isPositionInValidArea(const BlockPosition& pos) {
    // 检查方块是否完全在两条红线之间
    // 考虑方块的大小，确保整个方块都在有效区域内
    return (pos.y + BLOCK_SIZE <= BOTTOM_LINE_Y) && 
           (pos.y >= TOP_LINE_Y + LINE_WIDTH);
}

// 显示剩余方块数量
void drawRemainingStamps(st7789::ST7789& lcd, int remaining) {
    char stamps_str[20];
    snprintf(stamps_str, sizeof(stamps_str), "Stamps: %02d", remaining);
    lcd.drawString(2, SCREEN_HEIGHT - 20, stamps_str, TEXT_COLOR, BG_COLOR, 2);
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
    // 初始化成功，绿灯亮1秒后熄灭
    joystick.set_rgb_color(JOYSTICK_LED_GREEN);
    sleep_ms(1000);
    joystick.set_rgb_color(JOYSTICK_LED_OFF);
    
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
            // 绘制红线
            drawLines(lcd);
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
    bool game_paused = false;
    bool game_started = false;
    uint32_t game_start_time = 0;
    int remaining_seconds = GAME_TIME;
    // 新增LED状态变量
    static bool is_active = false;
    
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
                
                // 如果游戏暂停，再次按下中间键重新开始
                if (game_paused) {
                    game_paused = false;
                    game_started = false;
                    remaining_seconds = GAME_TIME;
                    lcd.clearScreen(BG_COLOR);
                    drawLines(lcd);
                    drawAllStamps(lcd, stamps);
                    drawAllDots(lcd, wandering_dots);
                    continue;
                }
                
                // 单击：绘制stamp
                if (stamps.count < MAX_STAMPS && (MAX_STAMPS - stamps.count) > 0) {
                    // 检查位置是否在有效区域内且未被占用
                    if (isPositionInValidArea(block_pos) && !isPositionOccupied(block_pos, stamps)) {
                        stamps.positions[stamps.count].pos = block_pos;
                        stamps.positions[stamps.count].hit_count = 0;  // 初始化碰撞计数
                        stamps.positions[stamps.count].is_iron = false;
                        stamps.count++;
                        drawBlock(lcd, block_pos, true, false);
                        drawRemainingStamps(lcd, MAX_STAMPS - stamps.count);
                        printf("mid(%d)\n", stamps.count);
                    } else {
                        // 检查是否在已有方块上
                        for (uint8_t i = 0; i < stamps.count; i++) {
                            if (abs(block_pos.x - stamps.positions[i].pos.x) < BLOCK_SIZE &&
                                abs(block_pos.y - stamps.positions[i].pos.y) < BLOCK_SIZE) {
                                // 如果找到重叠的方块，将其转换为铁方块
                                stamps.positions[i].is_iron = true;
                                drawBlock(lcd, stamps.positions[i].pos, true, true);
                                printf("Convert to iron block\n");
                                break;
                            }
                        }
                    }
                } else {
                    // 当 stamps 数量达到最大值时，让 stamps 显示闪烁三次
                    for (int i = 0; i < 3; i++) {
                        // 清除显示
                        lcd.fillRect(2, SCREEN_HEIGHT - 20, 120, 20, BG_COLOR);
                        sleep_ms(200);
                        // 重新显示
                        drawRemainingStamps(lcd, MAX_STAMPS - stamps.count);
                        sleep_ms(200);
                    }
                    printf("Reached maximum stamps limit (%d)\n", MAX_STAMPS);
                }
            } else if (!long_press_triggered && (current_time - button_press_start_time >= 3000)) {
                // 长按3秒触发
                long_press_triggered = true;
                
                // 添加新的小球
                if (wandering_dots.count < MAX_DOTS) {
                    // 随机决定生成小绿球还是小黄球
                    bool is_yellow = (rand() % 2) == 0;  // 50%概率生成小黄球
                    
                    WanderingDot new_dot = {
                        {(SCREEN_WIDTH - BLOCK_SIZE) / 2, (SCREEN_HEIGHT - BLOCK_SIZE) / 2},  // 初始位置（屏幕中央）
                        static_cast<int16_t>((rand() % 5) - 2),  // 随机初始X速度
                        static_cast<int16_t>((rand() % 5) - 2),  // 随机初始Y速度
                        true,     // 激活状态
                        is_yellow // 是否为小黄球
                    };
                    wandering_dots.dots[wandering_dots.count] = new_dot;
                    wandering_dots.count++;
                    drawDot(lcd, new_dot.pos, is_yellow);
                    
                    // 如果是第一个球，开始计时
                    if (!game_started) {
                        game_started = true;
                        game_start_time = current_time;
                    }
                }
            }
        } else {
            button_pressed = false;
            long_press_triggered = false;
        }

        // 如果游戏暂停，跳过更新
        if (game_paused) {
            sleep_ms(JOYSTICK_LOOP_DELAY_MS);
            continue;
        }

        // 更新倒计时
        if (game_started) {
            uint32_t current_time = to_ms_since_boot(get_absolute_time());
            int elapsed_seconds = (current_time - game_start_time) / 1000;
            remaining_seconds = GAME_TIME - elapsed_seconds;
            
            if (remaining_seconds <= 0) {
                game_paused = true;
                lcd.drawString(SCREEN_WIDTH/2 - 40, SCREEN_HEIGHT/2, "You Win!", TEXT_COLOR, BG_COLOR, 2);
                // 等待5秒后重新开始
                sleep_ms(5000);
                // 重置游戏状态
                game_paused = false;
                game_started = false;
                remaining_seconds = GAME_TIME;
                stamps.count = 0;  // 清空所有方块
                wandering_dots.count = 0;  // 清空所有小球
                lcd.clearScreen(BG_COLOR);
                drawLines(lcd);
                continue;
            }
            
            drawCountdown(lcd, remaining_seconds);
        }

        // 检查方向（摇杆移动）
        uint16_t adc_x = 0, adc_y = 0;
        joystick.get_joy_adc_16bits_value_xy(&adc_x, &adc_y);
        int16_t offset_x = joystick.get_joy_adc_12bits_offset_value_x();
        int16_t offset_y = joystick.get_joy_adc_12bits_offset_value_y();
        int raw_direction = determine_joystick_direction(offset_x, offset_y);
        // 检查mid键
        bool mid_pressed = (joystick.get_button_value() == 0);
        static bool last_mid_pressed = false;
        static absolute_time_t last_red_time = {0};
        // 检测mid键按下的瞬间
        if (mid_pressed && !last_mid_pressed) {
            joystick.set_rgb_color(JOYSTICK_LED_RED);
            last_red_time = get_absolute_time();
        }
        // 50ms后自动关灯
        if (!is_nil_time(last_red_time) && absolute_time_diff_us(last_red_time, get_absolute_time()) > 50000) {
            joystick.set_rgb_color(JOYSTICK_LED_OFF);
            last_red_time = {0};
        }
        last_mid_pressed = mid_pressed;
        // 其余LED逻辑
        if (!mid_pressed && last_red_time == (absolute_time_t){0}) {
            if (raw_direction > 0 && !is_active) {
                is_active = true;
                joystick.set_rgb_color(JOYSTICK_LED_BLUE);
            } else if (raw_direction == 0 && is_active) {
                is_active = false;
                joystick.set_rgb_color(JOYSTICK_LED_OFF);
            }
        }

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
                    block_pos.y = (block_pos.y - MOVE_STEP < TOP_LINE_Y + LINE_WIDTH) ? (TOP_LINE_Y + LINE_WIDTH) : block_pos.y - MOVE_STEP;
                    break;
                case 2:  // 下
                    block_pos.y = (block_pos.y + MOVE_STEP > BOTTOM_LINE_Y - BLOCK_SIZE) ? 
                                 BOTTOM_LINE_Y - BLOCK_SIZE : block_pos.y + MOVE_STEP;
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
                drawBlock(lcd, block_pos, false, false);
                // 重新绘制所有盖章方块
                drawAllStamps(lcd, stamps);
                // 重新绘制红线
                drawLines(lcd);
            }
        }

        // 更新和绘制所有小球
        clearAllDots(lcd, wandering_dots);
        updateAllDots(wandering_dots, stamps, lcd);
        
        // 检查是否有小球碰到红线
        for (uint8_t i = 0; i < wandering_dots.count; i++) {
            if (wandering_dots.dots[i].active && checkLineCollision(wandering_dots.dots[i].pos)) {
                game_paused = true;
                lcd.drawString(SCREEN_WIDTH/2 - 40, SCREEN_HEIGHT/2, "You Lost!", TEXT_COLOR, BG_COLOR, 2);
                // 等待5秒后重新开始
                sleep_ms(5000);
                // 重置游戏状态
                game_paused = false;
                game_started = false;
                remaining_seconds = GAME_TIME;
                stamps.count = 0;  // 清空所有方块
                wandering_dots.count = 0;  // 清空所有小球
                lcd.clearScreen(BG_COLOR);
                drawLines(lcd);
                break;
            }
        }
        
        drawAllDots(lcd, wandering_dots);

        sleep_ms(JOYSTICK_LOOP_DELAY_MS);
    }
    
    return 0;
} 