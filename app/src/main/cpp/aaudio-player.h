//
// Created by kaina on 2024/7/21.
//

#ifndef AAUDIOPLAYER_AAUDIO_PLAYER_H
#define AAUDIOPLAYER_AAUDIO_PLAYER_H

#include <aaudio/AAudio.h>

#define ENABLE_CALLBACK // AAudio callback
#define USE_WAV_HEADER
// #define LATENCY_TEST     // latency test with gpio

class AAudioPlayer
{
private:
    aaudio_usage_t m_usage;
    aaudio_content_type_t m_content;
    int32_t m_sampleRate;
    int32_t m_channelCount;
    aaudio_format_t m_format;
    int32_t m_framesPerBurst;
    int32_t m_numOfBursts;
    aaudio_direction_t m_direction;
    aaudio_sharing_mode_t m_sharingMode;
    aaudio_performance_mode_t m_performanceMode;

    bool m_isPlaying;
    AAudioStream *m_aaudioStream;
    std::string m_audioFile;
    std::ifstream m_inputFile;

    void _stopPlayback();
    static int32_t _getBytesPerSample(aaudio_format_t format);

#ifdef ENABLE_CALLBACK
    static aaudio_data_callback_result_t dataCallback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames);
    static void errorCallback(AAudioStream *stream, void *userData, aaudio_result_t error);
#endif

public:
    AAudioPlayer();
    ~AAudioPlayer();

    bool startAAudioPlayback();
    bool stopAAudioPlayback();
};
#endif // AAUDIOPLAYER_AAUDIO_PLAYER_H
