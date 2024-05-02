#include <jni.h>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <vector>
#include <aaudio/AAudio.h>
#include "log.h"

#define ENABLE_CALLBACK 1

aaudio_usage_t usage = AAUDIO_USAGE_MEDIA;
aaudio_content_type_t content = AAUDIO_CONTENT_TYPE_MUSIC;
int32_t sampleRate = 48000;
int32_t channelCount = 2;
// aaudio_channel_mask_t channelMask = AAUDIO_CHANNEL_STEREO;
aaudio_format_t format = AAUDIO_FORMAT_PCM_I16;
aaudio_direction_t direction = AAUDIO_DIRECTION_OUTPUT;
aaudio_sharing_mode_t sharingMode = AAUDIO_SHARING_MODE_SHARED;
aaudio_performance_mode_t performanceMode = AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
int32_t capacityInFrames = sampleRate / 1000 * 40;
int32_t framesPerBurst = sampleRate / 1000 * 10;
int32_t bytesPerChannel = 2;
int32_t numOfBursts = 2;
bool isStart = false;

AAudioStreamBuilder *builder = nullptr;
AAudioStream *aaudioStream = nullptr;
std::ifstream inputFile;
std::string audioFile = "/data/48k_2ch_16bit.raw";

#if ENABLE_CALLBACK
aaudio_data_callback_result_t dataCallback(AAudioStream *stream __unused, void *userData __unused, void *audioData, int32_t
numFrames)
{
    // ALOGI("aaudio dataCallback, numFrames:%d, channelCount:%d, bytesPerChannel:%d\n", numFrames, mChannelCount, mBytesPerChannel);
    if (numFrames > 0) {
        if (inputFile.is_open()) {
            inputFile.read(static_cast<char *>(audioData), numFrames * channelCount * bytesPerChannel);
            int32_t bytesRead = inputFile.gcount();
            ALOGD("aaudio dataCallback, request numFrames:%d, bytesRead:%d\n", numFrames, bytesRead);
            if (inputFile.eof()) {
                ALOGI("aaudio read data file end\n");
                inputFile.close();
                return AAUDIO_CALLBACK_RESULT_STOP;
            }
        } else {
            ALOGI("aaudio data file closed\n");
            return AAUDIO_CALLBACK_RESULT_STOP;
        }
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void errorCallback(AAudioStream *stream __unused, void *userData __unused, aaudio_result_t error __unused)
{
    ALOGI("errorCallback\n");
}
#endif

bool stopAAudioPlayback();
bool startAAudioPlayback()
{
    ALOGI("start AAudioPlayback, isStart: %d\n", isStart);
    if (isStart) {
        ALOGI("in starting status, needn't start again\n");
        return false;
    }
    if (aaudioStream && AAudioStream_getState(aaudioStream) != AAUDIO_STREAM_STATE_CLOSED) {
        ALOGI("stream not in closed status, try again later\n"); // avoid start again during closing
        return false;
    }
    isStart = true;

    // prepare data
    inputFile.open(audioFile, std::ios::in | std::ios::binary);
    if (!inputFile.is_open()) {
        ALOGE("AAudioPlayer error opening file\n");
        isStart = false;
        return false;
    }

    // Use an AAudioStreamBuilder to contain requested parameters.
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        ALOGE("AAudio_createStreamBuilder() returned %d %s\n", result, AAudio_convertResultToText(result));
        isStart = false;
        return false;
    }
    AAudioStreamBuilder_setUsage(builder, usage);
    AAudioStreamBuilder_setContentType(builder, content);
    AAudioStreamBuilder_setSampleRate(builder, sampleRate);
    AAudioStreamBuilder_setChannelCount(builder, channelCount);
    // AAudioStreamBuilder_setChannelMask(builder, channelMask);
    AAudioStreamBuilder_setFormat(builder, format);
    AAudioStreamBuilder_setDirection(builder, direction);
    AAudioStreamBuilder_setPerformanceMode(builder, performanceMode);
    AAudioStreamBuilder_setSharingMode(builder, sharingMode);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, capacityInFrames);
    // AAudioStreamBuilder_setDeviceId(builder, AAUDIO_UNSPECIFIED);
    // AAudioStreamBuilder_setFramesPerDataCallback(builder, AAUDIO_UNSPECIFIED);
    // AAudioStreamBuilder_setAllowedCapturePolicy(builder, AAUDIO_ALLOW_CAPTURE_BY_ALL);
    // AAudioStreamBuilder_setPrivacySensitive(builder, false);
#if ENABLE_CALLBACK
    AAudioStreamBuilder_setDataCallback(builder, dataCallback, nullptr);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);
#endif
    ALOGI("set AAudio params: Usage:%d, SampleRate:%d, ChannelCount:%d, Format:%d\n", usage, sampleRate,
          channelCount, format);

    // Open an AAudioStream using the Builder.
    result = AAudioStreamBuilder_openStream(builder, &aaudioStream);
	AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK) {
        ALOGE("AAudioStreamBuilder_openStream() returned %d %s\n", result, AAudio_convertResultToText(result));
        isStart = false;
		return false;
    }
    framesPerBurst = AAudioStream_getFramesPerBurst(aaudioStream);
    AAudioStream_setBufferSizeInFrames(aaudioStream, numOfBursts * framesPerBurst);

    int32_t actualSampleRate = AAudioStream_getSampleRate(aaudioStream);
    int32_t actualChannelCount = AAudioStream_getChannelCount(aaudioStream);
    int32_t actualDataFormat = AAudioStream_getFormat(aaudioStream);
    int32_t actualBufferSize = AAudioStream_getBufferSizeInFrames(aaudioStream);
    ALOGI("get AAudio params: actualSampleRate:%d, actualChannelCount:%d, actualDataFormat:%d, actualBufferSize:%d, "
          "framesPerBurst:%d\n", actualSampleRate, actualChannelCount, actualDataFormat, actualBufferSize, framesPerBurst);

    switch (actualDataFormat)
    {
    case AAUDIO_FORMAT_PCM_FLOAT:
        bytesPerChannel = 4;
        break;
    case AAUDIO_FORMAT_PCM_I16:
        bytesPerChannel = 2;
        break;
    case AAUDIO_FORMAT_PCM_I24_PACKED:
        bytesPerChannel = 3;
        break;
    case AAUDIO_FORMAT_PCM_I32:
        bytesPerChannel = 4;
        break;
    default:
        bytesPerChannel = 2;
        break;
    }

    // request start
    result = AAudioStream_requestStart(aaudioStream);
    if (result != AAUDIO_OK) {
        ALOGE("AAudioStream_requestStart(input) returned %d %s\n", result, AAudio_convertResultToText(result));
        if (aaudioStream != nullptr) {
            AAudioStream_close(aaudioStream);
            aaudioStream = nullptr;
        }
		isStart = false;
        return false;
    }
    aaudio_stream_state_t state = AAudioStream_getState(aaudioStream);
    ALOGI("after request start, state = %s\n", AAudio_convertStreamStateToText(state));

    while (aaudioStream)
    {
        if (!inputFile.is_open()) break;
        if (inputFile.is_open() && inputFile.eof()) break;
#if !ENABLE_CALLBACK
		std::vector<char> dataBuf(actualBufferSize * actualChannelCount * bytesPerChannel);
        inputFile.read(dataBuf.data(), framesPerBurst * actualChannelCount * bytesPerChannel);
        int32_t bytes2Write = inputFile.gcount();
        int32_t framesPerWrite = framesPerBurst;
        if (bytes2Write != framesPerBurst * actualChannelCount * bytesPerChannel) {
            ALOGW("aaudio read file, framesPerBurst:%d, bytes2Write:%d\n", framesPerBurst, bytes2Write);
            framesPerWrite = (int32_t)(bytes2Write / actualChannelCount / bytesPerChannel);
            if (framesPerWrite == 0) continue;
        }
        char *temp_bytesData = dataBuf.data();
        for (int i = 0; i < 4; i++)
        {
            if (aaudioStream == nullptr) break;
            if (AAudioStream_getState(aaudioStream) >= AAUDIO_STREAM_STATE_STOPPING) break;
            // TBD should add mutex here, avoid to write during closing.
            result = AAudioStream_write(aaudioStream, temp_bytesData, framesPerWrite, 60 * 1000 * 1000);
            if (result > 0) {
                ALOGD("AAudioPlayer aaudio write framesPerWrite:%d, ret:%d\n", framesPerWrite, result);
                if (result == framesPerWrite) break;
                framesPerWrite -= result;
                temp_bytesData += result * actualChannelCount * bytesPerChannel;
            }
            if (result < 0) {
                ALOGE("AAudioPlayer aaudio write error, ret %d %s\n", result, AAudio_convertResultToText(result));
                break;
            }
            usleep(2 * 1000);
        }
#endif
		usleep(2 * 1000);
    }

	if (aaudioStream) stopAAudioPlayback();
    return true;
}


bool stopAAudioPlayback()
{
    ALOGI("stop AAudioPlayback, isStart: %d\n", isStart);
	if (isStart) {
        isStart = false;
		int32_t xRunCount = AAudioStream_getXRunCount(aaudioStream);
        ALOGI("AAudioStream_getXRunCount %d\n", xRunCount);
        aaudio_result_t result = AAudioStream_requestStop(aaudioStream);
		if (result == AAUDIO_OK) {
			aaudio_stream_state_t currentState = AAudioStream_getState(aaudioStream);
			aaudio_stream_state_t inputState = currentState;
			while (result == AAUDIO_OK && currentState != AAUDIO_STREAM_STATE_STOPPED)
			{
				result = AAudioStream_waitForStateChange(aaudioStream, inputState, &currentState, 60 * 1000 * 1000);
				inputState = currentState;
			}
		} else {
            ALOGE("aaudio request stop error, ret %d %s\n", result, AAudio_convertResultToText(result));
        }

        if (aaudioStream != nullptr) {
            AAudioStream_close(aaudioStream);
            aaudioStream = nullptr;
        }
        if (inputFile.is_open()) inputFile.close();
        // isStart = false;
    }
    return true;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_aaudioplayer_MainActivity_startAAudioPlaybackFromJNI(JNIEnv *env __unused, jobject)
{
    startAAudioPlayback();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_aaudioplayer_MainActivity_stopAAudioPlaybackFromJNI(JNIEnv *env __unused, jobject)
{
    stopAAudioPlayback();
}
