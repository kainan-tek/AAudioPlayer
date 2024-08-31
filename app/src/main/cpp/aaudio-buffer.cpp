//
// Created by kaina on 2024/8/2.
//

#include <cstring>
#include "aaudio-buffer.h"

SharedBuffer::SharedBuffer(size_t size) : mBuffer(size), mWritePtr(0), mReadPtr(0), mCount(0) {}
SharedBuffer::~SharedBuffer() = default;

void SharedBuffer::setBufSize(size_t size)
{
    mBuffer.resize(size, 0);
}

bool SharedBuffer::produce(const char *data, size_t size)
{
    // std::unique_lock<std::mutex> lock(mMutex);
    std::lock_guard<std::mutex> lock(mMutex);
    if (mCount + size > mBuffer.size())
    {
        return false; // not enough free buffer
    }

    size_t firstPart = std::min(size, mBuffer.size() - mWritePtr);
    memcpy(mBuffer.data() + mWritePtr, data, firstPart);

    size_t secondPart = size - firstPart;
    if (secondPart > 0)
    {
        memcpy(mBuffer.data(), data + firstPart, secondPart);
    }

    mWritePtr = (mWritePtr + size) % mBuffer.size();
    mCount += size;

    return true;
}

bool SharedBuffer::consume(char *data, size_t size)
{
    // std::unique_lock<std::mutex> lock(mMutex);
    std::lock_guard<std::mutex> lock(mMutex);
    if (mCount < size)
    {
        return false; // not enough data
    }

    size_t firstPart = std::min(size, mBuffer.size() - mReadPtr);
    memcpy(data, mBuffer.data() + mReadPtr, firstPart);

    size_t secondPart = size - firstPart;
    if (secondPart > 0)
    {
        memcpy(data + firstPart, mBuffer.data(), secondPart);
    }

    mReadPtr = (mReadPtr + size) % mBuffer.size();
    mCount -= size;

    return true;
}

/*
bool SharedBuffer::produce(const char *data, size_t size)
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mCount + size > mBuffer.size())
    {
        return false; // no enough free buffer
    }
    for (size_t i = 0; i < size; i++)
    {
        mBuffer[mWritePtr] = data[i];
        mWritePtr = (mWritePtr + 1) % mBuffer.size();
    }
    mCount += size;
    return true;
}

bool SharedBuffer::consume(char *data, size_t size)
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mCount < size)
    {
        return false; // no enough data
    }
    for (size_t i = 0; i < size; i++)
    {
        data[i] = mBuffer[mReadPtr];
        mReadPtr = (mReadPtr + 1) % mBuffer.size();
    }
    mCount -= size;
    return true;
}
*/
