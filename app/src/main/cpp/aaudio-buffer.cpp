//
// Created by kaina on 2024/8/2.
//

#include <cstring>
#include "aaudio-buffer.h"

SharedBuffer::SharedBuffer(size_t size) : m_buffer(size), m_write_ptr(0), m_read_ptr(0), m_count(0) {}
SharedBuffer::~SharedBuffer() = default;

void SharedBuffer::setBufSize(size_t bSize)
{
    m_buffer.resize(bSize, 0);
}

bool SharedBuffer::produce(const char *data, size_t size)
{
    // std::unique_lock<std::mutex> lock(m_mutex);
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_count + size > m_buffer.size())
    {
        return false; // not enough free buffer
    }

    size_t first_part = std::min(size, m_buffer.size() - m_write_ptr);
    memcpy(m_buffer.data() + m_write_ptr, data, first_part);

    size_t second_part = size - first_part;
    if (second_part > 0)
    {
        memcpy(m_buffer.data(), data + first_part, second_part);
    }

    m_write_ptr = (m_write_ptr + size) % m_buffer.size();
    m_count += size;

    return true;
}

bool SharedBuffer::consume(char *data, size_t size)
{
    // std::unique_lock<std::mutex> lock(m_mutex);
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_count < size)
    {
        return false; // not enough data
    }

    size_t first_part = std::min(size, m_buffer.size() - m_read_ptr);
    memcpy(data, m_buffer.data() + m_read_ptr, first_part);

    size_t second_part = size - first_part;
    if (second_part > 0)
    {
        memcpy(data + first_part, m_buffer.data(), second_part);
    }

    m_read_ptr = (m_read_ptr + size) % m_buffer.size();
    m_count -= size;

    return true;
}

/*
bool SharedBuffer::produce(const char *data, size_t size)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_count + size > m_buffer.size())
    {
        return false; // no enough free buffer
    }
    for (size_t i = 0; i < size; i++)
    {
        m_buffer[m_write_ptr] = data[i];
        m_write_ptr = (m_write_ptr + 1) % m_buffer.size();
    }
    m_count += size;
    return true;
}

bool SharedBuffer::consume(char *data, size_t size)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_count < size)
    {
        return false; // no enough data
    }
    for (size_t i = 0; i < size; i++)
    {
        data[i] = m_buffer[m_read_ptr];
        m_read_ptr = (m_read_ptr + 1) % m_buffer.size();
    }
    m_count -= size;
    return true;
}
*/
