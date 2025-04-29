# M5Stack Unit Joystick2 驱动程序

这是一个用于控制M5Stack Unit Joystick2模块的驱动程序，支持Raspberry Pi Pico平台。该驱动程序提供了完整的摇杆控制、按钮检测和状态指示功能。

## 功能特点

- I2C通信控制（400kHz）
- 12位ADC值读取
- 方向检测（上、下、左、右、中）
- 按钮状态检测
- RGB状态灯控制
- 自动校准功能
- 完整的错误处理

## 硬件要求

- Raspberry Pi Pico
- M5Stack Unit Joystick2模块
- 连接线（SDA、SCL、VCC、GND）

## 引脚连接

| Pico引脚 | Joystick2引脚 | 说明 |
|----------|--------------|------|
| GPIO6    | SDA          | I2C数据线 |
| GPIO7    | SCL          | I2C时钟线 |
| 3V3      | VCC          | 电源正极 |
| GND      | GND          | 电源负极 |

## 快速开始

1. 初始化设备
```c
joystick2_init(i2c0, 6, 7);  // 使用I2C0，SDA=6，SCL=7
```

2. 主循环示例
```c
while (true) {
    // 更新状态（按钮检测和灯控制）
    joystick2_update();
    
    // 读取摇杆数据
    uint16_t x, y;
    if (joystick2_read_xy(&x, &y) == JOYSTICK2_OK) {
        // 处理摇杆数据
    }
    
    sleep_ms(10);
}
```

## 状态灯说明

- 绿灯常亮：设备正常工作
- 红灯常亮：设备未找到或通信错误
- 绿灯闪烁：按钮按下（50ms亮，200ms灭）

## API参考

### 初始化
```c
void joystick2_init(i2c_inst_t *i2c_instance, uint sda_pin, uint scl_pin);
```

### 数据读取
```c
joystick2_error_t joystick2_read_xy(uint16_t *x, uint16_t *y);
uint8_t joystick2_get_button(void);
```

### 状态控制
```c
void joystick2_set_rgb(uint32_t color);
void joystick2_update(void);
```

### 校准功能
```c
joystick2_error_t joystick2_get_calibration(joystick2_calibration_t *cal);
joystick2_error_t joystick2_set_calibration(const joystick2_calibration_t *cal);
joystick2_error_t joystick2_calibrate(void);
```

### 版本信息
```c
uint8_t joystick2_get_firmware_version(void);
uint8_t joystick2_get_bootloader_version(void);
```

## 错误处理

驱动程序使用以下错误码：
```c
typedef enum {
    JOYSTICK2_OK = 0,                    // 操作成功
    JOYSTICK2_ERROR_I2C = -1,           // I2C通信错误
    JOYSTICK2_ERROR_INVALID_PARAM = -2,  // 参数错误
    JOYSTICK2_ERROR_NOT_INITIALIZED = -3,// 未初始化
    JOYSTICK2_ERROR_CALIBRATION = -4     // 校准错误
} joystick2_error_t;
```

## 校准说明

1. 自动校准
```c
joystick2_error_t ret = joystick2_calibrate();
if (ret == JOYSTICK2_OK) {
    // 校准成功
}
```

2. 手动校准
```c
joystick2_calibration_t cal;
// 设置校准参数
cal.x_min = 0;
cal.x_max = 4095;
cal.y_min = 0;
cal.y_max = 4095;
joystick2_set_calibration(&cal);
```

## 注意事项

1. 初始化后请检查状态灯，确保设备正常工作
2. 建议在使用前进行校准，以获得更准确的数据
3. 方向判断阈值（ADC_THRESHOLD）可根据实际需求调整
4. 确保I2C总线上没有地址冲突（默认地址0x63）

## 示例代码

完整的示例代码请参考`main.c`文件。

## 许可证

MIT License

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。 