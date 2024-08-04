#include <jni.h>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <vector>
#include <fcntl.h>
#include <aaudio/AAudio.h>
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

AAudioPlayer::AAudioPlayer() : m_usage(AAUDIO_USAGE_MEDIA),
                               m_content(AAUDIO_CONTENT_TYPE_MUSIC),
                               m_sampleRate(48000),
                               m_channelCount(2),
                               m_format(AAUDIO_FORMAT_PCM_I16),
                               m_framesPerBurst(480),
                               m_numOfBursts(2),
                               m_direction(AAUDIO_DIRECTION_OUTPUT),
                               m_sharingMode(AAUDIO_SHARING_MODE_SHARED),
                               m_performanceMode(AAUDIO_PERFORMANCE_MODE_LOW_LATENCY),
                               m_isPlaying(false),
                               m_aaudioStream(nullptr)
{
#ifdef ENABLE_CALLBACK
    m_sharedBuf = new SharedBuffer(static_cast<size_t>(m_sampleRate / 1000 * 40 * m_channelCount * 2));
#endif
#ifdef USE_WAV_HEADER
    m_audioFile = "/data/48k_2ch_16bit.wav";
#else
    m_audioFile = "/data/48k_2ch_16bit.raw";
#endif
}

AAudioPlayer::~AAudioPlayer() = default;

bool AAudioPlayer::startAAudioPlayback()
{
#ifdef LATENCY_TEST
    s_cycle = 0;
    s_invert_flag = 0;
#endif

    if (access(m_audioFile.c_str(), F_OK) == -1)
    {
        ALOGE("audio file %s not exist\n", m_audioFile.c_str());
        return false;
    }
    ALOGI("audio file path: %s\n", m_audioFile.c_str());

#ifdef USE_WAV_HEADER
    /* read wav header */
    WAVHeader header{};
    if (!readWAVHeader(m_audioFile, header))
    {
        ALOGE("readWAVHeader error\n");
        return false;
    }
    // header.print();

    m_sampleRate = (int32_t)header.sampleRate;
    m_channelCount = header.numChannels;
    if ((header.audioFormat == 1) && (header.bitsPerSample == 16))
    {
        m_format = AAUDIO_FORMAT_PCM_I16;
    }
    else if ((header.audioFormat == 1) && (header.bitsPerSample == 24))
    {
        m_format = AAUDIO_FORMAT_PCM_I24_PACKED;
    }
    else if ((header.audioFormat == 1) && (header.bitsPerSample == 32))
    {
        m_format = AAUDIO_FORMAT_PCM_I32;
    }
    else if ((header.audioFormat == 3) && (header.bitsPerSample == 32))
    {
        m_format = AAUDIO_FORMAT_PCM_FLOAT;
    }
    else
    {
        ALOGE("unsupported format and bitsPerSample\n");
        return false;
    }
    ALOGI("wav header params: sampleRate:%d, channelCount:%d, format:%d\n", m_sampleRate, m_channelCount, m_format);
#endif

    // prepare data
    std::ifstream inputFile(m_audioFile, std::ios::in | std::ios::binary);
    if (!inputFile.is_open() || !inputFile.good())
    {
        ALOGE("AAudioPlayer error opening file\n");
        return false;
    }

    // skip wav header
#ifdef USE_WAV_HEADER
    inputFile.seekg(sizeof(WAVHeader), std::ios::beg);
#endif

    AAudioStreamBuilder *builder{nullptr};
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudio_createStreamBuilder() returned %d %s\n", result, AAudio_convertResultToText(result));
        return false;
    }
    AAudioStreamBuilder_setUsage(builder, m_usage);
    AAudioStreamBuilder_setContentType(builder, m_content);
    AAudioStreamBuilder_setSampleRate(builder, m_sampleRate);
    AAudioStreamBuilder_setChannelCount(builder, m_channelCount);
    AAudioStreamBuilder_setFormat(builder, m_format);
    AAudioStreamBuilder_setDirection(builder, m_direction);
    AAudioStreamBuilder_setPerformanceMode(builder, m_performanceMode);
    AAudioStreamBuilder_setSharingMode(builder, m_sharingMode);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, m_sampleRate / 1000 * 40);
    // AAudioStreamBuilder_setChannelMask(builder, channelMask);
    // AAudioStreamBuilder_setDeviceId(builder, AAUDIO_UNSPECIFIED);
    // AAudioStreamBuilder_setFramesPerDataCallback(builder, AAUDIO_UNSPECIFIED);
    // AAudioStreamBuilder_setAllowedCapturePolicy(builder, AAUDIO_ALLOW_CAPTURE_BY_ALL);
    // AAudioStreamBuilder_setPrivacySensitive(builder, false);
#ifdef ENABLE_CALLBACK
    AAudioStreamBuilder_setDataCallback(builder, dataCallback, (void *)m_sharedBuf);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);
#endif
    ALOGI("set AAudio params: Usage:%d, SampleRate:%d, ChannelCount:%d, Format:%d\n", m_usage, m_sampleRate,
          m_channelCount, m_format);

    // Open an AAudioStream using the Builder.
    result = AAudioStreamBuilder_openStream(builder, &m_aaudioStream);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudioStreamBuilder_openStream() returned %d %s\n", result, AAudio_convertResultToText(result));
        return false;
    }
    m_framesPerBurst = AAudioStream_getFramesPerBurst(m_aaudioStream);
    AAudioStream_setBufferSizeInFrames(m_aaudioStream, m_numOfBursts * m_framesPerBurst);

    int32_t actualSampleRate = AAudioStream_getSampleRate(m_aaudioStream);
    int32_t actualChannelCount = AAudioStream_getChannelCount(m_aaudioStream);
    aaudio_format_t actualDataFormat = AAudioStream_getFormat(m_aaudioStream);
    int32_t actualBufferSize = AAudioStream_getBufferSizeInFrames(m_aaudioStream);
    ALOGI("get AAudio params: actualSampleRate:%d, actualChannelCount:%d, actualDataFormat:%d, actualBufferSize:%d, "
          "framesPerBurst:%d\n",
          actualSampleRate, actualChannelCount, actualDataFormat, actualBufferSize, m_framesPerBurst);

    int32_t bytesPerFrame = _getBytesPerSample(actualDataFormat) * actualChannelCount;
    // request start
    result = AAudioStream_requestStart(m_aaudioStream);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudioStream_requestStart returned %d %s\n", result, AAudio_convertResultToText(result));
        if (m_aaudioStream != nullptr)
        {
            AAudioStream_close(m_aaudioStream);
            m_aaudioStream = nullptr;
        }
        return false;
    }
    aaudio_stream_state_t state = AAudioStream_getState(m_aaudioStream);
    ALOGI("after request start, state = %s\n", AAudio_convertStreamStateToText(state));

#ifdef ENABLE_CALLBACK
    m_sharedBuf->setBufSize(m_framesPerBurst * bytesPerFrame * 8);
#endif
    m_isPlaying = true;
    char *bufWrite2File = new char[m_framesPerBurst * bytesPerFrame * 2];
    while (m_aaudioStream)
    {
        inputFile.read(bufWrite2File, m_framesPerBurst * bytesPerFrame * 2);
        if (inputFile.eof())
            m_isPlaying = false;
        int32_t bytes2Write = inputFile.gcount();
        if (bytes2Write > 0)
        {
#ifdef ENABLE_CALLBACK
            bool ret = false;
            do
            {
                ret = m_sharedBuf->produce(bufWrite2File, bytes2Write);
                usleep(8 * 1000);
            } while (!ret);
#else
            int32_t framesPerWrite = m_framesPerBurst * 2;
            // Only complete frames will be written
            if (bytes2Write != m_framesPerBurst * bytesPerFrame * 2)
            {
                framesPerWrite = (int32_t)(bytes2Write / bytesPerFrame);
                ALOGW("aaudio read file, framesPerBurst:%d, bytes2Write:%d, framesPerWrite:%d\n", m_framesPerBurst,
                      bytes2Write, framesPerWrite);
            }

            int32_t framesWritten = 0;
            while (framesWritten < framesPerWrite)
            {
                result = AAudioStream_write(m_aaudioStream, bufWrite2File + framesWritten * bytesPerFrame,
                                            framesPerWrite - framesWritten, 40 * 1000 * 1000);
                if (result < 0)
                {
                    ALOGE("AAudio write failed, result %d %s\n", result, AAudio_convertResultToText(result));
                    m_isPlaying = false;
                    break;
                }
                framesWritten += result;
            }
#endif
        }
        if (!m_isPlaying)
        {
            _stopPlayback();
            if (inputFile.is_open())
            {
                inputFile.close();
            }
            if (bufWrite2File)
            {
                delete[] bufWrite2File;
                bufWrite2File = nullptr;
            }
        }
    }
    return true;
}

bool AAudioPlayer::stopAAudioPlayback()
{
    m_isPlaying = false;
    return true;
}

void AAudioPlayer::_stopPlayback()
{
    int32_t xRunCount = AAudioStream_getXRunCount(m_aaudioStream);
    ALOGI("AAudioStream_getXRunCount %d\n", xRunCount);
    aaudio_result_t result = AAudioStream_requestStop(m_aaudioStream);
    if (result == AAUDIO_OK)
    {
        aaudio_stream_state_t currentState = AAudioStream_getState(m_aaudioStream);
        aaudio_stream_state_t inputState = currentState;
        while (result == AAUDIO_OK && currentState != AAUDIO_STREAM_STATE_STOPPED)
        {
            result = AAudioStream_waitForStateChange(m_aaudioStream, inputState, &currentState, 60 * 1000 * 1000);
            inputState = currentState;
        }
    }
    else
    {
        ALOGE("aaudio request stop error, ret %d %s\n", result, AAudio_convertResultToText(result));
    }

    aaudio_stream_state_t currentState = AAudioStream_getState(m_aaudioStream);
    if (currentState != AAUDIO_STREAM_STATE_STOPPED)
    {
        ALOGW("AAudioStream_waitForStateChange %s\n", AAudio_convertStreamStateToText(currentState));
    }
    if (m_aaudioStream != nullptr)
    {
        AAudioStream_close(m_aaudioStream);
        m_aaudioStream = nullptr;
    }
}

#ifdef ENABLE_CALLBACK
aaudio_data_callback_result_t
AAudioPlayer::dataCallback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames)
{
    if (numFrames > 0)
    {
        bool ret = false;
        int32_t channels = AAudioStream_getChannelCount(stream);
        int32_t bytesPerFrame = _getBytesPerSample(AAudioStream_getFormat(stream)) * channels;
        // ALOGD("aaudio dataCallback, request numFrames:%d, bytesPerFrame:%d\n", numFrames, bytesPerFrame);
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

void AAudioPlayer::errorCallback(AAudioStream *stream, void *userData, aaudio_result_t error)
{
    ALOGI("errorCallback\n");
}
#endif

int32_t AAudioPlayer::_getBytesPerSample(aaudio_format_t format)
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

AAudioPlayer *AAPlayer{nullptr};
extern "C" JNIEXPORT void JNICALL
Java_com_example_aaudioplayer_MainActivity_startAAudioPlaybackFromJNI(JNIEnv *env, jobject thiz)
{
    // TODO: implement startAAudioPlaybackFromJNI()
    AAPlayer = new AAudioPlayer();
    AAPlayer->startAAudioPlayback();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_aaudioplayer_MainActivity_stopAAudioPlaybackFromJNI(JNIEnv *env, jobject thiz)
{
    // TODO: implement stopAAudioPlaybackFromJNI()
    AAPlayer->stopAAudioPlayback();
}
