#include "Arduino.h"

#define LV_LVGL_H_INCLUDE_SIMPLE
#include <lvgl.h>
#include "lvgl_ui\events_init.h"
#include "lvgl_ui\widgets_init.h"
#include "lvgl_ui\gui_guider.h"
#include "lvgl_ui\custom.h"
#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>
#include "WiFi.h"
#include <time.h>
#include <NTPClient.h>
#include "Pin_definition.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "IMU.h"
#include "Wire.h"
#include "lvgl_indev.h"

// #include "SparkFun_APDS9960.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#define SD_CS_CLR() digitalWrite(PIN_SD_CS, HIGH)
#define SD_CS_SET() digitalWrite(PIN_SD_CS, LOW)
static lv_disp_draw_buf_t draw_buf;    // 定义显示器变量
static lv_color_t buf[TFT_WIDTH * 10]; // 定义刷新缓存

TFT_eSPI tft = TFT_eSPI();
lv_ui guider_ui;

void lv_task_handle_timer(void *pvParameters);
void handleGesture(void *pvParameters);
void taskAcquireSensorVal(void *pv);

const char *ssid = "helloworld";
const char *password = "123456789";

SemaphoreHandle_t mutex_update;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.aliyun.com", 28800, 60000);
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// SparkFun_APDS9960 apds = SparkFun_APDS9960();

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();                                        // 使能写功能
  tft.setAddrWindow(area->x1, area->y1, w, h);             // 填充区域
  tft.pushColors((uint16_t *)&color_p->full, w * h, true); // 颜色缓存和缓存大小
  tft.endWrite();                                          // 关闭写功能
  lv_disp_flush_ready(disp);                               // 区域填充颜色函数
}

void setup()
{
  pinMode(8, OUTPUT);
  pinMode(0, OUTPUT);
  analogWrite(8, 255 - 90);
  Serial.begin(115200);

  // Wire.setPins(PIN_I2C_SDA, PIN_I2C_SCL);
  SD_CS_CLR();
  tft.init();         // 初始化
  tft.setRotation(2); // 屏幕旋转方向（横向）
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_WIDTH * 10);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = TFT_WIDTH;
  disp_drv.ver_res = TFT_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
  LVGL_Indev_Init(); // register input devices

  lv_group_t *group = lv_group_create();
  lv_indev_set_group(my_indev, group);
  // lv_group_set_default(group);

  setup_ui(&guider_ui);
  events_init(&guider_ui);
  custom_init(&guider_ui);

  lv_group_add_obj(group, guider_ui.screen);
  // lv_group_add_obj(group, guider_ui.screen_1);

  WiFi.begin(ssid, password);

  mutex_update = xSemaphoreCreateMutex();
  int dot_cnt = 0;

  Wire.setPins(PIN_I2C_SDA, PIN_I2C_SCL);

  MPU_Init();

  xTaskCreate( // enable timer cnt for lvgl task handler
      lv_task_handle_timer,
      "task_handler",
      4096,
      NULL,
      15,
      NULL);

  xTaskCreate( // enable timer cnt for IMU task handler
      acquireMotion,
      "imu_handler",
      2048,
      NULL,
      15,
      NULL);

  // setupt motion detection
  /*
    if (apds.init())
    {
      Serial.println(F("APDS-9960 initialization complete"));
    }
    else
    {
      Serial.println(F("Something went wrong during APDS-9960 init!"));
    }

    // Start running the APDS-9960 gesture sensor engine
    if (apds.enableGestureSensor(true))
    {
      Serial.println(F("Gesture sensor is now running"));
    }
    else
    {
      Serial.println(F("Something went wrong during gesture sensor init!"));
    }
    */

  /*
    xTaskCreate( // gesture sensor
        taskAcquireSensorVal,
        "sensor_val",
        4096,
        NULL,
        10,
        NULL);
  */
  while (WiFi.status() != WL_CONNECTED)
  {
    if (lv_scr_act() == guider_ui.screen)
    {
      lv_label_set_text(guider_ui.screen_label_date, "Connecting WiFi");
      if (dot_cnt == 0)
      {
        lv_label_set_text(guider_ui.screen_label_time, ".");
      }
      else if (dot_cnt == 1)
      {
        lv_label_set_text(guider_ui.screen_label_time, "..");
      }
      else
      {
        lv_label_set_text(guider_ui.screen_label_time, "...");
      }
      ++dot_cnt;
      if (dot_cnt == 3)
        dot_cnt = 0;
      delay(500);
      // Serial.print(".");
    }
  }
  timeClient.begin();
}

void loop()
{
  if (lv_scr_act() == guider_ui.screen)
  {
    if (xSemaphoreTake(mutex_update, portMAX_DELAY) == pdTRUE)
    {
      timeClient.update();
      // Serial.println(timeClient.getFormattedTime());
      lv_label_set_text(guider_ui.screen_label_time, (timeClient.getFormattedTime()).c_str());
      // lv_label_set_text_fmt(guider_ui.screen_label_date, (daysOfTheWeek[timeClient.getDay()]));
      lv_label_set_text(guider_ui.screen_label_date, timeClient.getFormattedDate().c_str());
      xSemaphoreGive(mutex_update);
    }
  }
  delay(1000);
}

void lv_task_handle_timer(void *pvParameters)
{
  for (;;)
  {
    if (xSemaphoreTake(mutex_update, portMAX_DELAY) == pdTRUE)
    {
      lv_timer_handler();
      xSemaphoreGive(mutex_update);
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

/*
void taskAcquireSensorVal(void *pv){
  for (;;) {
  //if(mpu.getMotionInterruptStatus()) {
    if (1){
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    //Serial.print(a.acceleration.x);
    int y_real = map(long(g.acceleration.x), -10, 10, 0, 240);
    int x_real = map(long(g.acceleration.y), -10, 10, 0, 240);

    if (xSemaphoreTake(mutex_update, portMAX_DELAY) == pdTRUE)
    {
      lv_obj_set_pos(guider_ui.screen_led_1, x_real, y_real);
      xSemaphoreGive(mutex_update);
    }
    //Serial.print(a.acceleration.y);
    //Serial.print(a.acceleration.z);
    //Serial.print(g.gyro.x);
    //Serial.print(g.gyro.y);
    //Serial.print(g.gyro.z);
  }
  }
}
*/
/*
void handleGesture(void *pvParameters)
{
  for (;;)
  {
    if (apds.isGestureAvailable())
    {
      switch (apds.readGesture())
      {
      case DIR_UP:
        Serial.println("UP");
        break;
      case DIR_DOWN:
        Serial.println("DOWN");
        break;
      case DIR_LEFT:
        Serial.println("LEFT");
        break;
      case DIR_RIGHT:
        Serial.println("RIGHT");
        break;
      case DIR_NEAR:
        Serial.println("NEAR");
        break;
      case DIR_FAR:
        Serial.println("FAR");
        break;
      default:
        Serial.println("NONE");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
*/
