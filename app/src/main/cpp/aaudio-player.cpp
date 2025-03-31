#include <jni.h>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include "common.h"
#include "aaudio-player.h"
#include "wav-header.h"

#ifdef LATENCY_TEST
#define WRITE_CYCLE 100
#define GPIO_FILE "/sys/class/gpio/gpio376/value"
#define GPIO_VAL_H "1"
#define GPIO_VAL_L "0"

static int32_t s_cycle = 0;
static int32_t s_invert_flag = 0;

void gpio_set_high()
{
    int fd = open(GPIO_FILE, O_RDWR);
    if (fd != -1)
    {
        write(fd, GPIO_VAL_H, sizeof(GPIO_VAL_H));
        close(fd);
    }
    else
    {
        ALOGE("failed to open gpio file");
    }
}

void gpio_set_low()
{
    int fd = open(GPIO_FILE, O_RDWR);
    if (fd != -1)
    {
        write(fd, GPIO_VAL_L, sizeof(GPIO_VAL_L));
        close(fd);
    }
    else
    {
        ALOGE("failed to open gpio file");
    }
}
#endif

int32_t getBytesPerSample(aaudio_format_t format);
#ifdef ENABLE_CALLBACK
aaudio_data_callback_result_t dataCallback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames);
void errorCallback([[maybe_unused]] AAudioStream *stream, [[maybe_unused]] void *userData, aaudio_result_t result);
#endif

AAudioPlayer::AAudioPlayer() : mUsage(AAUDIO_USAGE_MEDIA),
                               mContent(AAUDIO_CONTENT_TYPE_MUSIC),
                               mSampleRate(48000),
                               mChannelCount(2),
                               mFormat(AAUDIO_FORMAT_PCM_I16),
                               mFramesPerBurst(480),
                               mNumOfBursts(2),
                               mDirection(AAUDIO_DIRECTION_OUTPUT),
                               mSharingMode(AAUDIO_SHARING_MODE_SHARED),
                               mPerformanceMode(AAUDIO_PERFORMANCE_MODE_LOW_LATENCY),
                               mIsPlaying(false),
                               mAAudioStream(nullptr),
#ifdef USE_WAV_HEADER
                               mAudioFile("/data/48k_2ch_16bit.wav")
#else
                               mAudioFile("/data/48k_2ch_16bit.wav")
#endif
{
#ifdef ENABLE_CALLBACK
    mSharedBuf = new SharedBuffer(static_cast<size_t>(mSampleRate / 1000 * 40 * mChannelCount * 2));
#endif
}

AAudioPlayer::~AAudioPlayer()
{
#ifdef ENABLE_CALLBACK
    if (mSharedBuf)
    {
        delete mSharedBuf;
        mSharedBuf = nullptr;
    }
#endif
}

bool AAudioPlayer::startAAudioPlayback()
{
    char *bufReadFromFile = nullptr;
    aaudio_stream_state_t state = AAUDIO_STREAM_STATE_UNINITIALIZED;

#ifdef LATENCY_TEST
    s_cycle = 0;
    s_invert_flag = 0;
#endif

    if (access(mAudioFile.c_str(), F_OK) == -1)
    {
        ALOGE("audio file %s not exist\n", mAudioFile.c_str());
        return false;
    }
    ALOGI("audio file path: %s\n", mAudioFile.c_str());

#ifdef USE_WAV_HEADER
    /* read wav header */
    WAVHeader header{};
    if (!readWAVHeader(mAudioFile, header))
    {
        ALOGE("readWAVHeader error\n");
        return false;
    }
    // header.print();

    mSampleRate = (int32_t)header.sampleRate;
    mChannelCount = header.numChannels;
    if ((header.audioFormat == 1) && (header.bitsPerSample == 16))
    {
        mFormat = AAUDIO_FORMAT_PCM_I16;
    }
    else if ((header.audioFormat == 1) && (header.bitsPerSample == 24))
    {
        mFormat = AAUDIO_FORMAT_PCM_I24_PACKED;
    }
    else if ((header.audioFormat == 1) && (header.bitsPerSample == 32))
    {
        mFormat = AAUDIO_FORMAT_PCM_I32;
    }
    else if ((header.audioFormat == 3) && (header.bitsPerSample == 32))
    {
        mFormat = AAUDIO_FORMAT_PCM_FLOAT;
    }
    else
    {
        ALOGE("unsupported format and bitsPerSample\n");
        return false;
    }
    ALOGI("wav header params: sampleRate:%d, channelCount:%d, format:%d\n", mSampleRate, mChannelCount, mFormat);
#endif

    // prepare data
    std::ifstream inputFile(mAudioFile, std::ios::in | std::ios::binary);
    if (!inputFile.is_open() || !inputFile.good())
    {
        ALOGE("AAudioPlayer error opening file\n");
        return false;
    }
#ifdef USE_WAV_HEADER
    inputFile.seekg(sizeof(WAVHeader), std::ios::beg); // skip wav header
#endif

    AAudioStreamBuilder *builder{nullptr};
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudio_createStreamBuilder() returned %d %s\n", result, AAudio_convertResultToText(result));
        return false;
    }
    AAudioStreamBuilder_setUsage(builder, mUsage);
    AAudioStreamBuilder_setContentType(builder, mContent);
    AAudioStreamBuilder_setSampleRate(builder, mSampleRate);
    AAudioStreamBuilder_setChannelCount(builder, mChannelCount);
    AAudioStreamBuilder_setFormat(builder, mFormat);
    AAudioStreamBuilder_setDirection(builder, mDirection);
    AAudioStreamBuilder_setPerformanceMode(builder, mPerformanceMode);
    AAudioStreamBuilder_setSharingMode(builder, mSharingMode);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, mSampleRate / 1000 * 40);
    // AAudioStreamBuilder_setChannelMask(builder, channelMask);
    // AAudioStreamBuilder_setDeviceId(builder, AAUDIO_UNSPECIFIED);
    // AAudioStreamBuilder_setFramesPerDataCallback(builder, AAUDIO_UNSPECIFIED);
    // AAudioStreamBuilder_setAllowedCapturePolicy(builder, AAUDIO_ALLOW_CAPTURE_BY_ALL);
    // AAudioStreamBuilder_setPrivacySensitive(builder, false);
#ifdef ENABLE_CALLBACK
    AAudioStreamBuilder_setDataCallback(builder, dataCallback, (void *)mSharedBuf);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);
#endif
    ALOGI("set AAudio params: Usage:%d, SampleRate:%d, ChannelCount:%d, Format:%d\n", mUsage, mSampleRate,
          mChannelCount, mFormat);

    // Open an AAudioStream using the Builder.
    result = AAudioStreamBuilder_openStream(builder, &mAAudioStream);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudioStreamBuilder_openStream() returned %d %s\n", result, AAudio_convertResultToText(result));
        return false;
    }
    mFramesPerBurst = AAudioStream_getFramesPerBurst(mAAudioStream);
    AAudioStream_setBufferSizeInFrames(mAAudioStream, mNumOfBursts * mFramesPerBurst);

    int32_t actualSampleRate = AAudioStream_getSampleRate(mAAudioStream);
    int32_t actualChannelCount = AAudioStream_getChannelCount(mAAudioStream);
    aaudio_format_t actualDataFormat = AAudioStream_getFormat(mAAudioStream);
    int32_t actualBufferSize = AAudioStream_getBufferSizeInFrames(mAAudioStream);
    ALOGI("get AAudio params: actualSampleRate:%d, actualChannelCount:%d, actualDataFormat:%d, actualBufferSize:%d, "
          "framesPerBurst:%d\n",
          actualSampleRate, actualChannelCount, actualDataFormat, actualBufferSize, mFramesPerBurst);

    int32_t bytesPerFrame = getBytesPerSample(actualDataFormat) * actualChannelCount;
    // request start
    result = AAudioStream_requestStart(mAAudioStream);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudioStream_requestStart returned %d %s\n", result, AAudio_convertResultToText(result));
        goto exit_label;
    }
    state = AAudioStream_getState(mAAudioStream);
    ALOGI("after request start, state = %s\n", AAudio_convertStreamStateToText(state));

#ifdef ENABLE_CALLBACK
    mSharedBuf->setBufSize(mFramesPerBurst * bytesPerFrame * 8);
#endif
    bufReadFromFile = new (std::nothrow) char[mFramesPerBurst * bytesPerFrame * 2];
    if (!bufReadFromFile)
    {
        ALOGE("AAudio new bufReadFromFile failed\n");
        goto exit_label;
    }
    mIsPlaying = true;
    while (mAAudioStream)
    {
        // memset(bufReadFromFile,0,mFramesPerBurst * bytesPerFrame * 2);
        inputFile.read(bufReadFromFile, mFramesPerBurst * bytesPerFrame * 2);
        if (inputFile.eof())
            mIsPlaying = false;
        int32_t bytes2Write = inputFile.gcount();
        if (bytes2Write > 0)
        {
#ifdef ENABLE_CALLBACK
            bool ret;
            do
            {
                ret = mSharedBuf->produce(bufReadFromFile, bytes2Write);
                usleep(8 * 1000);
            } while (!ret);
#else
            int32_t framesPerWrite = mFramesPerBurst * 2;
            // Only complete frames will be written
            if (bytes2Write != mFramesPerBurst * bytesPerFrame * 2)
            {
                framesPerWrite = (int32_t)(bytes2Write / bytesPerFrame);
                ALOGW("aaudio read file, framesPerBurst:%d, bytes2Write:%d, framesPerWrite:%d\n", mFramesPerBurst,
                      bytes2Write, framesPerWrite);
            }

            // block write as timeoutNanoseconds is not zero
            int32_t rst = AAudioStream_write(mAAudioStream, bufReadFromFile, framesPerWrite, 80 * 1000 * 1000);
            if (rst < 0)
            {
                ALOGE("AAudio write failed, rst %d %s\n", rst, AAudio_convertResultToText(rst));
                mIsPlaying = false;
            }
            if (rst != framesPerWrite)
                ALOGD("AAudio actually write frames %d, should write frames %d\n", rst, framesPerWrite);
#endif
        }
        if (!mIsPlaying)
        {
            stopPlayback();
            goto exit_label;
        }
    }

exit_label:
    if (mAAudioStream != nullptr)
    {
        AAudioStream_close(mAAudioStream);
        mAAudioStream = nullptr;
    }
    if (inputFile.is_open())
    {
        inputFile.close();
    }
    delete[] bufReadFromFile;

    return true;
}

bool AAudioPlayer::stopAAudioPlayback()
{
    mIsPlaying = false;
    return true;
}

void AAudioPlayer::stopPlayback()
{
    int32_t xRunCount = AAudioStream_getXRunCount(mAAudioStream);
    ALOGI("AAudioStream_getXRunCount %d\n", xRunCount);
    aaudio_result_t result = AAudioStream_requestStop(mAAudioStream);
    if (result == AAUDIO_OK)
    {
        aaudio_stream_state_t currentState = AAudioStream_getState(mAAudioStream);
        aaudio_stream_state_t inputState = currentState;
        while (result == AAUDIO_OK && currentState != AAUDIO_STREAM_STATE_STOPPED)
        {
            result = AAudioStream_waitForStateChange(mAAudioStream, inputState, &currentState, 60 * 1000 * 1000);
            inputState = currentState;
        }
    }
    else
    {
        ALOGE("aaudio request stop error, ret %d %s\n", result, AAudio_convertResultToText(result));
    }

    aaudio_stream_state_t currentState = AAudioStream_getState(mAAudioStream);
    if (currentState != AAUDIO_STREAM_STATE_STOPPED)
    {
        ALOGW("AAudioStream_waitForStateChange %s\n", AAudio_convertStreamStateToText(currentState));
    }
    if (mAAudioStream)
    {
        AAudioStream_close(mAAudioStream);
        mAAudioStream = nullptr;
    }
}

int32_t getBytesPerSample(aaudio_format_t format)
{
    switch (format)
    {
    case AAUDIO_FORMAT_PCM_I16:
        return 2;
    case AAUDIO_FORMAT_PCM_I24_PACKED:
        return 3;
    case AAUDIO_FORMAT_PCM_I32:
    case AAUDIO_FORMAT_PCM_FLOAT:
        return 4;
    default:
        return 2;
    }
}

#ifdef ENABLE_CALLBACK
aaudio_data_callback_result_t dataCallback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames)
{
    if (numFrames > 0)
    {
        bool ret;
        int32_t channels = AAudioStream_getChannelCount(stream);
        int32_t bytesPerFrame = getBytesPerSample(AAudioStream_getFormat(stream)) * channels;
        // ALOGD("aaudio dataCallback, request numFrames:%d, bytesPerFrame:%d\n", numFrames, bytesPerFrame);
        if (!userData)
        {
            return AAUDIO_CALLBACK_RESULT_STOP;
        }
#ifdef LATENCY_TEST
        if (s_cycle == WRITE_CYCLE)
        {
            s_invert_flag = 1;
        }
        if (s_cycle == 2 * WRITE_CYCLE)
        {
            s_cycle = 0;
            s_invert_flag = 0;
        }
        if (s_invert_flag == 1)
        {
            memset(audioData, 0, numFrames * bytesPerFrame);
            gpio_set_low();
        }
        else
        {
            ret = ((SharedBuffer *)userData)->consume((char *)audioData, numFrames * bytesPerFrame);
            if (!ret)
            {
                ALOGE("can't read from buffer, buffer is empty\n");
            }
            gpio_set_high();
        }
        s_cycle++;
#else
        ret = ((SharedBuffer *)userData)->consume((char *)audioData, numFrames * bytesPerFrame);
        if (!ret)
        {
            ALOGE("can't read from buffer, buffer is empty\n");
        }
#endif
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void errorCallback([[maybe_unused]] AAudioStream *stream, [[maybe_unused]] void *userData, aaudio_result_t result)
{
    ALOGE("AAudio errorCallback, result: %d %s\n", result, AAudio_convertResultToText(result));
}
#endif

AAudioPlayer *AAPlayer{nullptr};
extern "C" JNIEXPORT void JNICALL
Java_com_example_aaudioplayer_MainActivity_startAAudioPlaybackFromJNI([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jobject thiz)
{
    // TODO: implement startAAudioPlaybackFromJNI()
    AAPlayer = new AAudioPlayer();
    AAPlayer->startAAudioPlayback();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_aaudioplayer_MainActivity_stopAAudioPlaybackFromJNI([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jobject thiz)
{
    // TODO: implement stopAAudioPlaybackFromJNI()
    AAPlayer->stopAAudioPlayback();
}
