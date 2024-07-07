//
// Created by kaina on 2024/4/21.
//

#ifndef AAUDIOPLAYER_LOG_H
#define AAUDIOPLAYER_LOG_H

#include <android/log.h>

#define LOG_TAG "AAudioPlayer"
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#define ENABLE_CALLBACK 1 // AAudio callback
#define USE_WAV_HEADER 1

// #define LATENCY_TEST 1  // latency test with gpio
#ifdef LATENCY_TEST
#define WRITE_CYCLE 100
#define GPIO_FILE "/sys/class/gpio/gpio376/value"
#define GPIO_VAL_H "1"
#define GPIO_VAL_L "0"

void gpio_set_low(void);
void gpio_set_high(void);
#endif // LATENCY_TEST

#endif // AAUDIOPLAYER_LOG_H
