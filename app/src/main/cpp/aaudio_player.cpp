#include "aaudio_player.h"
#include <aaudio/AAudio.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <jni.h>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

// AAudio Player implementation
struct AudioPlayerState {
    AAudioStream* stream = nullptr;
    std::unique_ptr<WaveFile> waveFile;
    std::atomic<bool> isPlaying{false};

    // Java回调相关
    JavaVM* jvm = nullptr;
    jobject playerInstance = nullptr;
    jmethodID onPlaybackStartedMethod = nullptr;
    jmethodID onPlaybackStoppedMethod = nullptr;
    jmethodID onPlaybackErrorMethod = nullptr;

    // 配置参数
    aaudio_usage_t usage = AAUDIO_USAGE_MEDIA;
    aaudio_content_type_t contentType = AAUDIO_CONTENT_TYPE_MUSIC;
    aaudio_performance_mode_t performanceMode = AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
    aaudio_sharing_mode_t sharingMode = AAUDIO_SHARING_MODE_SHARED;
    std::string audioFilePath = "/data/48k_2ch_16bit.wav";
};

static AudioPlayerState g_player;

// Java回调函数
static void notifyPlaybackStarted() {
    if (g_player.jvm && g_player.playerInstance && g_player.onPlaybackStartedMethod) {
        JNIEnv* env;
        if (g_player.jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
            env->CallVoidMethod(g_player.playerInstance, g_player.onPlaybackStartedMethod);
        }
    }
}

static void notifyPlaybackStopped() {
    if (g_player.jvm && g_player.playerInstance && g_player.onPlaybackStoppedMethod) {
        JNIEnv* env;
        if (g_player.jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
            env->CallVoidMethod(g_player.playerInstance, g_player.onPlaybackStoppedMethod);
        }
    }
}

static void notifyPlaybackError(const std::string& error) {
    if (g_player.jvm && g_player.playerInstance && g_player.onPlaybackErrorMethod) {
        JNIEnv* env;
        if (g_player.jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
            jstring errorStr = env->NewStringUTF(error.c_str());
            env->CallVoidMethod(g_player.playerInstance, g_player.onPlaybackErrorMethod, errorStr);
            env->DeleteLocalRef(errorStr);
        }
    }
}

// 枚举映射
static const std::unordered_map<std::string, aaudio_usage_t> USAGE_MAP = {
    {"AAUDIO_USAGE_MEDIA", AAUDIO_USAGE_MEDIA},
    {"AAUDIO_USAGE_VOICE_COMMUNICATION", AAUDIO_USAGE_VOICE_COMMUNICATION},
    {"AAUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING", AAUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING},
    {"AAUDIO_USAGE_ALARM", AAUDIO_USAGE_ALARM},
    {"AAUDIO_USAGE_NOTIFICATION", AAUDIO_USAGE_NOTIFICATION},
    {"AAUDIO_USAGE_NOTIFICATION_RINGTONE", AAUDIO_USAGE_NOTIFICATION_RINGTONE},
    {"AAUDIO_USAGE_NOTIFICATION_EVENT", AAUDIO_USAGE_NOTIFICATION_EVENT},
    {"AAUDIO_USAGE_ASSISTANCE_ACCESSIBILITY", AAUDIO_USAGE_ASSISTANCE_ACCESSIBILITY},
    {"AAUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE", AAUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE},
    {"AAUDIO_USAGE_ASSISTANCE_SONIFICATION", AAUDIO_USAGE_ASSISTANCE_SONIFICATION},
    {"AAUDIO_USAGE_GAME", AAUDIO_USAGE_GAME},
    {"AAUDIO_USAGE_ASSISTANT", AAUDIO_USAGE_ASSISTANT}};

static const std::unordered_map<std::string, aaudio_content_type_t> CONTENT_TYPE_MAP = {
    {"AAUDIO_CONTENT_TYPE_SPEECH", AAUDIO_CONTENT_TYPE_SPEECH},
    {"AAUDIO_CONTENT_TYPE_MUSIC", AAUDIO_CONTENT_TYPE_MUSIC},
    {"AAUDIO_CONTENT_TYPE_MOVIE", AAUDIO_CONTENT_TYPE_MOVIE},
    {"AAUDIO_CONTENT_TYPE_SONIFICATION", AAUDIO_CONTENT_TYPE_SONIFICATION}};

// 枚举查找函数
static aaudio_usage_t getUsageFromString(const std::string& usage) {
    auto it = USAGE_MAP.find(usage);
    return (it != USAGE_MAP.end()) ? it->second : AAUDIO_USAGE_MEDIA;
}

static aaudio_content_type_t getContentTypeFromString(const std::string& contentType) {
    auto it = CONTENT_TYPE_MAP.find(contentType);
    return (it != CONTENT_TYPE_MAP.end()) ? it->second : AAUDIO_CONTENT_TYPE_MUSIC;
}

static aaudio_performance_mode_t getPerformanceModeFromString(const std::string& performanceMode) {
    if (performanceMode == "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY") {
        return AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
    } else if (performanceMode == "AAUDIO_PERFORMANCE_MODE_POWER_SAVING") {
        return AAUDIO_PERFORMANCE_MODE_POWER_SAVING;
    }
    return AAUDIO_PERFORMANCE_MODE_LOW_LATENCY; // 默认低延迟
}

static aaudio_sharing_mode_t getSharingModeFromString(const std::string& sharingMode) {
    if (sharingMode == "AAUDIO_SHARING_MODE_EXCLUSIVE") {
        return AAUDIO_SHARING_MODE_EXCLUSIVE;
    } else if (sharingMode == "AAUDIO_SHARING_MODE_SHARED") {
        return AAUDIO_SHARING_MODE_SHARED;
    }
    return AAUDIO_SHARING_MODE_SHARED; // 默认共享
}

// 音频回调
static aaudio_data_callback_result_t
audioCallback(AAudioStream* stream, void* userData, void* audioData, int32_t numFrames) {
    if (!g_player.isPlaying.load()) {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    if (!g_player.waveFile || !g_player.waveFile->isOpen()) {
        g_player.isPlaying.store(false);
        notifyPlaybackError("音频文件未打开");
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
        notifyPlaybackStopped();
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

// 错误回调
static void errorCallback(AAudioStream* stream, void* userData, aaudio_result_t error) {
    LOGE("AAudio error: %s", AAudio_convertResultToText(error));
    g_player.isPlaying.store(false);
    std::string errorMsg = "播放流错误: ";
    errorMsg += AAudio_convertResultToText(error);
    notifyPlaybackError(errorMsg);
}

// 创建AAudio流
static bool createAAudioStream() {
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

    // 缓冲区配置
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
                                                                                        jobject thiz,
                                                                                        jstring filePath) {
    LOGI("initializeNative");

    // 保存JVM引用
    env->GetJavaVM(&g_player.jvm);

    // 保存Java对象引用
    if (g_player.playerInstance) {
        env->DeleteGlobalRef(g_player.playerInstance);
    }
    g_player.playerInstance = env->NewGlobalRef(thiz);
    
    // 获取回调方法ID
    jclass clazz = env->GetObjectClass(thiz);
    g_player.onPlaybackStartedMethod = env->GetMethodID(clazz, "onNativePlaybackStarted", "()V");
    g_player.onPlaybackStoppedMethod = env->GetMethodID(clazz, "onNativePlaybackStopped", "()V");
    g_player.onPlaybackErrorMethod = env->GetMethodID(clazz, "onNativePlaybackError", "(Ljava/lang/String;)V");

    if (!g_player.onPlaybackStartedMethod || !g_player.onPlaybackStoppedMethod || !g_player.onPlaybackErrorMethod) {
        LOGE("Failed to get callback method IDs");
        return JNI_FALSE;
    }

    // 获取文件路径
    if (filePath) {
        const char* path = env->GetStringUTFChars(filePath, nullptr);
        g_player.audioFilePath = std::string(path);
        env->ReleaseStringUTFChars(filePath, path);
    }

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_setNativeConfig(JNIEnv* env,
                                                                                             jobject thiz,
                                                                                             jstring usage,
                                                                                             jstring contentType,
                                                                                             jstring performanceMode,
                                                                                             jstring sharingMode,
                                                                                             jstring filePath) {
    LOGI("setNativeConfig");

    // 更新配置参数
    if (usage) {
        const char* usageStr = env->GetStringUTFChars(usage, nullptr);
        g_player.usage = getUsageFromString(std::string(usageStr));
        env->ReleaseStringUTFChars(usage, usageStr);
    }

    if (contentType) {
        const char* contentTypeStr = env->GetStringUTFChars(contentType, nullptr);
        g_player.contentType = getContentTypeFromString(std::string(contentTypeStr));
        env->ReleaseStringUTFChars(contentType, contentTypeStr);
    }

    if (performanceMode) {
        const char* performanceModeStr = env->GetStringUTFChars(performanceMode, nullptr);
        g_player.performanceMode = getPerformanceModeFromString(std::string(performanceModeStr));
        env->ReleaseStringUTFChars(performanceMode, performanceModeStr);
    }

    if (sharingMode) {
        const char* sharingModeStr = env->GetStringUTFChars(sharingMode, nullptr);
        g_player.sharingMode = getSharingModeFromString(std::string(sharingModeStr));
        env->ReleaseStringUTFChars(sharingMode, sharingModeStr);
    }

    if (filePath) {
        const char* path = env->GetStringUTFChars(filePath, nullptr);
        g_player.audioFilePath = std::string(path);
        env->ReleaseStringUTFChars(filePath, path);
    }

    LOGI("Config updated: usage=%d, contentType=%d, performanceMode=%d, sharingMode=%d, file=%s", g_player.usage,
         g_player.contentType, g_player.performanceMode, g_player.sharingMode, g_player.audioFilePath.c_str());

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_startNativePlayback(JNIEnv* env, jobject thiz) {
    LOGI("startNativePlayback");

    if (g_player.isPlaying.load()) {
        return JNI_FALSE;
    }

    g_player.waveFile = std::make_unique<WaveFile>();
    if (!g_player.waveFile->open(g_player.audioFilePath)) {
        LOGE("Failed to open: %s", g_player.audioFilePath.c_str());
        g_player.waveFile.reset();
        return JNI_FALSE;
    }

    if (!createAAudioStream()) {
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
    notifyPlaybackStarted();
    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_stopNativePlayback(JNIEnv* env, jobject thiz) {
    LOGI("stopNativePlayback");

    g_player.isPlaying.store(false);

    if (g_player.stream) {
        AAudioStream_requestStop(g_player.stream);
        AAudioStream_close(g_player.stream);
        g_player.stream = nullptr;
    }

    g_player.waveFile.reset();
    notifyPlaybackStopped();
}

JNIEXPORT void JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_releaseNative(JNIEnv* env, jobject thiz) {
    LOGI("releaseNative");

    g_player.isPlaying.store(false);

    if (g_player.stream) {
        AAudioStream_requestStop(g_player.stream);
        AAudioStream_close(g_player.stream);
        g_player.stream = nullptr;
    }

    g_player.waveFile.reset();

    // 清理Java引用
    if (g_player.playerInstance) {
        env->DeleteGlobalRef(g_player.playerInstance);
        g_player.playerInstance = nullptr;
    }
}

} // extern "C"

// ============================================================================
// WaveFile Class Implementation
// ============================================================================

WaveFile::WaveFile() : isOpen_(false), header_{} {}

WaveFile::~WaveFile() { close(); }

bool WaveFile::open(const std::string& filePath) {
    close(); // 确保之前的文件已关闭

    file_.open(filePath, std::ios::binary);
    if (!file_.is_open()) {
        LOGE("Failed to open file: %s", filePath.c_str());
        return false;
    }

    if (!readHeader()) {
        LOGE("Failed to read WAV header from: %s", filePath.c_str());
        close();
        return false;
    }

    if (!isValidFormat()) {
        LOGE("Invalid WAV format in file: %s", filePath.c_str());
        close();
        return false;
    }

    isOpen_ = true;
    LOGI("Successfully opened WAV file: %s", filePath.c_str());
    LOGI("Format: %s", getFormatInfo().c_str());

    return true;
}

void WaveFile::close() {
    if (file_.is_open()) {
        file_.close();
    }
    isOpen_ = false;
    header_ = {};
}

size_t WaveFile::readAudioData(void* buffer, size_t bufferSize) {
    if (!isOpen_ || !buffer || bufferSize == 0) {
        return 0;
    }

    // 检查bufferSize是否超过streamsize的最大值
    constexpr auto maxStreamSize = static_cast<size_t>(std::numeric_limits<std::streamsize>::max());
    size_t actualReadSize = std::min(bufferSize, maxStreamSize);

    auto readSize = static_cast<std::streamsize>(actualReadSize);
    file_.read(static_cast<char*>(buffer), readSize);
    auto bytesRead = static_cast<size_t>(file_.gcount());

    if (bytesRead < bufferSize) {
        // 如果读取的数据不足，用0填充剩余部分
        memset(static_cast<char*>(buffer) + bytesRead, 0, bufferSize - bytesRead);
    }

    return bytesRead;
}

bool WaveFile::isOpen() const { return isOpen_; }

int32_t WaveFile::getAAudioFormat() const {
    // 根据WAV文件的位深度返回对应的AAudio格式
    // 注意：这里返回int32_t是为了避免包含AAudio头文件
    // AAudio格式常量值：
    // AAUDIO_FORMAT_PCM_I16 = 1
    // AAUDIO_FORMAT_PCM_I24_PACKED = 2
    // AAUDIO_FORMAT_PCM_I32 = 3
    // AAUDIO_FORMAT_PCM_FLOAT = 4
    switch (header_.bitsPerSample) {
    case 16:
        return 1; // AAUDIO_FORMAT_PCM_I16
    case 24:
        return 2; // AAUDIO_FORMAT_PCM_I24_PACKED
    case 32:
        return 3; // AAUDIO_FORMAT_PCM_I32
    default:
        return 1; // 默认返回16位格式
    }
}

std::string WaveFile::getFormatInfo() const {
    std::ostringstream oss;
    oss << static_cast<int32_t>(header_.sampleRate) << "Hz, " << static_cast<int32_t>(header_.numChannels)
        << " channels, " << static_cast<int32_t>(header_.bitsPerSample) << " bits, PCM";
    return oss.str();
}

bool WaveFile::isValidFormat() const {
    return (header_.audioFormat == 1 &&                               // PCM格式
            header_.numChannels > 0 && header_.numChannels <= 16 &&   // 合理的声道数
            header_.sampleRate > 0 && header_.sampleRate <= 192000 && // 合理的采样率
            (header_.bitsPerSample == 8 || header_.bitsPerSample == 16 || header_.bitsPerSample == 24 ||
             header_.bitsPerSample == 32) && // 支持的位深
            header_.dataSize > 0);           // 有音频数据
}

bool WaveFile::readHeader() {
    file_.seekg(0, std::ios::beg);
    return validateRiffHeader() && readFmtChunk() && findDataChunk();
}

bool WaveFile::validateRiffHeader() {
    // 读取RIFF标识
    file_.read(header_.riffId, 4);
    if (file_.gcount() != 4 || strncmp(header_.riffId, "RIFF", 4) != 0) {
        LOGE("Invalid RIFF header");
        return false;
    }

    // 读取文件大小
    file_.read(reinterpret_cast<char*>(&header_.riffSize), 4);
    if (file_.gcount() != 4) {
        LOGE("Failed to read RIFF size");
        return false;
    }

    // 读取WAVE标识
    file_.read(header_.waveId, 4);
    if (file_.gcount() != 4 || strncmp(header_.waveId, "WAVE", 4) != 0) {
        LOGE("Invalid WAVE header");
        return false;
    }

    return true;
}

bool WaveFile::readFmtChunk() {
    char chunkId[4];
    uint32_t chunkSize;

    // 查找fmt子块
    while (file_.good()) {
        file_.read(chunkId, 4);
        if (file_.gcount() != 4) {
            LOGE("Failed to read chunk ID");
            return false;
        }

        file_.read(reinterpret_cast<char*>(&chunkSize), 4);
        if (file_.gcount() != 4) {
            LOGE("Failed to read chunk size");
            return false;
        }

        if (strncmp(chunkId, "fmt ", 4) == 0) {
            // 找到fmt子块
            strncpy(header_.fmtId, chunkId, 4);

            // 读取fmt数据
            file_.read(reinterpret_cast<char*>(&header_.audioFormat), 2);
            file_.read(reinterpret_cast<char*>(&header_.numChannels), 2);
            file_.read(reinterpret_cast<char*>(&header_.sampleRate), 4);
            file_.read(reinterpret_cast<char*>(&header_.byteRate), 4);
            file_.read(reinterpret_cast<char*>(&header_.blockAlign), 2);
            file_.read(reinterpret_cast<char*>(&header_.bitsPerSample), 2);

            // 跳过额外的fmt数据（如果有）
            if (chunkSize > 16) {
                skipChunk(static_cast<uint32_t>(chunkSize - 16));
            }

            return true;
        } else {
            // 跳过其他子块
            skipChunk(chunkSize);
        }
    }

    LOGE("fmt chunk not found");
    return false;
}

bool WaveFile::findDataChunk() {
    char chunkId[4];
    uint32_t chunkSize;

    // 查找data子块
    while (file_.good()) {
        file_.read(chunkId, 4);
        if (file_.gcount() != 4) {
            LOGE("Failed to read chunk ID while looking for data");
            return false;
        }

        file_.read(reinterpret_cast<char*>(&chunkSize), 4);
        if (file_.gcount() != 4) {
            LOGE("Failed to read chunk size while looking for data");
            return false;
        }

        if (strncmp(chunkId, "data", 4) == 0) {
            // 找到data子块
            strncpy(header_.dataId, chunkId, 4);
            header_.dataSize = chunkSize;

            LOGD("Found data chunk: size = %u bytes", chunkSize);
            return true;
        } else {
            // 跳过其他子块
            skipChunk(chunkSize);
        }
    }

    LOGE("data chunk not found");
    return false;
}

void WaveFile::skipChunk(uint32_t chunkSize) {
    // 检查chunkSize是否超过streamoff的最大值
    constexpr auto maxStreamOff = static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max());
    if (static_cast<uint64_t>(chunkSize) > maxStreamOff) {
        LOGE("Chunk size too large: %u", chunkSize);
        return;
    }

    file_.seekg(static_cast<std::streamoff>(chunkSize), std::ios::cur);

    // WAV文件要求子块大小为偶数，如果是奇数需要跳过一个填充字节
    if (chunkSize % 2 == 1) {
        file_.seekg(1, std::ios::cur);
    }
}