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

    void setBufSize(size_t bSize);
    bool produce(const char *data, size_t size);
    bool consume(char *data, size_t size);

private:
    std::vector<char> m_buffer;
    size_t m_write_ptr{};
    size_t m_read_ptr{};
    size_t m_count{};
    std::mutex m_mutex;
};
#endif // AAUDIORECORDER_AAUDIO_BUFFER_H
