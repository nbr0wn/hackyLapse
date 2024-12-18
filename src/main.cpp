#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "iot_button.h"

// Dongle button config
button_config_t button_config = {
  .type = BUTTON_TYPE_GPIO,
  .long_press_time = 2000,
  .short_press_time = 20,
  .gpio_button_config = { 
    .gpio_num = 0,
    .active_level = 0
   }
};
button_handle_t button_handle;

#include "sdkconfig.h"

// SD-MMC interface using esp-idf:
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

// RGB and button drivers from https://github.com/UncleRus/esp-idf-lib.git
//#include "esp_idf_lib_helpers.h"
//#include "button_gpio.h"
//#include "led_strip_spi.h"

// Display driver from https://github.com/lovyan03/LovyanGFX.git
#include "LovyanGFX.hpp"
#include "wifi-captive-portal-esp-idf.h"

#include "control.h"

#define TAG "dongle"

extern "C" void ble_task(void *);

// GPIO configuration

// Button hardware configuration:
#define BUTTON_GPIO      0

// RGB led hardware configuration:
#define RGBLED_CI       39
#define RGBLED_DI       40
#define LEDSTRIP_LEN     1

// Display (ST7735s) hardware configuration:
#define DISPLAY_RST      1
#define DISPLAY_DC       2
#define DISPLAY_MOSI     3
#define DISPLAY_CS       4
#define DISPLAY_SCLK     5
#define DISPLAY_LEDA    38
#define DISPLAY_MISO    -1
#define DISPLAY_BUSY    -1
#define DISPLAY_WIDTH  160
#define DISPLAY_HEIGHT  80

// SD-MMC interface
#define SDMMC_MOUNTPOINT "/sdcard"
#define SDMMC_D0        (gpio_num_t)14
#define SDMMC_D1        (gpio_num_t)17
#define SDMMC_D2        (gpio_num_t)21
#define SDMMC_D3        (gpio_num_t)18
#define SDMMC_CLK       (gpio_num_t)12
#define SDMMC_CMD       (gpio_num_t)16

static void pauseScreen(void);
static void showPairScreen(void);
static void showTitleScreen(void);
static void snapshotScreen(void);
static void showCountdown(void);
static void showMessage(std::string msg);
static void state_timer_handler(void *arg);

const esp_timer_create_args_t state_timer_args = {
  .callback = &state_timer_handler,
  .dispatch_method = ESP_TIMER_TASK,
  .name = "State Timer"};
esp_timer_handle_t state_timer;

int countdown = 30000;
int timeout = 30000;
bool takeSnapshot = false;
#define BG_COLOR 0x000001

enum RunState currentState;

// Used by httpd
extern "C" void addTime(int time) {
  timeout += time;
  if(timeout < 500) {
    timeout = 500;
  }
}

extern "C" void setTime(int time) {
  timeout = time;
}

// Used by httpd
extern "C" int getShutterTimeout(void) {
  return timeout;
}

// Used by httpd
extern "C" void doSnapshot(void) {
  takeSnapshot=true;
}

static void state_timer_handler(void *arg)
{
	switch (getState())
	{
	case STATE_START:
    ble_task(0);
    setState(STATE_UNPAIRED);
		break;
	case STATE_UNPAIRED:
		break;
	case STATE_PAIRED:
    setState(STATE_RUNNING);
    countdown = timeout;
    break;
	case STATE_RUNNING:
    countdown = 0;
		break;
	case STATE_PAUSED:
		break;
	case STATE_SNAPSHOT:
		break;
	case STATE_SNAPSHOT_WAIT:
    setState(STATE_PAUSED);
		break;
	}
}

extern "C" void setState(enum RunState newState) {
  currentState = newState;
  switch (newState) {
    case STATE_START:
      showTitleScreen();
      esp_timer_stop(state_timer);
      ESP_ERROR_CHECK(esp_timer_start_once(state_timer, 2000 * 1000));
      break;
    case STATE_UNPAIRED:
      // Start BLE task
      showPairScreen();
      break;
    case STATE_PAIRED:
      showMessage("CONNECTED");
      esp_timer_stop(state_timer);
      ESP_ERROR_CHECK(esp_timer_start_once(state_timer, 1000 * 1000));
      break;
    case STATE_RUNNING:
      break;
    case STATE_PAUSED:
      pauseScreen();
      break;
    case STATE_SNAPSHOT:
      doSnapshot();
      snapshotScreen();
		  setState(STATE_SNAPSHOT_WAIT);
      break;
    case STATE_SNAPSHOT_WAIT:
      esp_timer_stop(state_timer);
      ESP_ERROR_CHECK(esp_timer_start_once(state_timer, 1000 * 1000));
      break;
  }
}

extern "C" enum RunState getState(void) {
  return currentState;
}

/* ----------------------*/
class LGFX_LiLyGo_TDongleS3 : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7735S _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;

public:
  LGFX_LiLyGo_TDongleS3(void)
  {
    {
      auto cfg = _bus_instance.config();

      cfg.spi_host    = SPI3_HOST;              // SPI2_HOST is in use by the RGB led
      cfg.spi_mode    = 0;                      // Set SPI communication mode (0 ~ 3)
      cfg.freq_write  = 27000000;               // SPI clock when sending (max 80MHz, rounded to 80MHz divided by an integer)
      cfg.freq_read   = 16000000;               // SPI clock when receiving
      cfg.spi_3wire   = true;                   // Set true when receiving on the MOSI pin
      cfg.use_lock    = false;                  // Set true when using transaction lock
      cfg.dma_channel = SPI_DMA_CH_AUTO;        // Set the DMA channel to use (0=not use DMA / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=auto setting)

      cfg.pin_sclk    = DISPLAY_SCLK;           // set SPI SCLK pin number
      cfg.pin_mosi    = DISPLAY_MOSI;           // Set MOSI pin number for SPI
      cfg.pin_miso    = DISPLAY_MISO;           // Set MISO pin for SPI (-1 = disable)
      cfg.pin_dc      = DISPLAY_DC;             // Set SPI D/C pin number (-1 = disable)

     _bus_instance.config (cfg);                // Apply the setting value to the bus.
     _panel_instance.setBus (&_bus_instance);   // Sets the bus to the panel.
    }

    {
      auto cfg = _panel_instance.config();      // Obtain the structure for display panel settings.

      cfg.pin_cs   = DISPLAY_CS;                // Pin number to which CS is connected (-1 = disable)
      cfg.pin_rst  = DISPLAY_RST;               // pin number where RST is connected (-1 = disable)
      cfg.pin_busy = DISPLAY_BUSY ;             // pin number to which BUSY is connected (-1 = disable)

      cfg.panel_width       = DISPLAY_HEIGHT;   // actual displayable width. Note: width/height swapped due to the rotation
      cfg.panel_height      = DISPLAY_WIDTH;    // Actual displayable height Note: width/height swapped due to the rotation
      cfg.offset_x          = 26;               // Panel offset in X direction
      cfg.offset_y          = 1;                // Y direction offset amount of the panel
      cfg.offset_rotation   = 1;                // Rotation direction value offset 0~7 (4~7 are upside down)
      cfg.dummy_read_pixel  = 8;                // Number of bits for dummy read before pixel read
      cfg.dummy_read_bits   = 1;                // Number of dummy read bits before non-pixel data read
      cfg.readable          = true;             // set to true if data can be read
      cfg.invert            = true;
      cfg.rgb_order         = false;
      cfg.dlen_16bit        = false;            // Set to true for panels that transmit data length in 16-bit units with 16-bit parallel or SPI
      cfg.bus_shared        = true;             // If the bus is shared with the SD card, set to true (bus control with drawJpgFile etc.)

      // Please set the following only when the display is shifted with a driver with a variable number of pixels such as ST7735 or ILI9163.
      cfg.memory_width  = 132;                  // Maximum width supported by driver IC
      cfg.memory_height = 160;                  // Maximum height supported by driver IC

      _panel_instance.config(cfg);
    }


    {
      auto cfg = _light_instance.config();

      cfg.pin_bl      = DISPLAY_LEDA;           // pin number to which the backlight is connected
      cfg.invert      = true;                   // true to invert backlight brightness
      cfg.freq        = 12000;                  // Backlight PWM frequency
      cfg.pwm_channel = 7;                      // PWM channel number to use

      _light_instance.config(cfg);
      _panel_instance. setLight (&_light_instance);
    }

    setPanel (&_panel_instance);
  }
};

static LGFX_LiLyGo_TDongleS3 lcd;

void showMessage(std::string msg) {
    lcd.setTextSize(1);
    lcd.clear(BG_COLOR);
    lcd.setFont(&fonts::FreeSans12pt7b);
    lcd.setTextColor(0x00FF00u, BG_COLOR);
    int centerY = (DISPLAY_HEIGHT-lcd.fontHeight()) / 2 + 2;
    lcd.drawCenterString(msg.c_str(), DISPLAY_WIDTH / 2, centerY);
}

void showTitleScreen() {
    lcd.setTextSize(1);
    lcd.clear(BG_COLOR);
    lcd.setFont(&fonts::FreeMonoBoldOblique18pt7b);
    lcd.setTextColor(0x555555, BG_COLOR);
    int yPos = (DISPLAY_HEIGHT/2-lcd.fontHeight()) / 2 + 2;
    lcd.drawCenterString("HACKY", DISPLAY_WIDTH / 2, yPos);
    yPos = DISPLAY_HEIGHT / 2 + (DISPLAY_HEIGHT / 2-lcd.fontHeight()) / 2 + 2;
    lcd.setTextColor(0x8565FFu, BG_COLOR);
    lcd.drawCenterString("LAPSE", DISPLAY_WIDTH / 2, yPos);
}

void showPairScreen() {
    lcd.setTextSize(1);
    lcd.clear(BG_COLOR);
    lcd.setFont(&fonts::FreeSans12pt7b);
    lcd.setTextColor(0x5555FFu, BG_COLOR);
    int yPos = (DISPLAY_HEIGHT/2-lcd.fontHeight()) / 2;
    lcd.drawCenterString("PAIR", DISPLAY_WIDTH / 2, yPos);
    yPos = DISPLAY_HEIGHT / 2 + (DISPLAY_HEIGHT / 2-lcd.fontHeight()) / 2;
    lcd.drawCenterString("BLUETOOTH", DISPLAY_WIDTH / 2, yPos);
}

void showCountdown(void) {
    lcd.clear(BG_COLOR);
    lcd.setFont(&fonts::DejaVu72);
    lcd.setTextColor(0xAAAA99u, BG_COLOR);
    lcd.setTextSize(1);
    int centerY = (DISPLAY_HEIGHT-lcd.fontHeight()) / 2 + 6;
    char buf[10];
    sprintf(buf, "%d", countdown / 1000);
    lcd.drawCenterString(buf, DISPLAY_WIDTH / 2, centerY, &fonts::DejaVu72);
}

void pauseScreen(void) {
    lcd.clear(BG_COLOR);
    lcd.setFont(&fonts::FreeSans24pt7b);
    lcd.setTextSize(1);
    lcd.setTextColor(0xFF0000u, BG_COLOR);
    int centerY = (DISPLAY_HEIGHT-lcd.fontHeight()) / 2 + 5;
    lcd.drawCenterString("PAUSE", DISPLAY_WIDTH / 2, centerY);
}

void snapshotScreen(void) {
    lcd.setTextSize(1);
    lcd.clear(BG_COLOR);
    lcd.setFont(&fonts::FreeSans24pt7b);
    lcd.setTextSize(1);
    lcd.setTextColor(0xddddddu, BG_COLOR);
    int centerY = (DISPLAY_HEIGHT-lcd.fontHeight()) / 2 + 5;
    lcd.drawCenterString("SNAP", DISPLAY_WIDTH / 2, centerY);
}

static void button_press_callback(void *button_handle, void *usr_data){
  esp_restart();
}

static void button_held_callback(void *button_handle, void *usr_data){
  //esp_restart();
  //ESP_LOGI(TAG, "HOLD");
}

extern "C" void app_main(void)
{
  ESP_ERROR_CHECK(esp_timer_create(&state_timer_args, &state_timer));

  button_handle = iot_button_create(&button_config);
  button_event_config_t event_cfg = {
    .event = BUTTON_PRESS_DOWN,
    .event_data = { .multiple_clicks = {.clicks = 1 } }
  };
  iot_button_register_event_cb(button_handle, event_cfg, button_press_callback, nullptr);

  event_cfg = {
    .event = BUTTON_LONG_PRESS_START,
    .event_data = { .long_press = { .press_time = 2000 } }
  };
  iot_button_register_event_cb(button_handle, event_cfg, button_held_callback, nullptr);

  // Fire up LCD
  if (!lcd.init()) {
    ESP_LOGW(TAG, "lcd.init() failed");
    return;
  }

  // Set brightness
  lcd.setBrightness(80);

  setState(STATE_START);

  // Start up captive control portal
  wifi_captive_portal_esp_idf();

  vTaskDelay(pdMS_TO_TICKS(1000));

  while (42)
  {
    if(currentState == STATE_RUNNING) {
      showCountdown();
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

#if 0

//
// handler for the button
//
static void on_button(button_t *btn, button_state_t state)
{
  ESP_LOGI(TAG, "button %s",  states[state]);
}

//
// helper functions to generate colors
//
uint32_t led_effect_color_wheel(uint8_t pos)
{
    pos = 255 - pos;
    if (pos < 85)
    {
        return ((uint32_t)(255 - pos * 3) << 16) | ((uint32_t)(0) << 8) | (pos * 3);
    }
    else if (pos < 170)
    {
        pos -= 85;
        return ((uint32_t)(0) << 16) | ((uint32_t)(pos * 3) << 8) | (255 - pos * 3);
    }
    else
    {
        pos -= 170;
        return ((uint32_t)(pos * 3) << 16) | ((uint32_t)(255 - pos * 3) << 8) | (0);
    }
}


rgb_t led_effect_color_wheel_rgb(uint8_t pos)
{
    uint32_t next_color;
    rgb_t next_pixel;

    next_color = led_effect_color_wheel(pos);
    next_pixel.r = (next_color >> 16) & 0xff;
    next_pixel.g = (next_color >>  8) & 0xff;
    next_pixel.b = (next_color      );
    return next_pixel;
}

static esp_err_t rainbow(led_strip_spi_t *strip)
{
    static uint8_t pos = 0;
    esp_err_t err = ESP_FAIL;
    rgb_t color;

    color = led_effect_color_wheel_rgb(pos);

    if ((err = led_strip_spi_fill(strip, 0, strip->length, color)) != ESP_OK) {
        ESP_LOGE(TAG, "led_strip_spi_fill(): %s", esp_err_to_name(err));
        goto fail;
    }
    pos += 1;
fail:
    return err;
}

//
// task for the rgb led
//
void RGBLED(void *pvParam)
{
led_strip_spi_t strip = LED_STRIP_SPI_DEFAULT();
static spi_device_handle_t device_handle;

  strip.mosi_io_num   = RGBLED_DI;
  strip.sclk_io_num   = RGBLED_CI;
  strip.length        = LEDSTRIP_LEN;
  strip.device_handle = device_handle;
  strip.max_transfer_sz = LED_STRIP_SPI_BUFFER_SIZE(LEDSTRIP_LEN);
  strip.clock_speed_hz  = 1000000 * 10; // 10Mhz

  ESP_LOGI(TAG, "Initializing LED strip");
  ESP_ERROR_CHECK(led_strip_spi_init(&strip));
  ESP_ERROR_CHECK(led_strip_spi_flush(&strip));

  while(42)
  {
    ESP_ERROR_CHECK(rainbow(&strip));
    ESP_ERROR_CHECK(led_strip_spi_flush(&strip));
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
void more_init(){
  // esp_err_t err;
  // sdmmc_card_t *card;
  // const char mount_point[] = SDMMC_MOUNTPOINT;
  // DIR *hDir;
  // struct dirent *eDir;
  // RGB Led, APA102 on GPIO39/GPIO40
  //
  err = led_strip_spi_install();
  if (err != ESP_OK)
    ESP_LOGE(TAG, "led_strip_spi_install(): %s", esp_err_to_name(err));

  if (xTaskCreate(RGBLED, "RGB", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL) != pdPASS)
    ESP_LOGE(TAG, "xTaskCreate(): failed");

  //
  // SDMMC
  //
  ESP_LOGI(TAG, "Initializing SD card");

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 16 * 1024,
    .disk_status_check_enable = true
  };

  sdmmc_host_t host = SDMMC_HOST_DEFAULT(); // default slot is SDMMC_HOST_SLOT_1
  sdmmc_slot_config_t slot_config = {
    .clk = SDMMC_CLK,
    .cmd = SDMMC_CMD,
    .d0  = SDMMC_D0,
    .d1  = SDMMC_D1,
    .d2  = SDMMC_D2,
    .d3  = SDMMC_D3,
    .d4  = (gpio_num_t)0,
    .d5  = (gpio_num_t)0,
    .d6  = (gpio_num_t)0,
    .d7  = (gpio_num_t)0,
    .cd  = SDMMC_SLOT_NO_CD,
    .wp  = SDMMC_SLOT_NO_WP,
    .width = 4,
    .flags = 0
  };

  ESP_LOGI(TAG, "Mounting filesystem");
  err = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

  if (err != ESP_OK)
  {
    if (err == ESP_FAIL)
    {
      ESP_LOGI(TAG, "Failed to mount filesystem. "\
                    "If you want the card to be formatted, set the format_if_mount_failed option in mount_config.");
    }
    else
    {
      ESP_LOGI(TAG, "Failed to initialize the card (%s). "\
                    "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(err));
    }
  }
  else
  {
    ESP_LOGI(TAG, "Mounted filesystem");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    hDir = opendir(mount_point);
    if (hDir)
    {
      do
      {
        eDir = readdir(hDir);
        if (eDir)
        {
          ESP_LOGI(TAG, "Dir> %s (%s)", eDir->d_name, (eDir->d_type == DT_UNKNOWN) ?"-unknown-" :  (eDir->d_type == DT_REG)? "<file>": "<dir>");
        }
      }while(eDir);

      closedir(hDir);
    }
  }
}
#endif
