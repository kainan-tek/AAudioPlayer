//
// Created by kaina on 2024/7/21.
//

#ifndef AAUDIOPLAYER_AAUDIO_PLAYER_H
#define AAUDIOPLAYER_AAUDIO_PLAYER_H

#include <aaudio/AAudio.h>
#include "aaudio-buffer.h"

#define ENABLE_CALLBACK
#define USE_WAV_HEADER
// #define LATENCY_TEST

class AAudioPlayer
{
public:
    AAudioPlayer();
    ~AAudioPlayer();

    bool startAAudioPlayback();
    bool stopAAudioPlayback();

private:
    aaudio_usage_t mUsage;
    aaudio_content_type_t mContent;
    int32_t mSampleRate;
    int32_t mChannelCount;
    aaudio_format_t mFormat;
    int32_t mFramesPerBurst;
    int32_t mNumOfBursts;
    aaudio_direction_t mDirection;
    aaudio_sharing_mode_t mSharingMode;
    aaudio_performance_mode_t mPerformanceMode;

    bool mIsPlaying;
    AAudioStream *mAAudioStream;
    const std::string mAudioFile;
#ifdef ENABLE_CALLBACK
    SharedBuffer *mSharedBuf;
#endif

    void stopPlayback();
};
#endif // AAUDIOPLAYER_AAUDIO_PLAYER_H
