#include <Wire.h>
#include "IMU.h"

Adafruit_MPU6050 mpu;

void MPU_Init()
{
    if (!mpu.begin())
    {
        Serial.println("Failed to find MPU6050 chip");
    }

    mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
    mpu.setMotionDetectionThreshold(1);
    mpu.setMotionDetectionDuration(20);
    mpu.setInterruptPinLatch(true); // Keep it latched.  Will turn off when reinitialized.
    mpu.setInterruptPinPolarity(true);
    mpu.setMotionInterrupt(true);
}


volatile uint8_t anti_jitter_cnt = 0; 

void acquireMotion(void *pv)
{
    sensors_event_t a, g, temp;
    static bool flag_cnt_begin = false;

    for (;;)
    {
        mpu.getEvent(&a, &g, &temp);
        
        Serial.printf("x_acc=%f,y_gyro=%f\r\n", a.acceleration.x, g.gyro.y);

        if (a.acceleration.y >= 3 && g.gyro.x >= 3.8 && !anti_jitter_cnt)
        {
            flag_cnt_begin = true;
            Serial.println("Right!");
        } else if (a.acceleration.y <= -3 && g.gyro.x <= -3.8 && !anti_jitter_cnt)
        {
            flag_cnt_begin = true;
            Serial.println("Left!");
        } else if (0){

        }

        if (flag_cnt_begin && anti_jitter_cnt < 30) {
            anti_jitter_cnt ++;
        } else {
            anti_jitter_cnt = 0;
            flag_cnt_begin = false;
        }
        vTaskDelay(10);
    }
}
