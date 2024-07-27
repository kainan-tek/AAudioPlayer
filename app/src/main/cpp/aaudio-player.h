//
// Created by kaina on 2024/7/21.
//

#ifndef AAUDIOPLAYER_AAUDIO_PLAYER_H
#define AAUDIOPLAYER_AAUDIO_PLAYER_H

#include <aaudio/AAudio.h>

#define ENABLE_CALLBACK // AAudio callback
#define USE_WAV_HEADER
// #define LATENCY_TEST     // latency test with gpio

#ifdef LATENCY_TEST
#define WRITE_CYCLE 100
#define GPIO_FILE "/sys/class/gpio/gpio376/value"
#define GPIO_VAL_H "1"
#define GPIO_VAL_L "0"
#endif // LATENCY_TEST

class AAudioPlayer
{
private:
    aaudio_usage_t usage;
    aaudio_content_type_t content;
    int32_t sampleRate;
    int32_t channelCount;
    aaudio_format_t format;
    int32_t framesPerBurst;
    int32_t numOfBursts;
    aaudio_direction_t direction;
    aaudio_sharing_mode_t sharingMode;
    aaudio_performance_mode_t performanceMode;

    bool isPlaying;
    AAudioStream *aaudioStream;
    std::string audioFile;

    static int32_t bytesPerFrame;
    static std::ifstream inputFile;

#ifdef ENABLE_CALLBACK
    static aaudio_data_callback_result_t dataCallback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames);
    static void errorCallback(AAudioStream *stream, void *userData, aaudio_result_t error);
#endif

#ifdef LATENCY_TEST
    static int32_t cycle;
    static int32_t invert_flag;
    static void gpio_set_high();
    static void gpio_set_low();
#endif

public:
    AAudioPlayer();
    ~AAudioPlayer();

    bool startAAudioPlayback();
    bool stopAAudioPlayback();
    void stopPlayback();
};
#endif // AAUDIOPLAYER_AAUDIO_PLAYER_H
