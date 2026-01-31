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

// Latency test configuration
#define LATENCY_TEST_ENABLE 0

#if LATENCY_TEST_ENABLE
#define LATENCY_TEST_GPIO_FILE "/sys/class/gpio/gpio376/value"
#define LATENCY_TEST_INTERVAL 100 // Toggle every 100 writes

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

// AAudio Player implementation
struct AudioPlayerState {
    AAudioStream* stream = nullptr;
    std::unique_ptr<WaveFile> waveFile;
    std::atomic<bool> isPlaying{false};

    // Java callback related
    JavaVM* jvm = nullptr;
    jobject playerInstance = nullptr;
    jmethodID onPlaybackStartedMethod = nullptr;
    jmethodID onPlaybackStoppedMethod = nullptr;
    jmethodID onPlaybackErrorMethod = nullptr;

    // Configuration parameters
    aaudio_usage_t usage = AAUDIO_USAGE_MEDIA;
    aaudio_content_type_t contentType = AAUDIO_CONTENT_TYPE_MUSIC;
    aaudio_performance_mode_t performanceMode = AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
    aaudio_sharing_mode_t sharingMode = AAUDIO_SHARING_MODE_SHARED;
    std::string audioFilePath = "/data/48k_2ch_16bit.wav";

#if LATENCY_TEST_ENABLE
    // Latency test variables
    std::atomic<int> writeCounter{0};
    std::atomic<bool> gpioState{false};
    std::atomic<bool> muteAudio{false};
    std::atomic<bool> latencyTestEnabled{false}; // 控制是否启用latency测试
    int gpioFd = -1;                             // Pre-opened GPIO file descriptor
#endif
};

static AudioPlayerState g_player;

#if LATENCY_TEST_ENABLE
// Optimized GPIO control functions for latency test
static bool initGpio() {
    g_player.gpioFd = open(LATENCY_TEST_GPIO_FILE, O_WRONLY);
    if (g_player.gpioFd < 0) {
        LOGE("Failed to open GPIO file: %s, errno: %d", LATENCY_TEST_GPIO_FILE, errno);
        return false;
    }
    LOGI("GPIO file opened successfully: fd=%d", g_player.gpioFd);
    return true;
}

static void closeGpio() {
    if (g_player.gpioFd >= 0) {
        close(g_player.gpioFd);
        g_player.gpioFd = -1;
    }
}

static inline bool writeGpioValue(int value) {
    if (g_player.gpioFd < 0) {
        return false;
    }

    char buf = value ? '1' : '0';
    ssize_t result = write(g_player.gpioFd, &buf, 1);

    if (result != 1) {
        LOGE("Failed to write GPIO value: %d, result: %zd, errno: %d", value, result, errno);
        return false;
    }

    return true;
}

static inline void toggleGpio() {
    bool currentState = g_player.gpioState.load(std::memory_order_relaxed);
    bool newState = !currentState;

    if (writeGpioValue(newState ? 1 : 0)) {
        g_player.gpioState.store(newState, std::memory_order_relaxed);
        // Remove debug log in callback to reduce overhead
        // LOGD("GPIO toggled to: %d", newState ? 1 : 0);
    }
}
#endif

// Java callback functions
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

// Audio callback
static aaudio_data_callback_result_t
audioCallback(AAudioStream* stream, void* userData, void* audioData, int32_t numFrames) {
    if (!g_player.isPlaying.load()) {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    if (!g_player.waveFile || !g_player.waveFile->isOpen()) {
        g_player.isPlaying.store(false);
        notifyPlaybackError("Audio file not opened");
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    // Calculate required bytes
    int32_t channelCount = AAudioStream_getChannelCount(stream);
    int32_t bytesPerSample = (AAudioStream_getFormat(stream) == AAUDIO_FORMAT_PCM_I16) ? 2 : 4;
    int32_t bytesToRead = numFrames * channelCount * bytesPerSample;

    size_t bytesRead = g_player.waveFile->readAudioData(audioData, bytesToRead);

    if (bytesRead < static_cast<size_t>(bytesToRead)) {
        // Playback completed
        g_player.isPlaying.store(false);
        notifyPlaybackStopped();
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

#if LATENCY_TEST_ENABLE
    // Latency test logic - only execute if enabled
    if (g_player.latencyTestEnabled.load(std::memory_order_relaxed)) {
        int currentCount = g_player.writeCounter.fetch_add(1);

        if (currentCount % LATENCY_TEST_INTERVAL == 0) {
            // Toggle GPIO every LATENCY_TEST_INTERVAL writes
            toggleGpio();

            // Toggle audio mute state
            bool currentMuteState = g_player.muteAudio.load(std::memory_order_relaxed);
            g_player.muteAudio.store(!currentMuteState, std::memory_order_relaxed);

            // Reduce logging in callback for better performance
            // Only log every 1000 intervals to avoid spam
            if (currentCount % (LATENCY_TEST_INTERVAL * 1000) == 0) {
                LOGD("Latency test: count=%d, gpio=%d, mute=%d", currentCount,
                     g_player.gpioState.load(std::memory_order_relaxed) ? 1 : 0, !currentMuteState ? 1 : 0);
            }
        }

        // Apply mute if enabled
        if (g_player.muteAudio.load(std::memory_order_relaxed)) {
            memset(audioData, 0, bytesToRead);
        }
    }
#endif

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

// Error callback
static void errorCallback(AAudioStream* stream, void* userData, aaudio_result_t error) {
    LOGE("AAudio error: %s", AAudio_convertResultToText(error));
    g_player.isPlaying.store(false);
    std::string errorMsg = "Playback stream error: ";
    errorMsg += AAudio_convertResultToText(error);
    notifyPlaybackError(errorMsg);
}

// Create AAudio stream
static bool createAAudioStream() {
    AAudioStreamBuilder* builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGE("Failed to create builder: %s", AAudio_convertResultToText(result));
        return false;
    }

    // Use WAV file parameters or default values
    int32_t sampleRate = 48000;
    int32_t channelCount = 2;
    aaudio_format_t format = AAUDIO_FORMAT_PCM_I16;

    if (g_player.waveFile && g_player.waveFile->isOpen()) {
        sampleRate = g_player.waveFile->getSampleRate();
        channelCount = g_player.waveFile->getChannelCount();
        format = static_cast<aaudio_format_t>(g_player.waveFile->getAAudioFormat());
    }

    // Configure stream
    AAudioStreamBuilder_setSampleRate(builder, sampleRate);
    AAudioStreamBuilder_setChannelCount(builder, channelCount);
    AAudioStreamBuilder_setFormat(builder, format);
    AAudioStreamBuilder_setUsage(builder, g_player.usage);
    AAudioStreamBuilder_setContentType(builder, g_player.contentType);
    AAudioStreamBuilder_setSharingMode(builder, g_player.sharingMode);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setPerformanceMode(builder, g_player.performanceMode);

    // Buffer configuration
    int32_t bufferCapacity = (g_player.performanceMode == AAUDIO_PERFORMANCE_MODE_LOW_LATENCY)
                                 ? (sampleRate * 40) / 1000   // 40ms for low latency
                                 : (sampleRate * 100) / 1000; // 100ms for power saving
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, bufferCapacity);

    // Set callbacks
    AAudioStreamBuilder_setDataCallback(builder, audioCallback, nullptr);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);

    // Create stream
    result = AAudioStreamBuilder_openStream(builder, &g_player.stream);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK) {
        LOGE("Failed to open stream: %s", AAudio_convertResultToText(result));
        return false;
    }

    // Optimize buffer size
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

// JNI method implementations
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

    // Save JVM reference
    env->GetJavaVM(&g_player.jvm);

    // Save Java object reference
    if (g_player.playerInstance) {
        env->DeleteGlobalRef(g_player.playerInstance);
    }
    g_player.playerInstance = env->NewGlobalRef(thiz);

    // Get callback method IDs
    jclass clazz = env->GetObjectClass(thiz);
    g_player.onPlaybackStartedMethod = env->GetMethodID(clazz, "onNativePlaybackStarted", "()V");
    g_player.onPlaybackStoppedMethod = env->GetMethodID(clazz, "onNativePlaybackStopped", "()V");
    g_player.onPlaybackErrorMethod = env->GetMethodID(clazz, "onNativePlaybackError", "(Ljava/lang/String;)V");

    if (!g_player.onPlaybackStartedMethod || !g_player.onPlaybackStoppedMethod || !g_player.onPlaybackErrorMethod) {
        LOGE("Failed to get callback method IDs");
        return JNI_FALSE;
    }

    // Get file path
    if (filePath) {
        const char* path = env->GetStringUTFChars(filePath, nullptr);
        g_player.audioFilePath = std::string(path);
        env->ReleaseStringUTFChars(filePath, path);
    }

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_setNativeConfig(JNIEnv* env,
                                                                                             jobject thiz,
                                                                                             jint usage,
                                                                                             jint contentType,
                                                                                             jint performanceMode,
                                                                                             jint sharingMode,
                                                                                             jstring filePath) {
    LOGI("setNativeConfig");

    // Update configuration parameters - direct integer assignment
    g_player.usage = static_cast<aaudio_usage_t>(usage);
    g_player.contentType = static_cast<aaudio_content_type_t>(contentType);
    g_player.performanceMode = static_cast<aaudio_performance_mode_t>(performanceMode);
    g_player.sharingMode = static_cast<aaudio_sharing_mode_t>(sharingMode);

    if (filePath) {
        const char* path = env->GetStringUTFChars(filePath, nullptr);
        g_player.audioFilePath = std::string(path);
        env->ReleaseStringUTFChars(filePath, path);
    }

    LOGI("Config updated: usage=%d, contentType=%d, performanceMode=%d, sharingMode=%d, file=%s", 
         g_player.usage, g_player.contentType, g_player.performanceMode, g_player.sharingMode, 
         g_player.audioFilePath.c_str());

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_startNativePlayback(JNIEnv* env,
                                                                                                 jobject thiz) {
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

#if LATENCY_TEST_ENABLE
    // Initialize latency test
    if (!initGpio()) {
        LOGE("Failed to initialize GPIO for latency test - latency test DISABLED");
        g_player.latencyTestEnabled.store(false);
        // Continue without latency test functionality
    } else {
        // Reset latency test counters
        g_player.writeCounter.store(0);
        g_player.gpioState.store(false);
        g_player.muteAudio.store(false);
        g_player.latencyTestEnabled.store(true);

        // Initialize GPIO to low state
        writeGpioValue(0);
        LOGI("Latency test initialized: GPIO=%s, interval=%d", LATENCY_TEST_GPIO_FILE, LATENCY_TEST_INTERVAL);
    }
#endif

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

#if LATENCY_TEST_ENABLE
    // Close GPIO file descriptor
    closeGpio();
#endif

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

#if LATENCY_TEST_ENABLE
    // Close GPIO file descriptor
    closeGpio();
#endif

    // Clean up Java references
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
    close(); // Ensure previous file is closed

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

    // Check if bufferSize exceeds maximum streamsize value
    constexpr auto maxStreamSize = static_cast<size_t>(std::numeric_limits<std::streamsize>::max());
    size_t actualReadSize = std::min(bufferSize, maxStreamSize);

    auto readSize = static_cast<std::streamsize>(actualReadSize);
    file_.read(static_cast<char*>(buffer), readSize);
    auto bytesRead = static_cast<size_t>(file_.gcount());

    if (bytesRead < bufferSize) {
        // If insufficient data is read, fill remaining part with zeros
        memset(static_cast<char*>(buffer) + bytesRead, 0, bufferSize - bytesRead);
    }

    return bytesRead;
}

bool WaveFile::isOpen() const { return isOpen_; }

int32_t WaveFile::getAAudioFormat() const {
    // Return AAudio format based on WAV file bit depth
    // Return int32_t to avoid including AAudio header files
    // AAudio format constant values:
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
        return 1; // Default return 16-bit format
    }
}

std::string WaveFile::getFormatInfo() const {
    std::ostringstream oss;
    oss << static_cast<int32_t>(header_.sampleRate) << "Hz, " << static_cast<int32_t>(header_.numChannels)
        << " channels, " << static_cast<int32_t>(header_.bitsPerSample) << " bits, PCM";
    return oss.str();
}

bool WaveFile::isValidFormat() const {
    return (header_.audioFormat == 1 &&                               // PCM format
            header_.numChannels > 0 && header_.numChannels <= 16 &&   // Reasonable channel count
            header_.sampleRate > 0 && header_.sampleRate <= 192000 && // Reasonable sample rate
            (header_.bitsPerSample == 8 || header_.bitsPerSample == 16 || header_.bitsPerSample == 24 ||
             header_.bitsPerSample == 32) && // Supported bit depth
            header_.dataSize > 0);           // Has audio data
}

bool WaveFile::readHeader() {
    file_.seekg(0, std::ios::beg);
    return validateRiffHeader() && readFmtChunk() && findDataChunk();
}

bool WaveFile::validateRiffHeader() {
    // Read RIFF identifier
    file_.read(header_.riffId, 4);
    if (file_.gcount() != 4 || strncmp(header_.riffId, "RIFF", 4) != 0) {
        LOGE("Invalid RIFF header");
        return false;
    }

    // Read file size
    file_.read(reinterpret_cast<char*>(&header_.riffSize), 4);
    if (file_.gcount() != 4) {
        LOGE("Failed to read RIFF size");
        return false;
    }

    // Read WAVE identifier
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

    // Find fmt subchunk
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
            // Found fmt subchunk
            strncpy(header_.fmtId, chunkId, 4);

            // Read fmt data
            file_.read(reinterpret_cast<char*>(&header_.audioFormat), 2);
            file_.read(reinterpret_cast<char*>(&header_.numChannels), 2);
            file_.read(reinterpret_cast<char*>(&header_.sampleRate), 4);
            file_.read(reinterpret_cast<char*>(&header_.byteRate), 4);
            file_.read(reinterpret_cast<char*>(&header_.blockAlign), 2);
            file_.read(reinterpret_cast<char*>(&header_.bitsPerSample), 2);

            // Skip extra fmt data (if any)
            if (chunkSize > 16) {
                skipChunk(static_cast<uint32_t>(chunkSize - 16));
            }

            return true;
        } else {
            // Skip other subchunks
            skipChunk(chunkSize);
        }
    }

    LOGE("fmt chunk not found");
    return false;
}

bool WaveFile::findDataChunk() {
    char chunkId[4];
    uint32_t chunkSize;

    // Find data subchunk
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
            // Found data subchunk
            strncpy(header_.dataId, chunkId, 4);
            header_.dataSize = chunkSize;

            LOGD("Found data chunk: size = %u bytes", chunkSize);
            return true;
        } else {
            // Skip other subchunks
            skipChunk(chunkSize);
        }
    }

    LOGE("data chunk not found");
    return false;
}

void WaveFile::skipChunk(uint32_t chunkSize) {
    // Check if chunkSize exceeds maximum streamoff value
    constexpr auto maxStreamOff = static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max());
    if (static_cast<uint64_t>(chunkSize) > maxStreamOff) {
        LOGE("Chunk size too large: %u", chunkSize);
        return;
    }

    file_.seekg(static_cast<std::streamoff>(chunkSize), std::ios::cur);

    // WAV files require subchunk size to be even, if odd need to skip one padding byte
    if (chunkSize % 2 == 1) {
        file_.seekg(1, std::ios::cur);
    }
}