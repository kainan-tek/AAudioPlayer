//
// Created by kaina on 2024/7/7.
//

#include "wav-header.h"
#include <cstdint>
#include <fstream>

/**
 * Writes the WAV file header to the specified output stream.
 *
 * @param outFile The output stream to write the header to.
 * @param numSamples The number of samples in the audio data.
 * @param sampleRate The sample rate of the audio data.
 * @param numChannels The number of channels in the audio data.
 * @param bitsPerSample The number of bits per sample in the audio data.
 *
 * @return True if the header was written successfully, false otherwise.
 */
[[maybe_unused]] bool writeWAVHeader(std::ofstream &outFile, uint32_t numSamples, uint32_t sampleRate,
                                     uint32_t numChannels, uint32_t bitsPerSample) {
    if (!outFile.is_open()) {
        return false;
    }

    WAVHeader header{};
    header.riffID[0] = 'R';
    header.riffID[1] = 'I';
    header.riffID[2] = 'F';
    header.riffID[3] = 'F';

    header.riffSize = 0;
    header.waveID[0] = 'W';
    header.waveID[1] = 'A';
    header.waveID[2] = 'V';
    header.waveID[3] = 'E';

    header.fmtID[0] = 'f';
    header.fmtID[1] = 'm';
    header.fmtID[2] = 't';
    header.fmtID[3] = ' ';

    header.fmtSize = 16;
    header.audioFormat = 1;
    header.numChannels = numChannels;
    header.sampleRate = sampleRate;

    header.byteRate = sampleRate * numChannels * bitsPerSample / 8;
    header.blockAlign = numChannels * bitsPerSample / 8;
    header.bitsPerSample = bitsPerSample;

    header.dataID[0] = 'd';
    header.dataID[1] = 'a';
    header.dataID[2] = 't';
    header.dataID[3] = 'a';
    header.dataSize = numSamples * numChannels * bitsPerSample / 8;

    header.write(outFile);

    return true;
}

/**
 * Reads the WAV file header from the specified input stream.
 *
 * @param inFile The input stream to read the header from.
 * @param header The WAVHeader object to store the header in.
 *
 * @return True if the header was read successfully, false otherwise.
 */
bool readWAVHeader(const std::string &filename, WAVHeader &header) {
    std::ifstream in(filename, std::ios::binary | std::ios::in);
    if (!in.is_open()) {
        return false;
    }
    header.read(in);
    if (in.is_open()) {
        in.close();
    }
    return true;
}

/**
 * Updates the sizes of the RIFF and data chunks in the WAV file.
 *
 * @param outfile The output stream to write the updated header to.
 * @param data_chunk_size The size of the data chunk in bytes.
 */
[[maybe_unused]] void UpdateSizes(std::ofstream &outfile, uint32_t data_chunk_size) {
    // record current position
    std::streampos current_position = outfile.tellp();

    // calculates RIFF chunk size and data chunk size
    uint32_t riff_size = 4 + (8 + 16) + (8 + data_chunk_size);
    uint32_t data_size = data_chunk_size;

    // move to riff size field and update
    outfile.seekp(4, std::ios::beg);
    outfile.write(reinterpret_cast<const char *>(&riff_size), sizeof(riff_size));

    // move to data size field and update
    outfile.seekp(40, std::ios::beg);
    outfile.write(reinterpret_cast<const char *>(&data_size), sizeof(data_size));

    // return to record position
    outfile.seekp(current_position);
}
