//
// Created by kaina on 2024/7/21.
//

#ifndef AAUDIOPLAYER_AAUDIO_PLAYER_H
#define AAUDIOPLAYER_AAUDIO_PLAYER_H

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

#endif //AAUDIOPLAYER_AAUDIO_PLAYER_H
