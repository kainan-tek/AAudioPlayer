//
// Created by kaina on 2024/7/7.
//

#ifndef AAUDIOPLAYER_WAV_HEADER_H
#define AAUDIOPLAYER_WAV_HEADER_H

#include <string>
#include <cstdint>
#include "common.h"

struct WAVHeader
{
    // WAV header size is 44 bytes
    // format = RIFF + fmt + data
    char riffID[4];         // "RIFF"
    uint32_t riffSize;      // 36 + (numSamples * numChannels * bytesPerSample)
    char waveID[4];         // "WAVE"
    char fmtID[4];          // "fmt "
    uint32_t fmtSize;       // 16 for PCM; 18 for IEEE float
    uint16_t audioFormat;   // 1 for PCM; 3 for IEEE float
    uint16_t numChannels;   // 1 for mono; 2 for stereo
    uint32_t sampleRate;    // sample rate
    uint32_t byteRate;      // sampleRate * numChannels * bytesPerSample
    uint16_t blockAlign;    // numChannels * bytesPerSample
    uint16_t bitsPerSample; // 8 for 8-bit; 16 for 16-bit; 32 for 32-bit
    char dataID[4];         // "data"
    uint32_t dataSize;      // numSamples * numChannels * bytesPerSample

    /**
     * Writes the WAV file header to the specified output stream.
     *
     * @param out The output stream to write the header to.
     *
     * @throws None
     */
    void write(std::ofstream &out)
    {
        out.write(riffID, 4);                 // "RIFF"
        out.write((char *)&riffSize, 4);      // 36 + (numSamples * numChannels * bytesPerSample)
        out.write(waveID, 4);                 // "WAVE"
        out.write(fmtID, 4);                  // "fmt "
        out.write((char *)&fmtSize, 4);       // 16 for PCM; 18 for IEEE float
        out.write((char *)&audioFormat, 2);   // 1 for PCM; 3 for IEEE float
        out.write((char *)&numChannels, 2);   // 1 for mono; 2 for stereo
        out.write((char *)&sampleRate, 4);    // sample rate
        out.write((char *)&byteRate, 4);      // sampleRate * numChannels * bytesPerSample
        out.write((char *)&blockAlign, 2);    // numChannels * bytesPerSample
        out.write((char *)&bitsPerSample, 2); // 8 for 8-bit; 16 for 16-bit; 32 for 32-bit
        out.write(dataID, 4);                 // "data"
        out.write((char *)&dataSize, 4);      // numSamples * numChannels * bytesPerSample
    }

    /**
     * Reads the WAV file header from the specified input stream.
     *
     * @param in The input stream to read the header from.
     *
     * @throws None
     */
    void read(std::ifstream &in)
    {
        in.read(riffID, 4);                 // "RIFF"
        in.read((char *)&riffSize, 4);      // 36 + (numSamples * numChannels * bytesPerSample)
        in.read(waveID, 4);                 // "WAVE"
        in.read(fmtID, 4);                  // "fmt "
        in.read((char *)&fmtSize, 4);       // 16 for PCM; 18 for IEEE float
        in.read((char *)&audioFormat, 2);   // 1 for PCM; 3 for IEEE float
        in.read((char *)&numChannels, 2);   // 1 for mono; 2 for stereo
        in.read((char *)&sampleRate, 4);    // sample rate
        in.read((char *)&byteRate, 4);      // sampleRate * numChannels * bytesPerSample
        in.read((char *)&blockAlign, 2);    // numChannels * bytesPerSample
        in.read((char *)&bitsPerSample, 2); // 8 for 8-bit; 16 for 16-bit; 32 for 32-bit
        in.read(dataID, 4);                 // "data"
        in.read((char *)&dataSize, 4);      // numSamples * numChannels * bytesPerSample
    }

    /** Prints the WAV file header. */
    [[maybe_unused]] void print()
    {
        ALOGI("RiffID: %s\n", riffID);
        ALOGI("RiffSize: %d\n", riffSize);
        ALOGI("WaveID: %s\n", waveID);
        ALOGI("FmtID: %s\n", fmtID);
        ALOGI("FmtSize: %d\n", fmtSize);
        ALOGI("AudioFormat: %d\n", audioFormat);
        ALOGI("NumChannels: %d\n", numChannels);
        ALOGI("SampleRate: %d\n", sampleRate);
        ALOGI("ByteRate: %d\n", byteRate);
        ALOGI("BlockAlign: %d\n", blockAlign);
        ALOGI("BitsPerSample: %d\n", bitsPerSample);
        ALOGI("DataID: %s\n", dataID);
        ALOGI("DataSize: %d\n", dataSize);
    }
};

bool readWAVHeader(const std::string &filename, WAVHeader &header);

[[maybe_unused]] bool writeWAVHeader(std::ofstream &outFile, uint32_t numSamples, uint32_t sampleRate, uint32_t numChannels, uint32_t bitsPerSample);

[[maybe_unused]] void UpdateSizes(std::ofstream &outfile, uint32_t data_chunk_size);

#endif // AAUDIOPLAYER_WAV_HEADER_H
