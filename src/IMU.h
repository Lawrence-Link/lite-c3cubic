#ifndef _IMU__
#define _IMU__

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

enum class IMU_Motion {NO_MOTION, LEFT, RIGHT, OK};

void MPU_Init();
void acquireMotion(void *pv);

#endif