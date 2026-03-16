//                            USER DEFINED SETTINGS
#define USER_SETUP_INFO "User_Setup"

// ##################################################################################
// Section 1. 驱动配置（仅保留驱动/分辨率/颜色顺序，移除所有引脚定义）
// ##################################################################################
#define ST7789_2_DRIVER      // 仅启用ST7789驱动
#define TFT_RGB_ORDER TFT_RGB  // 颜色顺序（根据屏幕调整）
// #define TFT_INVERSION_OFF

#define TFT_WIDTH  240     // ST7789 240x320
#define TFT_HEIGHT 320
#define XPT2046_TOUCH       // 声明使用XPT2046触摸芯片

// // 触摸校准（先保留默认，后续校准后修改）
// #define TOUCH_MIN_X 150
// #define TOUCH_MAX_X 3800
// #define TOUCH_MIN_Y 150
// #define TOUCH_MAX_Y 3700
// #define TOUCH_SWAP_XY 0     // 交换XY轴（根据屏幕方向调整）

// 替换为你实际的校准值（对应 calData 数组：281, 3425, 219, 3439, 0）
// #define TOUCH_MIN_X  281    // 对应 calData[0]
// #define TOUCH_MAX_X  3425   // 对应 calData[1]
// #define TOUCH_MIN_Y  219    // 对应 calData[2]
// #define TOUCH_MAX_Y  3439   // 对应 calData[3]
// #define TOUCH_SWAP_XY 0     // 对应 calData[4]（0=不交换XY轴，1=交换）


// ##################################################################################
// Section 2. 引脚配置（必须放在这里！）
// ##################################################################################
// TFT屏幕SPI引脚

// // c3
// #define TFT_MISO 10
// #define TFT_MOSI 3
// #define TFT_SCLK 2
// #define TFT_CS    7  // Chip select
// #define TFT_DC    4  // Data Command
// #define TFT_RST   5  // Reset
// #define TFT_BL    13 // 背光引脚

// // 触摸芯片引脚
// #define TOUCH_CS  6  // XPT2046 CS引脚
// #define TOUCH_IRQ -1 // 若无IRQ引脚则设为-1，有则改为实际引脚（如8）

// // SPI频率配置（仅保留一处）
// #define SPI_FREQUENCY  27000000    // TFT SPI频率（ST7789推荐27MHz）
// #define SPI_READ_FREQUENCY  20000000
// #define SPI_TOUCH_FREQUENCY  2500000  // XPT2046标准频率（必须2.5MHz）

// // 禁用自动引脚分配（针对非标准引脚）
// #define TFT_MISO_PIN 11
// #define TFT_MOSI_PIN 3
// #define TFT_SCLK_PIN 2

// s3
#define TFT_MISO 3
#define TFT_MOSI 7 // In some display driver board, it might be written as "SDA" and so on.
#define TFT_SCLK 15
#define TFT_CS   4  // Chip select control pin
#define TFT_DC   6  // Data Command control pin
#define TFT_RST  5  // Reset pin (could connect to Arduino RESET pin)
#define TFT_BL   16           // LED back-light control pin
#define TFT_BACKLIGHT_ON HIGH  // Level to turn ON back-light (HIGH or LOW)



// 触摸芯片引脚
#define TOUCH_CS  8  // XPT2046 CS引脚
#define TOUCH_IRQ 46 // 若无IRQ引脚则设为-1，有则改为实际引脚（如8）





// ##################################################################################
// Section 3. 字体配置（保留原配置）
// ##################################################################################
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY  40000000

// Optional reduced SPI frequency for reading TFT
#define SPI_READ_FREQUENCY  20000000

// The XPT2046 requires a lower SPI clock rate of 2.5MHz so we define that here:
#define SPI_TOUCH_FREQUENCY  2500000

