#ifndef WAVEFILE_H
#define WAVEFILE_H

#include <cstdint>
#include <fstream>
#include <string>

/**
 * WAV文件管理类
 * 支持WAV文件的读取、解析和音频数据提取
 */
class WaveFile {
public:
    // WAV文件头结构
    struct WaveHeader {
        // RIFF头
        char riffId[4];    // "RIFF"
        uint32_t riffSize; // 文件大小 - 8
        char waveId[4];    // "WAVE"

        // fmt子块
        char fmtId[4];          // "fmt "
        uint16_t audioFormat;   // 音频格式 (1 = PCM)
        uint16_t numChannels;   // 声道数
        uint32_t sampleRate;    // 采样率
        uint32_t byteRate;      // 字节率
        uint16_t blockAlign;    // 块对齐
        uint16_t bitsPerSample; // 每样本位数

        // data子块
        char dataId[4];    // "data"
        uint32_t dataSize; // 音频数据大小
    };

    /**
     * 构造函数
     */
    WaveFile();

    /**
     * 析构函数
     */
    ~WaveFile();

    /**
     * 打开WAV文件
     * @param filePath 文件路径
     * @return 成功返回true，失败返回false
     */
    bool open(const std::string& filePath);

    /**
     * 关闭文件
     */
    void close();

    /**
     * 读取音频数据
     * @param buffer 数据缓冲区
     * @param bufferSize 缓冲区大小（字节）
     * @return 实际读取的字节数
     */
    size_t readAudioData(void* buffer, size_t bufferSize);

    bool isOpen() const;

    // 安全的getter方法
    int32_t getSampleRate() const { return static_cast<int32_t>(header_.sampleRate); }
    int32_t getChannelCount() const { return static_cast<int32_t>(header_.numChannels); }

    /**
     * 获取对应的AAudio格式
     * @return AAudio格式枚举值
     */
    int32_t getAAudioFormat() const;

    std::string getFormatInfo() const;

    /**
     * 验证WAV文件格式
     * @return 格式正确返回true
     */
    bool isValidFormat() const;

private:
    std::ifstream file_;
    WaveHeader header_{};
    bool isOpen_;

    /**
     * 读取并验证WAV文件头
     * @return 成功返回true
     */
    bool readHeader();

    /**
     * 验证RIFF头
     * @return 验证通过返回true
     */
    bool validateRiffHeader();

    /**
     * 查找并读取fmt子块
     * @return 成功返回true
     */
    bool readFmtChunk();

    /**
     * 查找并定位data子块
     * @return 成功返回true
     */
    bool findDataChunk();

    /**
     * 跳过未知的子块
     * @param chunkSize 子块大小
     */
    void skipChunk(uint32_t chunkSize);
};

#endif // WAVEFILE_H