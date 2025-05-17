#include <stdio.h>
#include <cstdlib>
#include <random>
#include <cmath>  // 添加这个头文件用于 abs 函数
#include "pico/stdlib.h"
#include "joystick.hpp"
#include "joystick/joystick_config.hpp"
#include "st7789/st7789.hpp"

// 定义屏幕尺寸
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// 定义颜色
#define TEXT_COLOR st7789::WHITE
#define BG_COLOR st7789::BLACK
#define MENU_BORDER_COLOR st7789::WHITE
#define SELECTED_COLOR st7789::WHITE

// 定义菜单项
#define MENU_ITEM_COUNT 2
#define MENU_ITEM_HEIGHT 60
#define MENU_ITEM_WIDTH 200
#define MENU_ITEM_GAP 20
#define MENU_BORDER_WIDTH 4

// 菜单项结构
struct MenuItem {
    const char* title;
    bool selected;
};

// 全局变量
MenuItem menuItems[MENU_ITEM_COUNT] = {
    {"A - CollisionX", false},
    {"B - PicoPilot", false}
};

// 绘制菜单边框
void drawMenuBorder(st7789::ST7789& lcd) {
    // 计算菜单区域
    int menuWidth = MENU_ITEM_WIDTH + (MENU_BORDER_WIDTH * 2);
    int menuHeight = (MENU_ITEM_HEIGHT * MENU_ITEM_COUNT) + 
                    (MENU_ITEM_GAP * (MENU_ITEM_COUNT - 1)) + 
                    (MENU_BORDER_WIDTH * 2);
    int startX = (SCREEN_WIDTH - menuWidth) / 2;
    int startY = (SCREEN_HEIGHT - menuHeight) / 2;

    // 绘制外框
    lcd.drawRect(startX, startY, menuWidth, menuHeight, MENU_BORDER_COLOR);
}

// 绘制菜单项
void drawMenuItem(st7789::ST7789& lcd, const MenuItem& item, int index) {
    int menuWidth = MENU_ITEM_WIDTH + (MENU_BORDER_WIDTH * 2);
    int startX = (SCREEN_WIDTH - menuWidth) / 2 + MENU_BORDER_WIDTH;
    int startY = (SCREEN_HEIGHT - ((MENU_ITEM_HEIGHT * MENU_ITEM_COUNT) + 
                                 (MENU_ITEM_GAP * (MENU_ITEM_COUNT - 1)))) / 2;
    
    int y = startY + (index * (MENU_ITEM_HEIGHT + MENU_ITEM_GAP));
    
    // 根据是否选中决定颜色
    uint16_t current_bg_color = item.selected ? TEXT_COLOR : BG_COLOR;
    uint16_t current_text_color = item.selected ? BG_COLOR : TEXT_COLOR;

    // 绘制背景
    lcd.fillRect(startX, y, MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT, current_bg_color);
    
    // 绘制文本
    lcd.drawString(startX + 10, y + (MENU_ITEM_HEIGHT - 20) / 2, 
                   item.title, current_text_color, current_bg_color, 2);
}

// 绘制所有菜单项
void drawMenu(st7789::ST7789& lcd) {
    // 清屏
    lcd.fillScreen(BG_COLOR);
    lcd.fillScreen(BG_COLOR);  // 添加第二次清屏以确保完全清除
    
    // 绘制边框
    drawMenuBorder(lcd);
    
    // 绘制所有菜单项
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        drawMenuItem(lcd, menuItems[i], i);
    }
}

// 确定摇杆方向
int determine_joystick_direction(int16_t x, int16_t y) {
    int16_t abs_x = std::abs(x);
    int16_t abs_y = std::abs(y);
    
    if (abs_y > abs_x * 1.2) {
        return (y < 0) ? 1 : 2;  // 1=up, 2=down
    }
    
    if (abs_x > abs_y * 1.2) {
        return (x < 0) ? 3 : 4;  // 3=left, 4=right
    }
    
    return 0;  // center
}

// 启动选中的游戏
void launchSelectedGame() {
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        if (menuItems[i].selected) {
            // 这里需要实现游戏启动逻辑
            // 由于Pico不支持直接动态加载程序，这里需要重启并加载不同的程序
            // 实际实现可能需要通过修改启动配置或使用引导程序来实现
            printf("Launching %s...\n", menuItems[i].title);
            break;
        }
    }
}

int main() {
    // 初始化标准库
    stdio_init_all();
    
    // 初始化显示屏
    st7789::ST7789 lcd;
    st7789::Config lcd_config;
    lcd_config.spi_inst = spi0;
    lcd_config.pin_din = 19;    // MOSI
    lcd_config.pin_sck = 18;    // SCK
    lcd_config.pin_cs = 17;     // CS
    lcd_config.pin_dc = 20;     // DC
    lcd_config.pin_reset = 15;  // RESET
    lcd_config.pin_bl = 10;     // 背光
    lcd_config.width = SCREEN_WIDTH;
    lcd_config.height = SCREEN_HEIGHT;
    lcd_config.rotation = st7789::ROTATION_0;
    if (!lcd.begin(lcd_config)) {
        printf("LCD initialization failed!\n");
        return -1;
    }
    lcd.setRotation(st7789::ROTATION_180);
    lcd.clearScreen(BG_COLOR);
    
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
    
    // 初始化菜单选择
    int selectedIndex = 0;
    menuItems[selectedIndex].selected = true;
    
    // 绘制初始菜单
    drawMenu(lcd);
    
    while (true) {
        // 熄灭LED灯
        joystick.set_rgb_color(JOYSTICK_LED_OFF);

        // 读取摇杆状态
        int16_t x = joystick.get_joy_adc_12bits_offset_value_x();
        int16_t y = joystick.get_joy_adc_12bits_offset_value_y();
        bool buttonPressed = (joystick.get_button_value() == 0);
        
        // 处理方向输入
        int direction = determine_joystick_direction(x, y);
        if (direction != 0) { // 如果不是中间位置
            // 根据方向设置灯光
            if (direction == 1) {  // 上
                joystick.set_rgb_color(JOYSTICK_LED_BLUE);
            } else if (direction == 2) {  // 下
                joystick.set_rgb_color(JOYSTICK_LED_BLUE);
            } else if (direction == 3) {  // 左
                joystick.set_rgb_color(JOYSTICK_LED_BLUE);
            } else if (direction == 4) {  // 右
                joystick.set_rgb_color(JOYSTICK_LED_BLUE);
            }

            // 取消当前选择
            menuItems[selectedIndex].selected = false;
            
            // 更新选择
            if (direction == 1) {  // 上
                selectedIndex = (selectedIndex - 1 + MENU_ITEM_COUNT) % MENU_ITEM_COUNT;
            } else if (direction == 2) {  // 下
                selectedIndex = (selectedIndex + 1) % MENU_ITEM_COUNT;
            }
            // 对于左右移动，如果也想让它改变菜单选项，需要在这里添加逻辑
            // 目前的逻辑只处理上下移动来改变菜单选项

            // 设置新选择 (仅当上下移动时，若左右也需更改，则移出此if)
            menuItems[selectedIndex].selected = true;
            
            // 重绘菜单
            drawMenu(lcd);
            
            // 等待摇杆回中
            while (std::abs(joystick.get_joy_adc_12bits_offset_value_x()) > 1000 || std::abs(joystick.get_joy_adc_12bits_offset_value_y()) > 1000) {
                sleep_ms(10);
            }
            joystick.set_rgb_color(JOYSTICK_LED_OFF); // 摇杆回中后熄灭灯
        }
        
        // 处理确认按钮
        if (buttonPressed) {
            joystick.set_rgb_color(JOYSTICK_LED_RED); // 按钮按下时亮红灯
            launchSelectedGame();
            // 等待按钮释放
            while (joystick.get_button_value() == 0) {
                sleep_ms(10);
            }
            joystick.set_rgb_color(JOYSTICK_LED_OFF); // 按钮释放后熄灭灯
        }
        
        sleep_ms(10);
    }
    
    return 0;
} 