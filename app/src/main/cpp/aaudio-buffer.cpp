//
// Created by kaina on 2024/8/2.
//

#include "aaudio-buffer.h"

SharedBuffer::SharedBuffer(size_t size) : m_buffer(size), m_write_ptr(0), m_read_ptr(0), m_count(0) {}
SharedBuffer::~SharedBuffer() = default;

void SharedBuffer::setBufSize(size_t bSize)
{
    m_buffer.resize(bSize, 0);
}

bool SharedBuffer::produce(const char *data, size_t size)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_count + size > m_buffer.size())
    {
        return false; // no enough free buffer
    }
    for (size_t i = 0; i < size; ++i)
    {
        m_buffer[m_write_ptr] = data[i];
        m_write_ptr = (m_write_ptr + 1) % m_buffer.size();
        ++m_count;
    }
    return true;
}

bool SharedBuffer::consume(char *data, size_t size)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_count < size)
    {
        return false; // no enough data
    }
    for (size_t i = 0; i < size; ++i)
    {
        data[i] = m_buffer[m_read_ptr];
        m_read_ptr = (m_read_ptr + 1) % m_buffer.size();
        --m_count;
    }
    return true;
}
