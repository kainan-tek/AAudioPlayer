#include <aaudio/AAudio.h>
#include <android/log.h>
#include <atomic>
#include <cstdarg>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <jni.h>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// 定义播放模式控制宏
// 取消注释下面这行启用回调模式（使用AAudioStreamBuilder_setDataCallback）
#define CALLBACK_MODE_ENABLE
// 注释掉上面这行使用直接写入模式（使用AAudioStream_write）

// 定义延迟测试相关宏
// #define LATENCY_TEST_ENABLE // 启用延迟测试功能

// AAudio配置常量（方便统一修改）
static const aaudio_direction_t kAudioDirection = AAUDIO_DIRECTION_OUTPUT;
static const aaudio_performance_mode_t kPerformanceMode = AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
static const aaudio_sharing_mode_t kSharingMode = AAUDIO_SHARING_MODE_SHARED; // AAUDIO_SHARING_MODE_EXCLUSIVE
static const aaudio_usage_t kAudioUsage = AAUDIO_USAGE_MEDIA;
static const aaudio_content_type_t kContentType = AAUDIO_CONTENT_TYPE_MUSIC;

// 日志辅助函数前向声明
static void logDebug(const char* message, ...);
static void logError(const char* message, ...);
static void logWarning(const char* message, ...);

// 全局变量
static JavaVM* gJvm = nullptr;
static jobject gPlaybackStateCallback = nullptr;
static jmethodID gOnPlaybackStateChangedMethodId = nullptr;

// AAudio相关变量
static AAudioStream* gAudioStream = nullptr;
static std::atomic<bool> gIsPlaying(false);
static std::mutex gLock;
static std::string gAudioFilePath = "/data/48k_2ch_16bit.wav"; // 注意：这个路径需要根据实际设备调整
static std::ifstream gAudioFile;
static int32_t gSampleRate = 48000;                     // 使用wav文件的采样率
static int32_t gChannelCount = 2;                       // 使用wav文件的通道数
static aaudio_format_t gFormat = AAUDIO_FORMAT_PCM_I16; // 使用wav文件的格式
static int32_t gBytesPerSample = 2;                     // 根据gFormat计算获得

// 直接写入模式相关变量
#ifdef CALLBACK_MODE_ENABLE
// 回调模式：保持原有实现
#else
// 直接写入模式：播放线程变量
static std::thread gPlaybackThread;
static std::atomic<bool> gShouldStopThread(false);
static std::mutex gAudioFileLock;
static const int32_t kBufferDurationMs = 20;        // 每次写入的时长（毫秒）
static int32_t gFramesPerBurst = 0;                 // 每次写入的帧数，kBufferDurationMs * gSampleRate / 1000
static const int32_t kBufferCapacityMultiplier = 4; // 缓冲区容量是单次写入帧数的倍数
static const int64_t kTimeoutNanos = kBufferDurationMs * kBufferCapacityMultiplier * 1000 * 1000; // 超时时间（纳秒）
#endif // CALLBACK_MODE_ENABLE

#ifdef LATENCY_TEST_ENABLE
// 延迟测试配置参数
#define LATENCY_TEST_WRITE_CYCLE 100                           // 每N个回调周期切换一次GPIO状态
#define LATENCY_TEST_GPIO_FILE "/sys/class/gpio/gpio376/value" // GPIO文件路径
#define LATENCY_TEST_GPIO_ACTIVE_HIGH "1"                      // GPIO高电平值
#define LATENCY_TEST_GPIO_ACTIVE_LOW "0"                       // GPIO低电平值

// 延迟测试状态变量
static int32_t gLatencyTestCycleCount = 0;   // 当前回调周期计数
static bool gAudioMuteToggleFlag = false;    // 音频静音切换标志
static bool gLatencyTestInitialized = false; // 延迟测试初始化标志

// 初始化延迟测试
static void initializeLatencyTest() {
    gLatencyTestCycleCount = 0;
    gAudioMuteToggleFlag = false;
    gLatencyTestInitialized = true;
    logDebug("Latency test initialized");
}

// 清理延迟测试资源
__attribute__((unused)) static void cleanupLatencyTest() {
    gLatencyTestInitialized = false;
    logDebug("Latency test resources cleaned up");
}

// 设置GPIO为高电平
static void setGpioHigh() {
    int fd = open(LATENCY_TEST_GPIO_FILE, O_WRONLY);
    if (fd != -1) {
        ssize_t bytesWritten = write(fd, LATENCY_TEST_GPIO_ACTIVE_HIGH, strlen(LATENCY_TEST_GPIO_ACTIVE_HIGH));
        if (bytesWritten < 0) {
            logError("Failed to write high value to GPIO file");
        }
        close(fd);
    } else {
        logError("Failed to open GPIO file: %s, error: %d", LATENCY_TEST_GPIO_FILE, errno);
    }
}

// 设置GPIO为低电平
static void setGpioLow() {
    int fd = open(LATENCY_TEST_GPIO_FILE, O_WRONLY);
    if (fd != -1) {
        ssize_t bytesWritten = write(fd, LATENCY_TEST_GPIO_ACTIVE_LOW, strlen(LATENCY_TEST_GPIO_ACTIVE_LOW));
        if (bytesWritten < 0) {
            logError("Failed to write low value to GPIO file");
        }
        close(fd);
    } else {
        logError("Failed to open GPIO file: %s, error: %d", LATENCY_TEST_GPIO_FILE, errno);
    }
}

// 更新延迟测试状态并控制GPIO
static void updateLatencyTestState() {
    if (!gLatencyTestInitialized) {
        initializeLatencyTest();
    }

    gLatencyTestCycleCount++;

    // 每WRITE_CYCLE个周期切换一次状态
    if (gLatencyTestCycleCount % LATENCY_TEST_WRITE_CYCLE == 0) {
        gAudioMuteToggleFlag = !gAudioMuteToggleFlag;

        // 根据切换标志设置GPIO状态
        if (gAudioMuteToggleFlag) {
            setGpioLow();
        } else {
            setGpioHigh();
        }

        logDebug("Latency test state toggled: cycle=%d, mute=%d", gLatencyTestCycleCount, gAudioMuteToggleFlag);
    }
}

// 检查是否需要静音当前音频数据
static bool shouldMuteAudio() { return gAudioMuteToggleFlag; }
#endif // LATENCY_TEST_ENABLE

// 日志辅助函数
static void logDebug(const char* message, ...) {
    va_list args;
    va_start(args, message);
    __android_log_vprint(ANDROID_LOG_DEBUG, "SimpleAAudio", message, args);
    va_end(args);
}

static void logError(const char* message, ...) {
    va_list args;
    va_start(args, message);
    __android_log_vprint(ANDROID_LOG_ERROR, "SimpleAAudio", message, args);
    va_end(args);
}

static void logWarning(const char* message, ...) {
    va_list args;
    va_start(args, message);
    __android_log_vprint(ANDROID_LOG_WARN, "SimpleAAudio", message, args);
    va_end(args);
}

// 获取每个样本的字节数
static int32_t getBytesPerSample(aaudio_format_t format) {
    switch (format) {
    case AAUDIO_FORMAT_PCM_I16:
        return 2;
    case AAUDIO_FORMAT_PCM_I24_PACKED:
        return 3;
    case AAUDIO_FORMAT_PCM_I32:
    case AAUDIO_FORMAT_PCM_FLOAT:
        return 4;
    default:
        logError("Unknown audio format");
        return 2;
    }
}

#ifdef CALLBACK_MODE_ENABLE
// 回调模式：使用AAudioStreamBuilder_setDataCallback
static aaudio_data_callback_result_t
dataCallback(AAudioStream* stream, void* userData, void* audioData, int32_t numFrames) {
    // 即使gIsPlaying为false，也应该继续处理直到流完全停止
    if (!gAudioFile.is_open()) {
        logError("Audio file not open in callback");
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    int32_t bytesPerFrame = gBytesPerSample * gChannelCount;
    int32_t totalBytes = numFrames * bytesPerFrame;

    // 如果不在播放状态，填充静音
    if (!gIsPlaying.load()) {
        memset(audioData, 0, totalBytes);
        return AAUDIO_CALLBACK_RESULT_CONTINUE; // 继续返回直到流被正确停止
    }

#ifdef LATENCY_TEST_ENABLE
    // 更新延迟测试状态
    updateLatencyTestState();
#endif

    // 从文件读取数据到音频缓冲区
    gAudioFile.read(static_cast<char*>(audioData), totalBytes);
#ifdef LATENCY_TEST_ENABLE
    // 根据延迟测试标志决定是否静音音频
    if (shouldMuteAudio()) {
        memset(audioData, 0, totalBytes); // 填充静音数据
    }
#endif
    std::streamsize bytesRead = gAudioFile.gcount();
    if (bytesRead < totalBytes) {
        // 文件读取完毕，填充静音
        memset(static_cast<char*>(audioData) + bytesRead, 0, totalBytes - bytesRead);
        gIsPlaying.store(false);
        logDebug("Audio file playback completed");

        // 通知Java层播放已停止
        JNIEnv* env = nullptr;
        bool isAttached = false;
        if (gJvm && gJvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
            if (gJvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
                isAttached = true;
            }
        }

        if (env && gPlaybackStateCallback && gOnPlaybackStateChangedMethodId) {
            jobject callbackRef = env->NewLocalRef(gPlaybackStateCallback);
            if (callbackRef) {
                env->CallVoidMethod(callbackRef, gOnPlaybackStateChangedMethodId, JNI_FALSE);
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    logError("Exception occurred while calling playback state callback");
                }
                env->DeleteLocalRef(callbackRef);
            }
        }

        if (isAttached) {
            gJvm->DetachCurrentThread();
        }

        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
#else
// 直接写入模式：播放线程函数（使用AAudioStream_write）
static void playbackThreadFunc() {
    if (!gAudioStream || !gAudioFile.is_open()) {
        logError("Audio stream or file not open in playback thread");
        return;
    }

    int32_t bytesPerFrame = gBytesPerSample * gChannelCount;
    int32_t bufferSize = gFramesPerBurst * bytesPerFrame;
    std::vector<uint8_t> buffer(bufferSize);

    while (gIsPlaying.load() && !gShouldStopThread.load()) {
        // 读取音频数据
        {
            std::lock_guard<std::mutex> lock(gAudioFileLock);
            gAudioFile.read(reinterpret_cast<char*>(buffer.data()), bufferSize);
            std::streamsize bytesRead = gAudioFile.gcount();
            if (bytesRead < bufferSize) {
                // 文件读取完毕，填充静音
                memset(buffer.data() + bytesRead, 0, bufferSize - bytesRead);
                logDebug("Audio file playback completed in direct write mode");

                // 写入剩余数据
                int32_t framesWritten = AAudioStream_write(gAudioStream, buffer.data(), gFramesPerBurst, kTimeoutNanos);
                if (framesWritten < 0) {
                    logError("Error writing to audio stream");
                }

                gIsPlaying.store(false);
                gShouldStopThread.store(true);

                // 通知Java层播放已停止
                JNIEnv* env = nullptr;
                bool isAttached = false;
                if (gJvm && gJvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
                    if (gJvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
                        isAttached = true;
                    }
                }

                if (env && gPlaybackStateCallback && gOnPlaybackStateChangedMethodId) {
                    jobject callbackRef = env->NewLocalRef(gPlaybackStateCallback);
                    if (callbackRef) {
                        env->CallVoidMethod(callbackRef, gOnPlaybackStateChangedMethodId, JNI_FALSE);
                        if (env->ExceptionCheck()) {
                            env->ExceptionClear();
                            logError("Exception occurred while calling playback state callback");
                        }
                        env->DeleteLocalRef(callbackRef);
                    }
                }

                if (isAttached) {
                    gJvm->DetachCurrentThread();
                }

                break;
            }
        }

        // 写入音频数据
        int32_t framesWritten = AAudioStream_write(gAudioStream, buffer.data(), gFramesPerBurst, kTimeoutNanos);
        if (framesWritten < 0) {
            logError("Error writing to audio stream");
            gIsPlaying.store(false);
            gShouldStopThread.store(true);
            break;
        }
    }
}
#endif

// 打开音频文件并读取格式信息
static bool openAudioFile() {
    // 确保之前的文件已关闭
    if (gAudioFile.is_open()) {
        gAudioFile.close();
        logDebug("Audio file already open, closing first");
    }

    // 尝试打开文件
    gAudioFile.open(gAudioFilePath, std::ios::binary);
    if (!gAudioFile.is_open()) {
        logError("Failed to open audio file: %s", gAudioFilePath.c_str());
        return false;
    }

    // 检查文件是否可读
    if (!gAudioFile.good()) {
        logError("Audio file is not readable");
        gAudioFile.close();
        return false;
    }

    // 简单的WAV文件解析
    char header[44];
    gAudioFile.read(header, 44);

    // 检查是否读取了完整的头部
    if (!gAudioFile || gAudioFile.gcount() < 44) {
        logError("Failed to read WAV file header");
        gAudioFile.close();
        return false;
    }

    // 检查是否为WAV文件
    if (header[0] != 'R' || header[1] != 'I' || header[2] != 'F' || header[3] != 'F') {
        logError("Not a valid WAV file");
        gAudioFile.close();
        return false;
    }

    // 读取采样率和通道数
    gSampleRate = *(int32_t*)(header + 24);
    gChannelCount = *(int16_t*)(header + 22);

    // 验证采样率和通道数是否有效
    if (gSampleRate <= 0 || gChannelCount <= 0) {
        logError("Invalid WAV file format: sampleRate=%d, channels=%d", gSampleRate, gChannelCount);
        gAudioFile.close();
        return false;
    }

// 在读取采样率后计算每次写入的帧数（直接写入模式）
#ifndef CALLBACK_MODE_ENABLE
    gFramesPerBurst = kBufferDurationMs * gSampleRate / 1000;
    if (gFramesPerBurst <= 0) {
        logError("Invalid buffer size calculated");
        gAudioFile.close();
        return false;
    }
    logDebug("Calculated frames per burst: %d", gFramesPerBurst);
#endif

    // 读取位深度并设置格式
    int16_t bitsPerSample = *(int16_t*)(header + 34);
    if (bitsPerSample == 16) {
        gFormat = AAUDIO_FORMAT_PCM_I16;
    } else if (bitsPerSample == 24) {
        gFormat = AAUDIO_FORMAT_PCM_I24_PACKED;
    } else if (bitsPerSample == 32) {
        gFormat = AAUDIO_FORMAT_PCM_I32;
    } else {
        logWarning("Unsupported bit depth: %d, using 16-bit instead", bitsPerSample);
        gFormat = AAUDIO_FORMAT_PCM_I16;
    }

    gBytesPerSample = getBytesPerSample(gFormat);

    // 移动到数据部分
    gAudioFile.seekg(44);
    if (!gAudioFile.good()) {
        logError("Failed to seek to audio data section");
        gAudioFile.close();
        return false;
    }

    logDebug("Successfully opened audio file with sample rate: %d, channels: %d, format: %d", gSampleRate,
             gChannelCount, gFormat);
    return true;
}

// 创建并配置AAudio流
static bool createAudioStream() {
    AAudioStreamBuilder* builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        logError("Failed to create stream builder: %d", result);
        return false;
    }

    // 配置AAudio流 - 使用常量便于统一修改配置
    AAudioStreamBuilder_setDirection(builder, kAudioDirection);
    AAudioStreamBuilder_setPerformanceMode(builder, kPerformanceMode);
    AAudioStreamBuilder_setSharingMode(builder, kSharingMode);
    AAudioStreamBuilder_setUsage(builder, kAudioUsage);
    AAudioStreamBuilder_setContentType(builder, kContentType);
    AAudioStreamBuilder_setSampleRate(builder, gSampleRate);
    AAudioStreamBuilder_setChannelCount(builder, gChannelCount);
    AAudioStreamBuilder_setFormat(builder, gFormat);

// 根据模式设置回调或不设置回调
#ifdef CALLBACK_MODE_ENABLE
    // 回调模式：设置数据回调函数
    AAudioStreamBuilder_setDataCallback(builder, dataCallback, nullptr);
#else
    // 直接写入模式：不设置回调函数，使用AAudioStream_write
    // AAudioStream_write的最后一个参数为0表示非阻塞模式
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, gFramesPerBurst * kBufferCapacityMultiplier);
    logDebug("Buffer capacity set to %d frames", gFramesPerBurst * kBufferCapacityMultiplier);
#endif

    // 打开流
    result = AAudioStreamBuilder_openStream(builder, &gAudioStream);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK) {
        logError("Failed to open audio stream: %d", result);
        gAudioStream = nullptr;
        return false;
    }

    // 获取实际的流配置参数
    int32_t actualSampleRate = AAudioStream_getSampleRate(gAudioStream);
    int32_t actualChannelCount = AAudioStream_getChannelCount(gAudioStream);
    aaudio_format_t actualFormat = AAudioStream_getFormat(gAudioStream);
    logDebug("Stream opened with sample rate: %d, channels: %d, format: %d", actualSampleRate, actualChannelCount,
             actualFormat);

    return true;
}

// 开始播放
static bool startPlayback() {
    std::lock_guard<std::mutex> lock(gLock);

    logDebug("Starting playback");

#ifdef LATENCY_TEST_ENABLE
    // 初始化延迟测试状态
    initializeLatencyTest();
#endif

    // 无论当前状态如何，先确保资源已清理
    if (gAudioStream) {
        AAudioStream_requestStop(gAudioStream);
        AAudioStream_close(gAudioStream);
        gAudioStream = nullptr;
        logDebug("Previous audio stream closed");
    }

    if (gAudioFile.is_open()) {
        gAudioFile.close();
        logDebug("Previous audio file closed");
    }

#ifndef CALLBACK_MODE_ENABLE
    // 直接写入模式：确保播放线程已停止
    if (gPlaybackThread.joinable()) {
        gShouldStopThread.store(true);
        gPlaybackThread.join();
        logDebug("Previous playback thread joined");
    }
    gShouldStopThread.store(false);
#endif

    // 在打开资源前就设置播放状态为true，避免回调问题
    gIsPlaying.store(true);
    logDebug("Set playback state to true");

    // 打开音频文件
    if (!openAudioFile()) {
        logError("Failed to open audio file");
        gIsPlaying.store(false);
        return false;
    }

    // 创建AAudio流
    if (!createAudioStream()) {
        logError("Failed to create audio stream");
        gAudioFile.close();
        gIsPlaying.store(false);
        return false;
    }

    // 启动流
    aaudio_result_t result = AAudioStream_requestStart(gAudioStream);
    if (result != AAUDIO_OK) {
        logError("Failed to start audio stream: %d", result);
        AAudioStream_close(gAudioStream);
        gAudioStream = nullptr;
        gAudioFile.close();
        gIsPlaying.store(false);
        return false;
    }
    logDebug("Audio stream started successfully");

#ifndef CALLBACK_MODE_ENABLE
    // 直接写入模式：启动播放线程
    gPlaybackThread = std::thread(playbackThreadFunc);
    logDebug("Playback thread started");
#endif

    logDebug("Playback started successfully");
    return true;
}

// 停止播放
static bool stopPlayback() {
    std::lock_guard<std::mutex> lock(gLock);

    logDebug("Stopping playback");

    // 先设置状态为停止
    gIsPlaying.store(false);

#ifndef CALLBACK_MODE_ENABLE
    // 直接写入模式：停止播放线程
    gShouldStopThread.store(true);
    if (gPlaybackThread.joinable()) {
        try {
            gPlaybackThread.join();
            logDebug("Playback thread joined");
        } catch (const std::system_error& e) {
            logError("Error joining playback thread: %s", e.what());
        }
    }
#endif

    // 然后清理资源
    if (gAudioStream) {
        aaudio_result_t result = AAudioStream_requestStop(gAudioStream);
        if (result != AAUDIO_OK) {
            logError("Failed to request stop audio stream: %d", result);
        }

        result = AAudioStream_close(gAudioStream);
        if (result != AAUDIO_OK) {
            logError("Failed to close audio stream: %d", result);
        }
        gAudioStream = nullptr;
        logDebug("Audio stream closed");
    }

    if (gAudioFile.is_open()) {
        gAudioFile.close();
        logDebug("Audio file closed");
    }

    return true;
}

// JNI方法 - 开始播放
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_aaudioplayer_SimpleAudioPlayer_startPlaybackNative(JNIEnv* env, jobject thiz) {
    return startPlayback() ? JNI_TRUE : JNI_FALSE;
}

// JNI方法 - 停止播放
extern "C" JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_SimpleAudioPlayer_stopPlaybackNative(JNIEnv* env,
                                                                                                         jobject thiz) {
    return stopPlayback() ? JNI_TRUE : JNI_FALSE;
}

// JNI方法 - 设置播放状态回调
extern "C" JNIEXPORT void JNICALL Java_com_example_aaudioplayer_SimpleAudioPlayer_setPlaybackStateCallbackNative(
    JNIEnv* env, jobject thiz, jobject callback) {
    // 保存JavaVM
    env->GetJavaVM(&gJvm);

    // 释放之前的回调
    if (gPlaybackStateCallback) {
        env->DeleteGlobalRef(gPlaybackStateCallback);
        gPlaybackStateCallback = nullptr;
        logDebug("Previous callback released");
    }

    if (callback) {
        gPlaybackStateCallback = env->NewGlobalRef(callback);
        jclass callbackClass = env->GetObjectClass(callback);
        gOnPlaybackStateChangedMethodId = env->GetMethodID(callbackClass, "onPlaybackStateChanged", "(Z)V");
        logDebug("Callback set successfully");
    } else {
        gOnPlaybackStateChangedMethodId = nullptr;
        logDebug("Callback cleared");
    }
}

// JNI方法 - 释放资源
extern "C" JNIEXPORT void JNICALL Java_com_example_aaudioplayer_SimpleAudioPlayer_releaseResourcesNative(JNIEnv* env,
                                                                                                         jobject thiz) {
    stopPlayback();

    if (gPlaybackStateCallback) {
        env->DeleteGlobalRef(gPlaybackStateCallback);
        gPlaybackStateCallback = nullptr;
    }

    gOnPlaybackStateChangedMethodId = nullptr;
}