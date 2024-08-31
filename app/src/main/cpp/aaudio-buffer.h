//
// Created by kaina on 2024/8/2.
//

#ifndef AAUDIORECORDER_AAUDIO_BUFFER_H
#define AAUDIORECORDER_AAUDIO_BUFFER_H

#include <mutex>
#include <vector>

class SharedBuffer
{
public:
    explicit SharedBuffer(size_t size);
    ~SharedBuffer();

    void setBufSize(size_t size);
    bool produce(const char *data, size_t size);
    bool consume(char *data, size_t size);

private:
    std::vector<char> mBuffer;
    size_t mWritePtr{};
    size_t mReadPtr{};
    size_t mCount{};
    std::mutex mMutex;
};
#endif // AAUDIORECORDER_AAUDIO_BUFFER_H
