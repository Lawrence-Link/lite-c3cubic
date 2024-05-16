#ifndef _IMU___
#define _IMU___

enum class IMU_Motion {NO_MOTION, LEFT, RIGHT, OK};

struct key_struct_typedef{
    uint32_t lv_key_type;
    bool isKeyPressed;
};

extern key_struct_typedef key_status;

void MPU_Init();
void acquireMotion(void *pv);

#endif