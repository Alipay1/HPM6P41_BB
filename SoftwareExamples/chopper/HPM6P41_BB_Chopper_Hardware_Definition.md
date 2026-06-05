# HPM6P41_BB Chopper 硬件功能定义

本文档基于当前 `chopper` 四开关 Buck-Boost 例程整理硬件功能定义，用作后续软件控制、闭环调节、保护逻辑和调试观测的硬件映射基准。

信息来源优先级：

1. 当前例程入口与调用关系：`SoftwareExamples/chopper/src/adc.c`
2. 实际构建引用的板级目录：`SoftwareExamples/board/hpm6p41_bb`
3. 构建产物中记录的工程、板卡与源文件引用信息

## 1. 工程与目标板

| 项目 | 当前定义 |
| --- | --- |
| 应用工程 | `chopper` |
| 目标板 | `hpm6p41_bb` |
| MCU/板卡 | HPM6P41_BB |
| 构建类型 | `flash_xip` |
| 本地化 SDK 目录 | `SoftwareExamples/chopper/hpm_sdk_localized_for_hpm6p41_bb` |
| 板级目录 | `SoftwareExamples/board/hpm6p41_bb` |
| 应用源码 | `src/adc.c`, `src/cdc_acm.c` |

当前 `CMakeLists.txt` 将 `BOARD` 固定为 `hpm6p41_bb`，并使用本地化 SDK。构建缓存显示板级文件来自同级 `SoftwareExamples/board/hpm6p41_bb`，其中包含 `board.c`、`pinmux.c`、`drivers/pwmv2`、`drivers/adc`、`drivers/cp`、`drivers/vofa` 等。

## 2. 当前启动流程

`src/adc.c` 的 `main()` 当前按如下顺序初始化：

| 顺序 | 调用 | 硬件作用 |
| --- | --- | --- |
| 1 | `board_init()` | 初始化系统时钟、console、PMP/PMA 等板级基础设施 |
| 2 | `board_init_usb((USB_Type *)HPM_USB0_BASE)` | 初始化 USB0 引脚与时钟 |
| 3 | `cdc_acm_init(0, HPM_USB0_BASE)` | 初始化 USB CDC ACM 设备 |
| 4 | `vofa_init_frame()` | 初始化 VOFA 数据帧缓冲 |
| 5 | `board_init_led_pins()` | 初始化板载 LED GPIO |
| 6 | `board_init_btn()` | 初始化板载按键 GPIO 与中断 |
| 7 | `board_timer_create(200, board_led0_toggle)` | 使用 GPTMR 周期翻转 LED0 |
| 8 | `board_init_pwm()` | 初始化四开关 PWM 引脚 |
| 9 | `board_pwmv2_start_out()` | 配置并启动 PWM1 输出框架 |
| 10 | `board_pwm_set_duty_phase(0.5, 0, 90)` | 设置初始占空比与相位 |
| 11 | `board_adc_init()` | 初始化 ADC0/ADC1 与 PWM 触发采样链路 |

主循环中调用 `send_something()`，等待 ADC0 转换完成标志，然后通过 VOFA/USB CDC 发送采样数据。

当前主程序中以下两项被注释，电荷泵/栅极驱动辅助 PWM 未实际启用：

```c
//init_gdrive_cp_pins();
//board_charge_pump_init();
```

## 3. 四开关功率级 PWM

### 3.1 功率开关引脚

四个功率开关的引脚定义来自 `pinmux.c`：

| 开关 | Pad | 复用功能 | PWM 外设 | 说明 |
| --- | --- | --- | --- | --- |
| S1 | `PA12` | `PWM1_P_4` | `HPM_PWM1` channel 4 | 左桥臂/功率级开关 1 |
| S2 | `PA13` | `PWM1_P_5` | `HPM_PWM1` channel 5 | 左桥臂/功率级开关 2 |
| S3 | `PA14` | `PWM1_P_6` | `HPM_PWM1` channel 6 | 右桥臂/功率级开关 3 |
| S4 | `PA15` | `PWM1_P_7` | `HPM_PWM1` channel 7 | 右桥臂/功率级开关 4 |

`init_pwm_pins()` 将 `PA12..PA15` 设置为 `PWM1_P_4..PWM1_P_7`，并配置为较强驱动能力和快速边沿。

### 3.2 PWM 基本参数

| 项目 | 当前值 |
| --- | --- |
| PWM 外设 | `HPM_PWM1` |
| PWM 时钟来源 | `clock_ahb0` |
| 初始周期 | `5 us` |
| reload 计算 | `clock_ahb0 / 1000 / 1000 * period_us - 1` |
| 使用计数器 | `pwm_counter_2`, `pwm_counter_3` |
| 使用通道 | `pwm_channel_4..pwm_channel_7` |
| 启动函数 | `board_pwmv2_start_out()` |
| 更新函数 | `board_pwm_set_duty_phase(double dutyh1, double dutyh2, double phase)` |

`board_pwmv2_start_out()` 默认调用 `board_pwm_calc_reload(5)`，即功率级 PWM 周期为 5 us，等效频率约 200 kHz。

### 3.3 占空比与相位

当前例程初始定义：

```c
#define H1 0.5
#define H2 0
board_pwm_set_duty_phase(H1, H2, H1 * 180);
```

因此初始值为：

| 参数 | 当前初始值 | 作用 |
| --- | --- | --- |
| `dutyh1` | `0.5` | 写入 PWM1 `CMP2`，影响第一组桥臂 PWM |
| `dutyh2` | `0` | 写入 PWM1 `CMP4`，影响第二组桥臂 PWM |
| `phase` | `90 deg` | 写入 PWM1 `PHASE` shadow，用于第二计数器相位偏移计算 |

`board_pwm_set_duty_phase()` 同时更新两个 ADC 触发比较值：

| Shadow | 当前计算 |
| --- | --- |
| `ADC_TRG1` | `reload * ((1 - dutyh1) * 0.5 + dutyh1)` |
| `ADC_TRG2` | `reload * ((1 - dutyh2) * 0.5 + dutyh2)` |

### 3.4 互补与死区

PWM1 当前启用成对模式：

| 通道 | 当前配置 |
| --- | --- |
| channel 4 | pair mode, dead area `0x4F` |
| channel 5 | pair mode, dead area `0x4F` |
| channel 6 | pair mode, dead area `0x7` |
| channel 7 | pair mode, dead area `0x7` |

`board_pwmv2_start_out()` 对 channel 4/5 和 channel 6/7 均调用 `pwmv2_enable_pair_mode()`，并对 channel 4、channel 6 调用 `pwmv2_disable_four_cmp()`。channel 6/7 的输出反相配置目前仍为注释状态。

## 4. ADC 采样定义

### 4.1 ADC 引脚与通道

ADC 模拟输入引脚来自 `pinmux.c` 和 `board_adc.c`：

| 信号名 | Pad | ADC 外设 | ADC 通道 | PMT 队列顺序 | 采样含义 |
| --- | --- | --- | --- | --- | --- |
| `INPUT_V` | `PD06` | `HPM_ADC1` | ch8 | ADC1 queue[1] | 输入电压 |
| `INPUT_I` | `PD07` | `HPM_ADC1` | ch9 | ADC1 queue[0] | 输入电流 |
| `OUTPUT_I` | `PD08` | `HPM_ADC0` | ch6 | ADC0 queue[0] | 输出电流 |
| `OUTPUT_V` | `PD09` | `HPM_ADC0` | ch7 | ADC0 queue[1] | 输出电压 |

当前通道宏：

```c
#define CHANNEL_INPUT_V_SENSE 8
#define CHANNEL_INPUT_I_SENSE 9
#define CHANNEL_OUTPUT_I_SENSE 6
#define CHANNEL_OUTPUT_V_SENSE 7
```

### 4.2 ADC 外设配置

| 项目 | 当前配置 |
| --- | --- |
| ADC 外设 | `HPM_ADC0`, `HPM_ADC1` |
| 分辨率 | 16 bit |
| 转换模式 | `adc16_conv_mode_preemption` |
| ADC clock source | `clk_adc_src_ahb0` |
| ADC clock divider | `adc16_clock_divider_4` |
| AHB sync | enabled |
| PMT trigger | `ADC16_CONFIG_TRG0A` |
| 每次触发长度 | 2 channel |
| PMT DMA | 缓冲区已定义，DMA 初始化当前注释 |
| ADC 中断 | ISR 已定义，但 `intc_m_enable_irq...` 和 `adc16_enable_interrupts...` 当前注释 |

`board_adc_init()` 中定义了 ADC0/ADC1 PMT DMA 缓冲区和转换完成标志，但当前没有启用 ADC PMT DMA 和 ADC 中断。`src/adc.c` 主循环等待 `get_adc0_conv_stat()`，该标志目前依赖 ADC0 ISR 设置，因此后续需要确认实际工程中是否还有其他未检索到的启用路径。

### 4.3 ADC 换算公式

现有 `board_adc.c` 提供如下换算接口：

| 接口 | 计算公式 | 说明 |
| --- | --- | --- |
| `get_input_current()` | `((ADC1[0] / 65535 * 3.0 - 1.5) / 50 / 0.001)` | 输入电流，1.5 V 偏置，50 倍增益，1 mOhm 采样电阻 |
| `get_input_voltage()` | `ADC1[1] / 65535 * 3.0 / 5.1 * (120 + 5.1)` | 输入电压，120 kOhm / 5.1 kOhm 分压 |
| `get_output_current()` | `((ADC0[0] / 65535 * 3.0 - 1.5) / 50 / 0.001)` | 输出电流，1.5 V 偏置，50 倍增益，1 mOhm 采样电阻 |
| `get_output_voltage()` | `ADC0[1] / 65535 * 3.0 / 5.1 * (120 + 5.1)` | 输出电压，120 kOhm / 5.1 kOhm 分压 |

换算参数提取：

| 参数 | 当前值 |
| --- | --- |
| ADC 满量程计数 | `UINT16_MAX` |
| ADC 参考电压/量程 | `3.0 V` |
| 电流零点偏置 | `1.5 V` |
| 电流放大倍数 | `50` |
| 电流采样电阻 | `0.001 Ohm` |
| 电压上拉分压电阻 | `120 kOhm` |
| 电压下拉分压电阻 | `5.1 kOhm` |

## 5. PWM-TRGM-ADC 触发链

当前 PWM 和 ADC 之间通过 TRGM0 建立触发链路。

### 5.1 PWM 到 ADC 的触发输出

`board_pwmv2_start_out()` 中：

| PWM trigger out | 比较源 |
| --- | --- |
| PWM1 TRGO 0 | `PWMV2_CMP_INDEX(10)` / `ADC_TRG1` |
| PWM1 TRGO 1 | `PWMV2_CMP_INDEX(14)` / `ADC_TRG2` |

`board_adc_init()` 中：

| TRGM 输入 | TRGM 输出 |
| --- | --- |
| `HPM_TRGM0_INPUT_SRC_PWM1_TRGO_0` | `HPM_TRGM0_OUTPUT_SRC_ADCX_PTRGI0A` |
| `HPM_TRGM0_INPUT_SRC_PWM1_TRGO_1` | `HPM_TRGM0_OUTPUT_SRC_ADCX_PTRGI0A` |

注意：当前代码对同一个 `HPM_TRGM0_OUTPUT_SRC_ADCX_PTRGI0A` 连续配置了两次，第二次配置会覆盖还是合并取决于 TRGM 外设/驱动行为，后续做同步采样策略时需要重点确认。

### 5.2 TRGM 调试输出

`board_pwmv2_start_out()` 末尾将 `PB00/PB01` 配为 TRGM 输出：

| Pad | 功能 | 当前输入源 |
| --- | --- | --- |
| `PB00` | `TRGM_P_31` | `HPM_TRGM0_INPUT_SRC_SYNT_CH00` |
| `PB01` | `TRGM_P_30` | `HPM_TRGM0_INPUT_SRC_SYNT_CH00` |

文件中也保留了将 `PB00/PB01` 输出 `PWM1_TRGO_0/1` 的注释代码，可用于后续示波器观测 ADC 触发点。

## 6. 电荷泵与栅极驱动辅助

### 6.1 引脚定义

`init_gdrive_cp_pins()` 定义如下引脚：

| 信号 | Pad | 功能 | 初始/用途 |
| --- | --- | --- | --- |
| `CPENP` | `PA16` | `GPIO_A_16` | GPIO 输出，写 0 |
| `CPENN` | `PA18` | `GPIO_A_18` | GPIO 输出，写 0 |
| `CP_INP` | `PA17` | `PWM2_P_1` | 电荷泵 PWM 输入 |
| `CP_INN` | `PA19` | `PWM2_P_3` | 电荷泵 PWM 输入 |

主程序当前未调用 `init_gdrive_cp_pins()`，因此这些引脚是否进入上述状态取决于其他初始化路径。

### 6.2 PWM2 电荷泵

`board_charge_pump_init()` 使用 `HPM_PWM2`：

| 项目 | 当前值 |
| --- | --- |
| PWM 外设 | `HPM_PWM2` |
| 周期 | `10 us` |
| 使用计数器 | `pwm_counter_0`, `pwm_counter_1` |
| 输出通道 | `pwm_channel_1`, `pwm_channel_3` |
| 初始比较值 | `0`, `0.125 * reload`, `0.125 * reload`, `0.250 * reload` |
| 死区 | 相关配置当前注释 |

当前主程序注释掉了 `board_charge_pump_init()`，所以该辅助 PWM 在本例程默认流程中未启动。

## 7. 通信与观测

### 7.1 USB CDC ACM

| 项目 | 当前定义 |
| --- | --- |
| USB 外设 | `HPM_USB0` |
| USB 引脚 | `PD00` = USB0_P, `PD01` = USB0_N |
| IRQ | `IRQn_USB0`, priority 1 |
| Device class | CDC ACM |
| IN endpoint | `0x81` |
| OUT endpoint | `0x01` |
| INT endpoint | `0x83` |
| VID/PID | 来自 `usb_config.h` 的 `USBD_VID`, `USBD_PID` |

`src/cdc_acm.c` 实现 CDC 描述符、端点回调和 DTR 状态。

### 7.2 VOFA 数据帧

VOFA 数据结构：

```c
#define VOFA_CH_CNT 32
typedef struct {
  float fdata[VOFA_CH_CNT];
  unsigned char tail[4];
} vofa_frame;
```

尾部固定为 `00 00 80 7F`。当前使用双缓冲 `cdc_frame_0/cdc_frame_1`，缓冲满后通过 USB CDC IN endpoint `0x81` 发送。

`src/adc.c` 当前写入前 4 路：

| VOFA 通道 | 当前写入 | 注释含义 |
| --- | --- | --- |
| `fdata[0]` | `get_vin()` | Vin |
| `fdata[1]` | `get_vout()` | Vout |
| `fdata[2]` | `get_ipeak()` | peak |
| `fdata[3]` | `get_iavg()` | avg |

接口不一致项：当前可检索到的 ADC 换算接口是 `get_input_current()`、`get_input_voltage()`、`get_output_current()`、`get_output_voltage()`；但 `src/adc.c` 调用的是 `get_vin()`、`get_vout()`、`get_ipeak()`、`get_iavg()`。后续需要确认这些函数是否来自未纳入索引的文件、旧接口残留，或需要做接口重命名/适配。

## 8. 板载辅助 IO

### 8.1 当前例程已使用

| 功能 | Pad/外设 | 定义 |
| --- | --- | --- |
| LED0 | `PY00`, `GPIOY_00` | `BOARD_LED0_GPIO_PIN 0`，低电平点亮 |
| LED1 | `PY01`, `GPIOY_01` | `BOARD_LED1_GPIO_PIN 1`，低电平点亮 |
| LED2 | `PY02`, `GPIOY_02` | `BOARD_LED2_GPIO_PIN 2`，低电平点亮 |
| BTN1 | `PD21`, `GPIOD_21` | falling edge interrupt，按下值 0 |
| BTN2 | `PD20`, `GPIOD_20` | falling edge interrupt，按下值 0 |
| 周期定时器 | `GPTMR0` channel 1 | `board_timer_create()` 用于 LED0 200 ms 翻转 |
| Console UART | `UART2` | 115200 bps；本工程定义了 `CONFIG_NDEBUG_CONSOLE=1` |

### 8.2 板级预留/其他驱动定义

| 功能 | Pad/外设 | 说明 |
| --- | --- | --- |
| JTAG | `PA05 TDI`, `PA04 TDO`, `PA06 TCK`, `PA07 TMS` | 调试接口 |
| CAN | `PB14 MCAN1_RXD`, `PB15 MCAN1_TXD`, `PC13 MCAN1_STBY` | 板级 `BOARD_APP_CAN_BASE HPM_MCAN1` |
| I2C/EEPROM | `PD24 I2C0_SCL`, `PD25 I2C0_SDA`, `PD22 EEPROM_nWC` | 板级 I2C/EEPROM 预留 |
| LCD SPI | `SPI0`, `PB13 MOSI`, `PB04 SCLK`, `PB03 CS3` | ST7789 驱动目录存在 |
| LCD 控制 | `PD27 LCD_DC`, `PD26 LCD_RES`, `PA00 LCD_BKL` | LCD GPIO/背光 |
| Buzzer | `PY03 PTMR_COMP_3` | pinmux 中预留 |
| 温度告警 | `PD23 TSEN_ALERT` | GPIO 输出初始化为 0 |

## 9. 现有硬件相关接口

本轮不新增或修改 C API。当前文档记录以下现有接口：

| 接口 | 文件 | 作用 |
| --- | --- | --- |
| `board_init_pwm()` | `drivers/pwmv2/board_pwmv2.c` | 初始化四开关 PWM 引脚 |
| `board_pwmv2_start_out()` | `drivers/pwmv2/board_pwmv2.c` | 配置并启动 PWM1 输出 |
| `board_pwm_set_duty_phase()` | `drivers/pwmv2/board_pwmv2.c` | 更新两组占空比、相位和 ADC 触发点 |
| `board_adc_init()` | `drivers/adc/board_adc.c` | 初始化 ADC0/ADC1 与 TRGM 触发 |
| `get_input_current()` | `drivers/adc/board_adc.c` | 获取换算后的输入电流 |
| `get_input_voltage()` | `drivers/adc/board_adc.c` | 获取换算后的输入电压 |
| `get_output_current()` | `drivers/adc/board_adc.c` | 获取换算后的输出电流 |
| `get_output_voltage()` | `drivers/adc/board_adc.c` | 获取换算后的输出电压 |
| `board_charge_pump_init()` | `drivers/cp/board_cp.c` | 配置并启动 PWM2 电荷泵输出，当前主流程未调用 |
| `vofa_send_frame()` | `drivers/vofa/vofa.c` | 通过 USB CDC 发送 VOFA 帧 |

## 10. 后续待确认项

| 项目 | 当前观察 | 建议 |
| --- | --- | --- |
| VOFA 数据源接口 | `src/adc.c` 调用 `get_vin/get_vout/get_ipeak/get_iavg`，但当前可检索源码未定义 | 确认是否需要改为 `get_input_voltage/get_output_voltage/...`，或补充兼容接口 |
| ADC 完成标志 | `send_something()` 等待 `get_adc0_conv_stat()`，但 ADC0 中断启用代码当前注释 | 确认实际运行时 ADC 完成标志来源 |
| TRGM ADC 触发 | `ADCX_PTRGI0A` 被 PWM1 TRGO0/1 连续配置两次 | 明确期望是双触发、单触发还是分别触发 ADC0/ADC1 |
| 电荷泵启用 | `init_gdrive_cp_pins()` 和 `board_charge_pump_init()` 当前注释 | 上电顺序和栅极驱动供电策略确定后再启用 |
| 右桥臂反相 | channel 6/7 输出反相代码当前注释 | 结合实际驱动器输入极性确认是否需要启用 |
