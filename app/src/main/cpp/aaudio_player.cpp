#include "aaudio_player.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <limits>
#include <memory>
#include <sstream>
#include <string>

#include <aaudio/AAudio.h>

#define LATENCY_TEST_ENABLE 0

#if LATENCY_TEST_ENABLE
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

#define LATENCY_TEST_GPIO_FILE "/sys/class/gpio/gpio376/value"
#define LATENCY_TEST_INTERVAL 100
#endif

struct AudioPlayerState {
    AAudioStream* stream = nullptr;
    std::unique_ptr<WavFile> wav_file;
    std::atomic<bool> is_playing{false};

    JavaVM* jvm = nullptr;
    jobject player_instance = nullptr;
    jmethodID on_playback_started_method = nullptr;
    jmethodID on_playback_stopped_method = nullptr;
    jmethodID on_playback_error_method = nullptr;

    aaudio_usage_t usage = AAUDIO_USAGE_MEDIA;
    aaudio_content_type_t content_type = AAUDIO_CONTENT_TYPE_MUSIC;
    aaudio_performance_mode_t performance_mode = AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
    aaudio_sharing_mode_t sharing_mode = AAUDIO_SHARING_MODE_SHARED;
    std::string audio_file_path = "/data/48k_2ch_16bit.wav";

#if LATENCY_TEST_ENABLE
    std::atomic<int> write_counter{0};
    std::atomic<bool> gpio_state{false};
    std::atomic<bool> mute_audio{false};
    std::atomic<bool> latency_test_enabled{false};
    int gpio_fd = -1;
#endif
};

namespace {

AudioPlayerState g_player;

}  // namespace

#if LATENCY_TEST_ENABLE
static bool initGpio() {
    g_player.gpio_fd = open(LATENCY_TEST_GPIO_FILE, O_WRONLY);
    if (g_player.gpio_fd < 0) {
        LOGE("Failed to open GPIO file: %s, errno: %d", LATENCY_TEST_GPIO_FILE, errno);
        return false;
    }
    LOGI("GPIO file opened successfully: fd=%d", g_player.gpio_fd);
    return true;
}

static void closeGpio() {
    if (g_player.gpio_fd >= 0) {
        close(g_player.gpio_fd);
        g_player.gpio_fd = -1;
    }
}

static inline bool writeGpioValue(int value) {
    if (g_player.gpio_fd < 0) {
        return false;
    }

    char buf = value ? '1' : '0';
    ssize_t result = write(g_player.gpio_fd, &buf, 1);

    if (result != 1) {
        LOGE("Failed to write GPIO value: %d, result: %zd, errno: %d", value, result, errno);
        return false;
    }

    return true;
}

static inline void toggleGpio() {
    bool current_state = g_player.gpio_state.load(std::memory_order_relaxed);
    bool new_state = !current_state;

    if (writeGpioValue(new_state ? 1 : 0)) {
        g_player.gpio_state.store(new_state, std::memory_order_relaxed);
    }
}
#endif

static void notifyPlaybackStarted() {
    if (g_player.jvm && g_player.player_instance && g_player.on_playback_started_method) {
        JNIEnv* env;
        if (g_player.jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
            env->CallVoidMethod(g_player.player_instance, g_player.on_playback_started_method);
        }
    }
}

static void notifyPlaybackStopped() {
    if (g_player.jvm && g_player.player_instance && g_player.on_playback_stopped_method) {
        JNIEnv* env;
        if (g_player.jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
            env->CallVoidMethod(g_player.player_instance, g_player.on_playback_stopped_method);
        }
    }
}

static void notifyPlaybackError(const std::string& error) {
    if (g_player.jvm && g_player.player_instance && g_player.on_playback_error_method) {
        JNIEnv* env;
        if (g_player.jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
            jstring error_str = env->NewStringUTF(error.c_str());
            env->CallVoidMethod(g_player.player_instance, g_player.on_playback_error_method, error_str);
            env->DeleteLocalRef(error_str);
        }
    }
}

static aaudio_data_callback_result_t audioCallback(AAudioStream* stream,
                                                   void* userData,
                                                   void* audioData,
                                                   int32_t numFrames) {
    if (!g_player.is_playing.load(std::memory_order_acquire)) {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    if (!g_player.wav_file || !g_player.wav_file->isOpen()) {
        g_player.is_playing.store(false, std::memory_order_release);
        notifyPlaybackError("[FILE] Audio file not opened");
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    // Validate numFrames
    if (numFrames <= 0) {
        LOGE("Invalid numFrames: %d", numFrames);
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    int32_t channel_count = AAudioStream_getChannelCount(stream);
    if (channel_count <= 0 || channel_count > 16) {
        LOGE("Invalid channel count: %d", channel_count);
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    // Get bytes per sample based on format
    int32_t bytes_per_sample;
    switch (AAudioStream_getFormat(stream)) {
        case AAUDIO_FORMAT_PCM_I16:
            bytes_per_sample = 2;
            break;
        case AAUDIO_FORMAT_PCM_I24_PACKED:
            bytes_per_sample = 3;
            break;
        case AAUDIO_FORMAT_PCM_I32:
        case AAUDIO_FORMAT_PCM_FLOAT:
            bytes_per_sample = 4;
            break;
        default:
            bytes_per_sample = 2;
            break;
    }

    // Calculate bytes to read
    int32_t bytes_to_read = numFrames * channel_count * bytes_per_sample;

    // Clear buffer first to prevent residual data
    memset(audioData, 0, static_cast<size_t>(bytes_to_read));

    size_t bytes_read = g_player.wav_file->readAudioData(audioData, static_cast<size_t>(bytes_to_read));

    // bytes_read == 0 indicates end of file
    if (bytes_read == 0) {
        g_player.is_playing.store(false, std::memory_order_release);
        notifyPlaybackStopped();
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    // Read error (negative value)
    if (bytes_read < 0) {
        g_player.is_playing.store(false, std::memory_order_release);
        notifyPlaybackError("[FILE] Audio read error");
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    // Partial read (file ending), buffer already zero-filled
    // Continue playing the remaining data, next callback will detect EOF
    if (bytes_read < static_cast<size_t>(bytes_to_read)) {
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }

#if LATENCY_TEST_ENABLE
    if (g_player.latency_test_enabled.load(std::memory_order_relaxed)) {
        int current_count = g_player.write_counter.fetch_add(1);

        if (current_count % LATENCY_TEST_INTERVAL == 0) {
            toggleGpio();

            bool current_mute_state = g_player.mute_audio.load(std::memory_order_relaxed);
            g_player.mute_audio.store(!current_mute_state, std::memory_order_relaxed);

            if (current_count % (LATENCY_TEST_INTERVAL * 1000) == 0) {
                LOGD("Latency test: count=%d, gpio=%d, mute=%d", current_count,
                     g_player.gpio_state.load(std::memory_order_relaxed) ? 1 : 0, !current_mute_state ? 1 : 0);
            }
        }

        if (g_player.mute_audio.load(std::memory_order_relaxed)) {
            memset(audioData, 0, bytes_to_read);
        }
    }
#endif

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

static void errorCallback(AAudioStream* stream, void* userData, aaudio_result_t error) {
    LOGE("AAudio error: %s", AAudio_convertResultToText(error));
    g_player.is_playing.store(false, std::memory_order_release);
    std::string error_msg = "[STREAM] Playback stream error: ";
    error_msg += AAudio_convertResultToText(error);
    notifyPlaybackError(error_msg);
}

static bool createAAudioStream() {
    AAudioStreamBuilder* builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGE("Failed to create builder: %s", AAudio_convertResultToText(result));
        return false;
    }

    int32_t sample_rate = 48000;
    int32_t channel_count = 2;
    aaudio_format_t format = AAUDIO_FORMAT_PCM_I16;

    if (g_player.wav_file && g_player.wav_file->isOpen()) {
        sample_rate = g_player.wav_file->getSampleRate();
        channel_count = g_player.wav_file->getChannelCount();
        format = static_cast<aaudio_format_t>(g_player.wav_file->getAAudioFormat());
    }

    AAudioStreamBuilder_setSampleRate(builder, sample_rate);
    AAudioStreamBuilder_setChannelCount(builder, channel_count);
    AAudioStreamBuilder_setFormat(builder, format);
    AAudioStreamBuilder_setUsage(builder, g_player.usage);
    AAudioStreamBuilder_setContentType(builder, g_player.content_type);
    AAudioStreamBuilder_setSharingMode(builder, g_player.sharing_mode);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setPerformanceMode(builder, g_player.performance_mode);

    int32_t buffer_capacity = (g_player.performance_mode == AAUDIO_PERFORMANCE_MODE_LOW_LATENCY)
                                  ? (sample_rate * 40) / 1000
                                  : (sample_rate * 100) / 1000;
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, buffer_capacity);

    AAudioStreamBuilder_setDataCallback(builder, audioCallback, nullptr);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);

    result = AAudioStreamBuilder_openStream(builder, &g_player.stream);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK) {
        LOGE("Failed to open stream: %s", AAudio_convertResultToText(result));
        return false;
    }

    int32_t frames_per_burst = AAudioStream_getFramesPerBurst(g_player.stream);
    if (frames_per_burst > 0) {
        int32_t optimal_size =
            frames_per_burst * (g_player.performance_mode == AAUDIO_PERFORMANCE_MODE_LOW_LATENCY ? 2 : 4);
        optimal_size = std::min(optimal_size, AAudioStream_getBufferCapacityInFrames(g_player.stream));
        AAudioStream_setBufferSizeInFrames(g_player.stream, optimal_size);
    }

    LOGI("Stream created: %dHz, %dch, format=%d, mode=%d", AAudioStream_getSampleRate(g_player.stream),
         AAudioStream_getChannelCount(g_player.stream), AAudioStream_getFormat(g_player.stream),
         AAudioStream_getPerformanceMode(g_player.stream));

    return true;
}

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

    if (g_player.player_instance) {
        env->DeleteGlobalRef(g_player.player_instance);
    }
    g_player.player_instance = env->NewGlobalRef(thiz);

    jclass clazz = env->GetObjectClass(thiz);
    g_player.on_playback_started_method = env->GetMethodID(clazz, "onNativePlaybackStarted", "()V");
    g_player.on_playback_stopped_method = env->GetMethodID(clazz, "onNativePlaybackStopped", "()V");
    g_player.on_playback_error_method = env->GetMethodID(clazz, "onNativePlaybackError", "(Ljava/lang/String;)V");

    if (!g_player.on_playback_started_method || !g_player.on_playback_stopped_method ||
        !g_player.on_playback_error_method) {
        LOGE("Failed to get callback method IDs");
        return JNI_FALSE;
    }

    if (filePath) {
        const char* path = env->GetStringUTFChars(filePath, nullptr);
        g_player.audio_file_path = std::string(path);
        env->ReleaseStringUTFChars(filePath, path);
    }

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_setNativeConfig(
    JNIEnv* env, jobject thiz, jint usage, jint contentType, jint performanceMode, jint sharingMode, jstring filePath) {
    LOGI("setNativeConfig");

    g_player.usage = static_cast<aaudio_usage_t>(usage);
    g_player.content_type = static_cast<aaudio_content_type_t>(contentType);
    g_player.performance_mode = static_cast<aaudio_performance_mode_t>(performanceMode);
    g_player.sharing_mode = static_cast<aaudio_sharing_mode_t>(sharingMode);

    if (filePath) {
        const char* path = env->GetStringUTFChars(filePath, nullptr);
        g_player.audio_file_path = std::string(path);
        env->ReleaseStringUTFChars(filePath, path);
    }

    LOGI("Config updated: usage=%d, contentType=%d, performanceMode=%d, sharingMode=%d, file=%s", g_player.usage,
         g_player.content_type, g_player.performance_mode, g_player.sharing_mode, g_player.audio_file_path.c_str());

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_startNativePlayback(JNIEnv* env,
                                                                                                 jobject thiz) {
    LOGI("startNativePlayback");

    if (g_player.is_playing.load(std::memory_order_acquire)) {
        return JNI_FALSE;
    }

    g_player.wav_file = std::make_unique<WavFile>();
    if (!g_player.wav_file->open(g_player.audio_file_path)) {
        LOGE("Failed to open: %s", g_player.audio_file_path.c_str());
        g_player.wav_file.reset();
        notifyPlaybackError("[FILE] Cannot open audio file");
        return JNI_FALSE;
    }

    if (!createAAudioStream()) {
        g_player.wav_file.reset();
        notifyPlaybackError("[STREAM] Failed to create playback stream");
        return JNI_FALSE;
    }

#if LATENCY_TEST_ENABLE
    if (!initGpio()) {
        LOGE("Failed to initialize GPIO for latency test - latency test DISABLED");
        g_player.latency_test_enabled.store(false);
    } else {
        g_player.write_counter.store(0);
        g_player.gpio_state.store(false);
        g_player.mute_audio.store(false);
        g_player.latency_test_enabled.store(true);

        writeGpioValue(0);
        LOGI("Latency test initialized: GPIO=%s, interval=%d", LATENCY_TEST_GPIO_FILE, LATENCY_TEST_INTERVAL);
    }
#endif

    g_player.is_playing.store(true, std::memory_order_release);
    aaudio_result_t result = AAudioStream_requestStart(g_player.stream);

    if (result != AAUDIO_OK) {
        LOGE("Failed to start: %s", AAudio_convertResultToText(result));
        g_player.is_playing.store(false, std::memory_order_release);
        AAudioStream_close(g_player.stream);
        g_player.stream = nullptr;
        g_player.wav_file.reset();
        notifyPlaybackError("[STREAM] Failed to start playback stream");
        return JNI_FALSE;
    }

    notifyPlaybackStarted();
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_stopNativePlayback(JNIEnv* env,
                                                                                                jobject thiz) {
    LOGI("stopNativePlayback");

    g_player.is_playing.store(false, std::memory_order_release);

    if (g_player.stream) {
        aaudio_result_t result = AAudioStream_requestStop(g_player.stream);
        if (result != AAUDIO_OK) {
            LOGW("Failed to request stop: %s", AAudio_convertResultToText(result));
        } else {
            aaudio_stream_state_t state = AAUDIO_STREAM_STATE_STOPPING;
            result = AAudioStream_waitForStateChange(g_player.stream, AAUDIO_STREAM_STATE_STOPPING, &state, 100000000);
            if (result != AAUDIO_OK) {
                LOGW("Failed to wait for stop: %s", AAudio_convertResultToText(result));
            }
        }
        AAudioStream_close(g_player.stream);
        g_player.stream = nullptr;
    }

    g_player.wav_file.reset();

#if LATENCY_TEST_ENABLE
    closeGpio();
#endif

    notifyPlaybackStopped();
    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_example_aaudioplayer_player_AAudioPlayer_releaseNative(JNIEnv* env, jobject thiz) {
    LOGI("Releasing AAudio player");

    if (g_player.is_playing.load(std::memory_order_acquire)) {
        Java_com_example_aaudioplayer_player_AAudioPlayer_stopNativePlayback(env, thiz);
    }

    if (g_player.player_instance) {
        env->DeleteGlobalRef(g_player.player_instance);
        g_player.player_instance = nullptr;
    }

    g_player.jvm = nullptr;
    g_player.on_playback_started_method = nullptr;
    g_player.on_playback_stopped_method = nullptr;
    g_player.on_playback_error_method = nullptr;

    LOGI("AAudioPlayer released");
}

}  // extern "C"

WavFile::WavFile() : is_open_(false), header_{} {}

WavFile::~WavFile() noexcept {
    close();
}

bool WavFile::open(const std::string& filePath) {
    close();

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

    is_open_ = true;
    LOGI("WAV file opened: %s, %s", filePath.c_str(), getFormatInfo().c_str());

    return true;
}

void WavFile::close() {
    if (file_.is_open()) {
        file_.close();
    }
    is_open_ = false;
    header_ = {};
}

size_t WavFile::readAudioData(void* buffer, size_t bufferSize) {
    if (!is_open_ || !buffer || bufferSize == 0) {
        return 0;
    }

    constexpr auto kMaxStreamSize = static_cast<size_t>(std::numeric_limits<std::streamsize>::max());
    size_t actual_read_size = std::min(bufferSize, kMaxStreamSize);

    auto read_size = static_cast<std::streamsize>(actual_read_size);
    file_.read(static_cast<char*>(buffer), read_size);
    auto bytes_read = static_cast<size_t>(file_.gcount());

    if (bytes_read < bufferSize) {
        memset(static_cast<char*>(buffer) + bytes_read, 0, bufferSize - bytes_read);
    }

    return bytes_read;
}

bool WavFile::isOpen() const {
    return is_open_;
}

int32_t WavFile::getAAudioFormat() const {
    switch (header_.bits_per_sample) {
        case 16:
            return AAUDIO_FORMAT_PCM_I16;
        case 24:
            return AAUDIO_FORMAT_PCM_I24_PACKED;
        case 32:
            return AAUDIO_FORMAT_PCM_I32;
        default:
            return AAUDIO_FORMAT_PCM_I16;
    }
}

std::string WavFile::getFormatInfo() const {
    std::ostringstream oss;
    oss << static_cast<int32_t>(header_.sample_rate) << "Hz, " << static_cast<int32_t>(header_.num_channels)
        << " channels, " << static_cast<int32_t>(header_.bits_per_sample) << " bits, PCM";
    return oss.str();
}

bool WavFile::isValidFormat() const {
    return (header_.audio_format == 1 && header_.num_channels > 0 && header_.num_channels <= 16 &&
            header_.sample_rate > 0 && header_.sample_rate <= 192000 &&
            (header_.bits_per_sample == 8 || header_.bits_per_sample == 16 || header_.bits_per_sample == 24 ||
             header_.bits_per_sample == 32) &&
            header_.data_size > 0);
}

bool WavFile::readHeader() {
    file_.seekg(0, std::ios::beg);
    return validateRiffHeader() && readFmtChunk() && findDataChunk();
}

bool WavFile::validateRiffHeader() {
    file_.read(header_.riff_id, 4);
    if (file_.gcount() != 4 || strncmp(header_.riff_id, "RIFF", 4) != 0) {
        LOGE("Invalid RIFF header");
        return false;
    }

    file_.read(reinterpret_cast<char*>(&header_.riff_size), 4);
    if (file_.gcount() != 4) {
        LOGE("Failed to read RIFF size");
        return false;
    }

    file_.read(header_.wave_id, 4);
    if (file_.gcount() != 4 || strncmp(header_.wave_id, "WAVE", 4) != 0) {
        LOGE("Invalid WAVE header");
        return false;
    }

    return true;
}

bool WavFile::readFmtChunk() {
    char chunk_id[4];
    uint32_t chunk_size;

    while (file_.good()) {
        file_.read(chunk_id, 4);
        if (file_.gcount() != 4) {
            LOGE("Failed to read chunk ID");
            return false;
        }

        file_.read(reinterpret_cast<char*>(&chunk_size), 4);
        if (file_.gcount() != 4) {
            LOGE("Failed to read chunk size");
            return false;
        }

        if (strncmp(chunk_id, "fmt ", 4) == 0) {
            strncpy(header_.fmt_id, chunk_id, 4);

            file_.read(reinterpret_cast<char*>(&header_.audio_format), 2);
            file_.read(reinterpret_cast<char*>(&header_.num_channels), 2);
            file_.read(reinterpret_cast<char*>(&header_.sample_rate), 4);
            file_.read(reinterpret_cast<char*>(&header_.byte_rate), 4);
            file_.read(reinterpret_cast<char*>(&header_.block_align), 2);
            file_.read(reinterpret_cast<char*>(&header_.bits_per_sample), 2);

            if (chunk_size > 16) {
                skipChunk(static_cast<uint32_t>(chunk_size - 16));
            }

            return true;
        } else {
            skipChunk(chunk_size);
        }
    }

    LOGE("fmt chunk not found");
    return false;
}

bool WavFile::findDataChunk() {
    char chunk_id[4];
    uint32_t chunk_size;

    while (file_.good()) {
        file_.read(chunk_id, 4);
        if (file_.gcount() != 4) {
            LOGE("Failed to read chunk ID while looking for data");
            return false;
        }

        file_.read(reinterpret_cast<char*>(&chunk_size), 4);
        if (file_.gcount() != 4) {
            LOGE("Failed to read chunk size while looking for data");
            return false;
        }

        if (strncmp(chunk_id, "data", 4) == 0) {
            strncpy(header_.data_id, chunk_id, 4);
            header_.data_size = chunk_size;

            LOGD("Found data chunk: size = %u bytes", chunk_size);
            return true;
        } else {
            skipChunk(chunk_size);
        }
    }

    LOGE("data chunk not found");
    return false;
}

void WavFile::skipChunk(uint32_t chunk_size) {
    constexpr auto kMaxStreamOff = static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max());
    if (static_cast<uint64_t>(chunk_size) > kMaxStreamOff) {
        LOGE("Chunk size too large: %u", chunk_size);
        return;
    }

    file_.seekg(static_cast<std::streamoff>(chunk_size), std::ios::cur);

    if (chunk_size % 2 == 1) {
        file_.seekg(1, std::ios::cur);
    }
}
