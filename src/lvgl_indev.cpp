#include <lvgl_indev.h>
#include <Arduino.h>

bool isKeyPressed() {
    static bool prevKeyPressed = false;  // 记住上一次的按键状态

    // 当前按键状态
    bool currentKeyPressed = key_status.isKeyPressed;

    // 检查是否有按键从未按下变为按下的过渡
    if (currentKeyPressed && !prevKeyPressed) {
        prevKeyPressed = true;
        return true;
    }

    // 更新上一次的按键状态
    prevKeyPressed = currentKeyPressed;
    return false;
}

void encoder_with_keys_read(lv_indev_drv_t * drv, lv_indev_data_t*data){
    data->key = key_status.lv_key_type; // get which key was recorded previously
   if(isKeyPressed()) { // process by func isKeyPressed to trigger only once
        data->state = LV_INDEV_STATE_PRESSED; 
    } else {
      data->state = LV_INDEV_STATE_RELEASED;
    }
}

lv_indev_t * my_indev;

void LVGL_Indev_Init(){
    static lv_indev_drv_t indev_drv;
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = encoder_with_keys_read;

    my_indev = lv_indev_drv_register(&indev_drv);
}

