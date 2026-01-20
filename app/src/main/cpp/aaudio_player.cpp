#include "WaveFile.h"
#include <aaudio/AAudio.h>
#include <algorithm>
#include <android/log.h>
#include <atomic>
#include <jni.h>
#include <memory>
#include <string>
#include <unordered_map>

// 日志宏
#define LOG_TAG "AAudioPlayer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
// #define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 简化的播放器状态结构
struct AudioPlayerState {
    AAudioStream* stream = nullptr;
    std::unique_ptr<WaveFile> waveFile;
    std::atomic<bool> isPlaying{false};

    // Java回调相关
    JavaVM* jvm = nullptr;
    jobject playerInstance = nullptr;
    jmethodID onStateChangedMethod = nullptr;

    // 配置参数
    aaudio_usage_t usage = AAUDIO_USAGE_MEDIA;
    aaudio_content_type_t contentType = AAUDIO_CONTENT_TYPE_MUSIC;
    aaudio_performance_mode_t performanceMode = AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
    aaudio_sharing_mode_t sharingMode = AAUDIO_SHARING_MODE_SHARED;
    std::string audioFilePath = "/data/48k_2ch_16bit.wav";
};

static AudioPlayerState g_player;

// 简化的Java回调
static void notifyStateChanged(bool playing) {
    if (g_player.jvm && g_player.playerInstance && g_player.onStateChangedMethod) {
        JNIEnv* env;
        if (g_player.jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
            env->CallVoidMethod(g_player.playerInstance, g_player.onStateChangedMethod, playing);
        }
    }
}

// 简化的枚举映射 - 移除重复的RINGTONE映射
static const std::unordered_map<std::string, aaudio_usage_t> USAGE_MAP = {
    {"MEDIA", AAUDIO_USAGE_MEDIA},
    {"VOICE_COMMUNICATION", AAUDIO_USAGE_VOICE_COMMUNICATION},
    {"VOICE_COMMUNICATION_SIGNALLING", AAUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING},
    {"ALARM", AAUDIO_USAGE_ALARM},
    {"NOTIFICATION", AAUDIO_USAGE_NOTIFICATION},
    {"RINGTONE", AAUDIO_USAGE_NOTIFICATION_RINGTONE},
    {"NOTIFICATION_EVENT", AAUDIO_USAGE_NOTIFICATION_EVENT},
    {"ACCESSIBILITY", AAUDIO_USAGE_ASSISTANCE_ACCESSIBILITY},
    {"NAVIGATION_GUIDANCE", AAUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE},
    {"SYSTEM_SONIFICATION", AAUDIO_USAGE_ASSISTANCE_SONIFICATION},
    {"GAME", AAUDIO_USAGE_GAME},
    {"ASSISTANT", AAUDIO_USAGE_ASSISTANT}};

static const std::unordered_map<std::string, aaudio_content_type_t> CONTENT_TYPE_MAP = {
    {"SPEECH", AAUDIO_CONTENT_TYPE_SPEECH},
    {"MUSIC", AAUDIO_CONTENT_TYPE_MUSIC},
    {"MOVIE", AAUDIO_CONTENT_TYPE_MOVIE},
    {"SONIFICATION", AAUDIO_CONTENT_TYPE_SONIFICATION}};
// 简化的枚举查找函数
static aaudio_usage_t getUsageFromString(const std::string& usage) {
    auto it = USAGE_MAP.find(usage);
    return (it != USAGE_MAP.end()) ? it->second : AAUDIO_USAGE_MEDIA;
}

static aaudio_content_type_t getContentTypeFromString(const std::string& contentType) {
    auto it = CONTENT_TYPE_MAP.find(contentType);
    return (it != CONTENT_TYPE_MAP.end()) ? it->second : AAUDIO_CONTENT_TYPE_MUSIC;
}

// 优化的音频回调 - 移除不必要的检查和日志
static aaudio_data_callback_result_t
audioCallback(AAudioStream* stream, void* userData, void* audioData, int32_t numFrames) {
    if (!g_player.isPlaying.load()) {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    if (!g_player.waveFile || !g_player.waveFile->isOpen()) {
        g_player.isPlaying.store(false);
        notifyStateChanged(false);
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    // 计算需要的字节数
    int32_t channelCount = AAudioStream_getChannelCount(stream);
    int32_t bytesPerSample = (AAudioStream_getFormat(stream) == AAUDIO_FORMAT_PCM_I16) ? 2 : 4;
    int32_t bytesToRead = numFrames * channelCount * bytesPerSample;

    size_t bytesRead = g_player.waveFile->readAudioData(audioData, bytesToRead);

    if (bytesRead < static_cast<size_t>(bytesToRead)) {
        // 播放完成
        g_player.isPlaying.store(false);
        notifyStateChanged(false);
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

// 简化的错误回调
static void errorCallback(AAudioStream* stream, void* userData, aaudio_result_t error) {
    LOGE("AAudio error: %s", AAudio_convertResultToText(error));
    g_player.isPlaying.store(false);
    notifyStateChanged(false);
}

// 简化的缓冲区配置 - 移除冗余的日志输出
static bool createOptimizedStream() {
    AAudioStreamBuilder* builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGE("Failed to create builder: %s", AAudio_convertResultToText(result));
        return false;
    }

    // 使用WAV文件参数或默认值
    int32_t sampleRate = 48000;
    int32_t channelCount = 2;
    aaudio_format_t format = AAUDIO_FORMAT_PCM_I16;

    if (g_player.waveFile && g_player.waveFile->isOpen()) {
        sampleRate = g_player.waveFile->getSampleRate();
        channelCount = g_player.waveFile->getChannelCount();
        format = static_cast<aaudio_format_t>(g_player.waveFile->getAAudioFormat());
    }

    // 配置流
    AAudioStreamBuilder_setSampleRate(builder, sampleRate);
    AAudioStreamBuilder_setChannelCount(builder, channelCount);
    AAudioStreamBuilder_setFormat(builder, format);
    AAudioStreamBuilder_setUsage(builder, g_player.usage);
    AAudioStreamBuilder_setContentType(builder, g_player.contentType);
    AAudioStreamBuilder_setSharingMode(builder, g_player.sharingMode);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setPerformanceMode(builder, g_player.performanceMode);

    // 简化的缓冲区配置
    int32_t bufferCapacity = (g_player.performanceMode == AAUDIO_PERFORMANCE_MODE_LOW_LATENCY)
                                 ? (sampleRate * 40) / 1000   // 40ms for low latency
                                 : (sampleRate * 100) / 1000; // 100ms for power saving
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, bufferCapacity);

    // 设置回调
    AAudioStreamBuilder_setDataCallback(builder, audioCallback, nullptr);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);

    // 创建流
    result = AAudioStreamBuilder_openStream(builder, &g_player.stream);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK) {
        LOGE("Failed to open stream: %s", AAudio_convertResultToText(result));
        return false;
    }

    // 优化缓冲区大小
    int32_t framesPerBurst = AAudioStream_getFramesPerBurst(g_player.stream);
    if (framesPerBurst > 0) {
        int32_t optimalSize =
            framesPerBurst * (g_player.performanceMode == AAUDIO_PERFORMANCE_MODE_LOW_LATENCY ? 2 : 4);
        optimalSize = std::min(optimalSize, AAudioStream_getBufferCapacityInFrames(g_player.stream));
        AAudioStream_setBufferSizeInFrames(g_player.stream, optimalSize);
    }

    LOGI("Stream created: %dHz, %dch, format=%d, mode=%d", AAudioStream_getSampleRate(g_player.stream),
         AAudioStream_getChannelCount(g_player.stream), AAudioStream_getFormat(g_player.stream),
         AAudioStream_getPerformanceMode(g_player.stream));

    return true;
}
// JNI方法实现
extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_player.jvm = vm;
    LOGI("JNI_OnLoad - AAudio Player");
    return JNI_VERSION_1_6;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_initializeNative(JNIEnv* env,
                                                                                              jobject thiz) {
    LOGI("initializeNative");
    g_player.playerInstance = env->NewGlobalRef(thiz);
    jclass clazz = env->GetObjectClass(thiz);
    g_player.onStateChangedMethod = env->GetMethodID(clazz, "onNativePlaybackStateChanged", "(Z)V");
    return (g_player.onStateChangedMethod != nullptr) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_setNativeConfig(JNIEnv* env,
                                                                                             jobject thiz,
                                                                                             jobject configMap) {
    if (!configMap)
        return JNI_FALSE;

    jclass mapClass = env->GetObjectClass(configMap);
    jmethodID getMethod = env->GetMethodID(mapClass, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");

    // 简化的配置获取 - 使用C++14 lambda
    auto getString = [&](const char* key) -> std::string {
        jstring keyStr = env->NewStringUTF(key);
        jobject valueObj = env->CallObjectMethod(configMap, getMethod, keyStr);
        env->DeleteLocalRef(keyStr);
        if (!valueObj)
            return "";

        jstring valueStr = static_cast<jstring>(valueObj);
        const char* chars = env->GetStringUTFChars(valueStr, nullptr);
        std::string result(chars);
        env->ReleaseStringUTFChars(valueStr, chars);
        env->DeleteLocalRef(valueObj);
        return result;
    };

    // 更新配置
    std::string usage = getString("usage");
    std::string contentType = getString("contentType");
    std::string performanceMode = getString("performanceMode");
    std::string sharingMode = getString("sharingMode");
    std::string audioFilePath = getString("audioFilePath");

    g_player.usage = getUsageFromString(usage);
    g_player.contentType = getContentTypeFromString(contentType);
    g_player.performanceMode =
        (performanceMode == "LOW_LATENCY") ? AAUDIO_PERFORMANCE_MODE_LOW_LATENCY : AAUDIO_PERFORMANCE_MODE_POWER_SAVING;
    g_player.sharingMode = (sharingMode == "EXCLUSIVE") ? AAUDIO_SHARING_MODE_EXCLUSIVE : AAUDIO_SHARING_MODE_SHARED;

    if (!audioFilePath.empty()) {
        g_player.audioFilePath = audioFilePath;
    }

    LOGI("Config updated: usage=%s, contentType=%s, performanceMode=%s", usage.c_str(), contentType.c_str(),
         performanceMode.c_str());

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_startPlaybackNative(JNIEnv* env,
                                                                                                 jobject thiz) {
    LOGI("startPlaybackNative");
    if (g_player.isPlaying.load())
        return JNI_FALSE;

    g_player.waveFile = std::make_unique<WaveFile>();
    if (!g_player.waveFile->open(g_player.audioFilePath)) {
        LOGE("Failed to open: %s", g_player.audioFilePath.c_str());
        g_player.waveFile.reset();
        return JNI_FALSE;
    }

    if (!createOptimizedStream()) {
        g_player.waveFile.reset();
        return JNI_FALSE;
    }

    g_player.isPlaying.store(true);
    aaudio_result_t result = AAudioStream_requestStart(g_player.stream);

    if (result != AAUDIO_OK) {
        LOGE("Failed to start: %s", AAudio_convertResultToText(result));
        g_player.isPlaying.store(false);
        AAudioStream_close(g_player.stream);
        g_player.stream = nullptr;
        g_player.waveFile.reset();
        return JNI_FALSE;
    }

    LOGI("Playback started successfully");
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_stopPlaybackNative(JNIEnv* env,
                                                                                                jobject thiz) {
    LOGI("stopPlaybackNative");
    g_player.isPlaying.store(false);

    if (g_player.stream) {
        AAudioStream_requestStop(g_player.stream);
        AAudioStream_close(g_player.stream);
        g_player.stream = nullptr;
    }

    g_player.waveFile.reset();
    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_cleanupNative(JNIEnv* env, jobject thiz) {
    LOGI("cleanupNative");
    g_player.isPlaying.store(false);

    if (g_player.stream) {
        AAudioStream_requestStop(g_player.stream);
        AAudioStream_close(g_player.stream);
        g_player.stream = nullptr;
    }

    g_player.waveFile.reset();

    if (g_player.playerInstance) {
        env->DeleteGlobalRef(g_player.playerInstance);
        g_player.playerInstance = nullptr;
    }
}

} // extern "C"