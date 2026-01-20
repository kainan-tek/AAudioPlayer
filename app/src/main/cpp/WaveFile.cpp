#include "WaveFile.h"
#include <algorithm>
#include <android/log.h>
#include <cstring>
#include <limits>
#include <sstream>

#define LOG_TAG "WaveFile"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

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