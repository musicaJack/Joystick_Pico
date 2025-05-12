#include <stdio.h>
#include <cstdlib>
#include <random>
#include "pico/stdlib.h"
#include "joystick.hpp"
#include "joystick/joystick_config.hpp"
#include "st7789/st7789.hpp"

// 定义屏幕尺寸
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// 定义游戏相关常量
#define BLOCK_SIZE 8
#define BLOCK_GAP 4
#define SPACESHIP_SIZE 16
#define MISSILE_SIZE 4
#define MISSILE_SPEED 5
#define SPACESHIP_SPEED 4
#define MATRIX_COUNT 5  // 1x1到5x5的矩阵数量

// 定义颜色
#define TEXT_COLOR st7789::WHITE
#define BG_COLOR st7789::BLACK
#define BLOCK_COLOR st7789::YELLOW
#define SPACESHIP_COLOR st7789::BLUE
#define MISSILE_COLOR st7789::RED
#define EXPLOSION_COLOR st7789::RED

// 定义位置结构
struct Position {
    int16_t x;
    int16_t y;
};

// 定义方块结构
struct Block {
    Position pos;
    bool active;
    uint8_t size;  // 1, 2, 或 3
    int8_t speed;  // 移动速度
    int8_t direction;  // 移动方向（1表示向右，-1表示向左）
    uint8_t hit_count;  // 被击中的次数
};

// 定义导弹结构
struct Missile {
    Position pos;
    bool active;
};

// 定义爆炸效果结构
struct Explosion {
    Position pos;
    uint8_t frame;
    bool active;
};

// 定义游戏状态
struct GameState {
    Block blocks[25];  // 最多5x5的方块
    Position spaceship;
    Missile missile;
    Explosion explosion;
    uint8_t score;
    bool game_over;
    uint8_t matrix_size;  // 当前矩阵大小（1-5）
};

// 确定摇杆方向
int determine_joystick_direction(int16_t x, int16_t y) {
    int16_t abs_x = abs(x);
    int16_t abs_y = abs(y);
    
    if (abs_y > abs_x * 1.2) {
        return (y < 0) ? 1 : 2;  // 1=up, 2=down
    }
    
    if (abs_x > abs_y * 1.2) {
        return (x < 0) ? 3 : 4;  // 3=left, 4=right
    }
    
    return 0;  // center
}

// 清除方块
void clearBlock(st7789::ST7789& lcd, const Block& block) {
    if (!block.active) return;
    uint8_t size = block.size * BLOCK_SIZE;
    lcd.fillCircle(block.pos.x + size/2, block.pos.y + size/2, size/2, BG_COLOR);
}

// 绘制方块
void drawBlock(st7789::ST7789& lcd, const Block& block) {
    if (!block.active) return;
    uint8_t size = block.size * BLOCK_SIZE;
    lcd.fillCircle(block.pos.x + size/2, block.pos.y + size/2, size/2, BLOCK_COLOR);
}

// 清除飞船
void clearSpaceship(st7789::ST7789& lcd, const Position& pos) {
    lcd.fillRect(pos.x, pos.y, SPACESHIP_SIZE, SPACESHIP_SIZE, BG_COLOR);
}

// 绘制飞船
void drawSpaceship(st7789::ST7789& lcd, const Position& pos) {
    lcd.fillRect(pos.x, pos.y, SPACESHIP_SIZE, SPACESHIP_SIZE, SPACESHIP_COLOR);
}

// 清除导弹
void clearMissile(st7789::ST7789& lcd, const Missile& missile) {
    if (!missile.active) return;
    lcd.fillRect(missile.pos.x, missile.pos.y, MISSILE_SIZE, MISSILE_SIZE, BG_COLOR);
}

// 绘制导弹
void drawMissile(st7789::ST7789& lcd, const Missile& missile) {
    if (!missile.active) return;
    lcd.fillRect(missile.pos.x, missile.pos.y, MISSILE_SIZE, MISSILE_SIZE, MISSILE_COLOR);
}

// 清除爆炸效果
void clearExplosion(st7789::ST7789& lcd, const Explosion& explosion) {
    if (!explosion.active) return;
    uint8_t size = explosion.frame * 2;
    lcd.fillCircle(explosion.pos.x + BLOCK_SIZE/2, explosion.pos.y + BLOCK_SIZE/2, size, BG_COLOR);
}

// 绘制爆炸效果
void drawExplosion(st7789::ST7789& lcd, const Explosion& explosion) {
    if (!explosion.active) return;
    uint8_t size = explosion.frame * 2;
    lcd.fillCircle(explosion.pos.x + BLOCK_SIZE/2, explosion.pos.y + BLOCK_SIZE/2, size, EXPLOSION_COLOR);
}

// 清除分数
void clearScore(st7789::ST7789& lcd) {
    lcd.fillRect(0, 0, 120, 20, BG_COLOR);
}

// 绘制分数
void drawScore(st7789::ST7789& lcd, uint8_t score) {
    char score_str[20];
    snprintf(score_str, sizeof(score_str), "Score: %d", score);
    lcd.drawString(2, 2, score_str, TEXT_COLOR, BG_COLOR, 2);
}

// 初始化游戏
void initGame(GameState& game) {
    // 初始化方块矩阵
    game.matrix_size = 1;  // 从1x1开始
    uint8_t block_index = 0;
    
    // 计算矩阵的总大小（包括间隔）
    uint8_t total_size = game.matrix_size * (BLOCK_SIZE + BLOCK_GAP) - BLOCK_GAP;
    
    // 计算起始位置（居中）
    int16_t start_x = (SCREEN_WIDTH - total_size) / 2;
    int16_t start_y = 20;
    
    for (uint8_t row = 0; row < game.matrix_size; row++) {
        for (uint8_t col = 0; col < game.matrix_size; col++) {
            game.blocks[block_index].pos.x = start_x + col * (BLOCK_SIZE + BLOCK_GAP);
            game.blocks[block_index].pos.y = start_y + row * (BLOCK_SIZE + BLOCK_GAP);
            game.blocks[block_index].active = true;
            game.blocks[block_index].size = 5;  // 初始大小为5倍
            // 设置随机移动速度和方向
            game.blocks[block_index].speed = (rand() % 3) + 1;  // 1-3的随机速度
            game.blocks[block_index].direction = (rand() % 2) ? 1 : -1;  // 随机方向
            game.blocks[block_index].hit_count = 0;  // 初始化击中次数
            block_index++;
        }
    }
    
    // 初始化飞船位置
    game.spaceship.x = (SCREEN_WIDTH - SPACESHIP_SIZE) / 2;
    game.spaceship.y = SCREEN_HEIGHT - SPACESHIP_SIZE - 20;
    
    // 初始化导弹
    game.missile.active = false;
    
    // 初始化爆炸效果
    game.explosion.active = false;
    
    // 初始化分数
    game.score = 0;
    game.game_over = false;
}

// 检查碰撞
bool checkCollision(const Position& pos1, uint8_t size1, const Position& pos2, uint8_t size2) {
    // 计算两个物体的边界
    int16_t left1 = pos1.x;
    int16_t right1 = pos1.x + size1;
    int16_t top1 = pos1.y;
    int16_t bottom1 = pos1.y + size1;
    
    int16_t left2 = pos2.x;
    int16_t right2 = pos2.x + size2;
    int16_t top2 = pos2.y;
    int16_t bottom2 = pos2.y + size2;
    
    // 检查是否有重叠
    return !(right1 < left2 || left1 > right2 || bottom1 < top2 || top1 > bottom2);
}

// 更新游戏状态
void updateGame(GameState& game, int direction, bool fire, st7789::ST7789& lcd) {
    if (game.game_over) return;
    
    // 保存旧位置
    Position old_spaceship = game.spaceship;
    Missile old_missile = game.missile;
    Explosion old_explosion = game.explosion;
    
    // 更新飞船位置
    if (direction == 1) {  // 上
        game.spaceship.y = std::max(0, game.spaceship.y - SPACESHIP_SPEED);
    } else if (direction == 2) {  // 下
        game.spaceship.y = std::min(SCREEN_HEIGHT - SPACESHIP_SIZE, game.spaceship.y + SPACESHIP_SPEED);
    } else if (direction == 3) {  // 左
        game.spaceship.x = std::max(0, game.spaceship.x - SPACESHIP_SPEED);
    } else if (direction == 4) {  // 右
        game.spaceship.x = std::min(SCREEN_WIDTH - SPACESHIP_SIZE, game.spaceship.x + SPACESHIP_SPEED);
    }
    
    // 如果飞船位置改变，清除旧位置并绘制新位置
    if (old_spaceship.x != game.spaceship.x || old_spaceship.y != game.spaceship.y) {
        clearSpaceship(lcd, old_spaceship);
        drawSpaceship(lcd, game.spaceship);
    }
    
    // 发射导弹
    if (fire && !game.missile.active) {
        game.missile.pos.x = game.spaceship.x + SPACESHIP_SIZE/2 - MISSILE_SIZE/2;
        game.missile.pos.y = game.spaceship.y;
        game.missile.active = true;
        drawMissile(lcd, game.missile);
    }
    
    // 更新导弹位置和检查碰撞
    if (game.missile.active) {
        // 先清除旧位置
        clearMissile(lcd, game.missile);
        
        // 更新位置
        game.missile.pos.y -= MISSILE_SPEED;
        
        // 检查是否超出屏幕
        if (game.missile.pos.y < 0) {
            game.missile.active = false;
        } else {
            // 检查碰撞
            bool collision = false;
            for (uint8_t i = 0; i < game.matrix_size * game.matrix_size; i++) {
                if (game.blocks[i].active && 
                    checkCollision(game.missile.pos, MISSILE_SIZE, 
                                 game.blocks[i].pos, game.blocks[i].size * BLOCK_SIZE)) {
                    // 创建爆炸效果
                    game.explosion.pos = game.blocks[i].pos;
                    game.explosion.frame = 0;
                    game.explosion.active = true;
                    
                    // 增加击中次数
                    game.blocks[i].hit_count++;
                    
                    // 根据击中次数调整大小
                    if (game.blocks[i].hit_count >= 5) {
                        // 第五次击中，游戏胜利
                        game.game_over = true;
                        lcd.drawString(SCREEN_WIDTH/2 - 40, SCREEN_HEIGHT/2, 
                                     "You Win!", TEXT_COLOR, BG_COLOR, 2);
                        lcd.drawString(SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT/2 + 30, 
                                     "Press MID to restart", TEXT_COLOR, BG_COLOR, 2);
                    } else {
                        // 根据击中次数设置新的大小
                        game.blocks[i].size = 5 - game.blocks[i].hit_count;
                        
                        // 清除旧位置
                        clearBlock(lcd, game.blocks[i]);
                        
                        // 绘制新大小
                        drawBlock(lcd, game.blocks[i]);
                    }
                    
                    // 增加分数
                    game.score += game.blocks[i].size;
                    clearScore(lcd);
                    drawScore(lcd, game.score);
                    
                    // 标记碰撞
                    collision = true;
                    
                    // 检查是否需要增加矩阵大小
                    bool all_blocks_destroyed = true;
                    for (uint8_t j = 0; j < game.matrix_size * game.matrix_size; j++) {
                        if (game.blocks[j].active) {
                            all_blocks_destroyed = false;
                            break;
                        }
                    }
                    
                    if (all_blocks_destroyed && game.matrix_size < 5) {
                        game.matrix_size++;
                        initGame(game);
                        // 重新绘制所有游戏元素
                        for (uint8_t j = 0; j < game.matrix_size * game.matrix_size; j++) {
                            drawBlock(lcd, game.blocks[j]);
                        }
                        drawSpaceship(lcd, game.spaceship);
                        drawScore(lcd, game.score);
                    }
                    
                    break;
                }
            }
            
            // 如果没有碰撞，绘制新位置
            if (!collision) {
                drawMissile(lcd, game.missile);
            } else {
                game.missile.active = false;
            }
        }
    }
    
    // 更新圆球位置
    for (uint8_t i = 0; i < game.matrix_size * game.matrix_size; i++) {
        if (game.blocks[i].active) {
            // 清除旧位置
            clearBlock(lcd, game.blocks[i]);
            
            // 更新位置
            game.blocks[i].pos.x += game.blocks[i].speed * game.blocks[i].direction;
            
            // 检查边界碰撞
            uint8_t size = game.blocks[i].size * BLOCK_SIZE;
            if (game.blocks[i].pos.x <= 0 || game.blocks[i].pos.x + size >= SCREEN_WIDTH) {
                game.blocks[i].direction *= -1;  // 反向
                game.blocks[i].pos.x = std::max<int16_t>(0, std::min<int16_t>(SCREEN_WIDTH - size, game.blocks[i].pos.x));
            }
            
            // 绘制新位置
            drawBlock(lcd, game.blocks[i]);
        }
    }
    
    // 更新爆炸效果
    if (game.explosion.active) {
        clearExplosion(lcd, old_explosion);
        game.explosion.frame++;
        if (game.explosion.frame >= 5) {
            game.explosion.active = false;
        } else {
            drawExplosion(lcd, game.explosion);
        }
    }
    
    // 检查游戏是否结束
    if (game.game_over) {
        lcd.drawString(SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT/2, 
                      "Game Over!", TEXT_COLOR, BG_COLOR, 2);
        lcd.drawString(SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT/2 + 30, 
                      "Press MID to restart", TEXT_COLOR, BG_COLOR, 2);
    }
}

int main() {
    stdio_init_all();
    printf("Little Bees Game\n");
    
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
    lcd.drawString(0, 10, "Press MID BTN start", TEXT_COLOR, BG_COLOR, 2);
    
    // 等待用户按下中间键
    bool started = false;
    while (!started) {
        if (joystick.get_button_value() == 0) {  // 0表示按下
            started = true;
            lcd.clearScreen(BG_COLOR);
            sleep_ms(200);  // 消抖
        }
        sleep_ms(JOYSTICK_LOOP_DELAY_MS);
    }
    
    // 初始化游戏状态
    GameState game;
    initGame(game);
    
    // 绘制初始游戏元素
    for (uint8_t i = 0; i < game.matrix_size * game.matrix_size; i++) {
        drawBlock(lcd, game.blocks[i]);
    }
    drawSpaceship(lcd, game.spaceship);
    drawScore(lcd, game.score);
    
    // 主循环
    while (true) {
        // 获取摇杆方向
        uint16_t adc_x = 0, adc_y = 0;
        int16_t offset_x = 0, offset_y = 0;
        
        // 读取摇杆数据
        joystick.get_joy_adc_16bits_value_xy(&adc_x, &adc_y);
        offset_x = joystick.get_joy_adc_12bits_offset_value_x();
        offset_y = joystick.get_joy_adc_12bits_offset_value_y();
        
        int direction = determine_joystick_direction(offset_x, offset_y);
        
        // 获取按键状态
        bool fire = (joystick.get_button_value() == 0);
        
        // 更新游戏状态
        updateGame(game, direction, fire, lcd);
        
        // 如果游戏结束，等待重新开始
        if (game.game_over && fire) {
            lcd.clearScreen(BG_COLOR);
            game.matrix_size = 1;  // 重置矩阵大小
            initGame(game);
            // 重新绘制所有游戏元素
            for (uint8_t i = 0; i < game.matrix_size * game.matrix_size; i++) {
                drawBlock(lcd, game.blocks[i]);
            }
            drawSpaceship(lcd, game.spaceship);
            drawScore(lcd, game.score);
        }
        
        sleep_ms(16);  // 约60FPS
    }
    
    return 0;
} 