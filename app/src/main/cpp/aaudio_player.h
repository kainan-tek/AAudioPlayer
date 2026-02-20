#ifndef AAUDIO_PLAYER_H
#define AAUDIO_PLAYER_H

#include <android/log.h>
#include <cstdint>
#include <fstream>
#include <jni.h>
#include <string>

// Log macros
#define LOG_TAG "AAudioPlayer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * AAudio Player JNI Interface
 *
 * This header defines the JNI interface for the AAudio player functionality.
 * It provides functions to initialize, control, and manage audio playback
 * using Android's AAudio API with WAV file support.
 */

/**
 * Initialize the audio player with the specified file path
 * @param env JNI environment
 * @param thiz Java object instance
 * @param filePath Path to the audio file to play
 * @return JNI_TRUE if initialization successful, JNI_FALSE otherwise
 */
JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_initializeNative(JNIEnv* env,
                                                                                              jobject thiz,
                                                                                              jstring filePath);

/**
 * Start audio playback
 * @param env JNI environment
 * @param thiz Java object instance
 * @return JNI_TRUE if playback started successfully, JNI_FALSE otherwise
 */
JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_startNativePlayback(JNIEnv* env,
                                                                                                 jobject thiz);

/**
 * Stop audio playback
 * @param env JNI environment
 * @param thiz Java object instance
 */
JNIEXPORT void JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_stopNativePlayback(JNIEnv* env, jobject thiz);

/**
 * Release audio player resources
 * @param env JNI environment
 * @param thiz Java object instance
 */
JNIEXPORT void JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_releaseNative(JNIEnv* env, jobject thiz);

/**
 * Set native audio configuration
 * @param env JNI environment
 * @param thiz Java object instance
 * @param usage Audio usage integer value
 * @param contentType Audio content type integer value
 * @param performanceMode Performance mode integer value
 * @param sharingMode Sharing mode integer value
 * @param filePath Audio file path
 * @return JNI_TRUE if configuration set successfully, JNI_FALSE otherwise
 */
JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_setNativeConfig(
    JNIEnv* env, jobject thiz, jint usage, jint contentType, jint performanceMode, jint sharingMode, jstring filePath);

#ifdef __cplusplus
}

/**
 * WAV file management class
 * Supports WAV file reading, parsing and audio data extraction
 */
class WaveFile {
public:
    // WAV file header structure
    struct WaveHeader {
        // RIFF header
        char riffId[4];    // "RIFF"
        uint32_t riffSize; // File size - 8
        char waveId[4];    // "WAVE"

        // fmt subchunk
        char fmtId[4];          // "fmt "
        uint16_t audioFormat;   // Audio format (1 = PCM)
        uint16_t numChannels;   // Channel count
        uint32_t sampleRate;    // Sample rate
        uint32_t byteRate;      // Byte rate
        uint16_t blockAlign;    // Block align
        uint16_t bitsPerSample; // Bits per sample

        // data subchunk
        char dataId[4];    // "data"
        uint32_t dataSize; // Audio data size
    };

    /**
     * Constructor
     */
    WaveFile();

    /**
     * Destructor
     */
    ~WaveFile() noexcept;

    // Disable copy and assignment
    WaveFile(const WaveFile&) = delete;
    WaveFile& operator=(const WaveFile&) = delete;

    // Allow move
    WaveFile(WaveFile&&) noexcept = default;
    WaveFile& operator=(WaveFile&&) noexcept = default;

    /**
     * Open WAV file
     * @param filePath File path
     * @return Returns true on success, false on failure
     */
    bool open(const std::string& filePath);

    /**
     * Close file
     */
    void close();

    /**
     * Read audio data
     * @param buffer Data buffer
     * @param bufferSize Buffer size (bytes)
     * @return Actual bytes read
     */
    size_t readAudioData(void* buffer, size_t bufferSize);

    bool isOpen() const;

    // Safe getter methods
    int32_t getSampleRate() const { return static_cast<int32_t>(header_.sampleRate); }
    int32_t getChannelCount() const { return static_cast<int32_t>(header_.numChannels); }

    /**
     * Get corresponding AAudio format
     * @return AAudio format enumeration value
     */
    int32_t getAAudioFormat() const;

    std::string getFormatInfo() const;

    /**
     * Validate WAV file format
     * @return Returns true if format is correct
     */
    bool isValidFormat() const;

private:
    std::ifstream file_;
    WaveHeader header_{};
    bool isOpen_;

    /**
     * Read and validate WAV file header
     * @return Returns true on success
     */
    bool readHeader();

    /**
     * Validate RIFF header
     * @return Returns true if validation passes
     */
    bool validateRiffHeader();

    /**
     * Find and read fmt subchunk
     * @return Returns true on success
     */
    bool readFmtChunk();

    /**
     * Find and locate data subchunk
     * @return Returns true on success
     */
    bool findDataChunk();

    /**
     * Skip unknown subchunks
     * @param chunkSize Subchunk size
     */
    void skipChunk(uint32_t chunkSize);
};

#endif

#endif // AAUDIO_PLAYER_H