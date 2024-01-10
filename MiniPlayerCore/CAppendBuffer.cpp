#include "pch.h"
#include "CAppendBuffer.h"

CAppendBuffer::CAppendBuffer(void* source, size_t size)
{
  this->size = size;
  buffer = source;
  pos = 0;
  holder = false;
}
CAppendBuffer::CAppendBuffer(size_t size)
{
  this->size = size;
  buffer = new char[size];
  pos = 0;
  holder = true;
}
CAppendBuffer::~CAppendBuffer()
{
  if (holder) {
    delete[] buffer;
    buffer = nullptr;
  }
}

void CAppendBuffer::Empty()
{
  memset(buffer, 0, size);
}
void CAppendBuffer::Reset()
{
  pos = 0;
}
void CAppendBuffer::MoveToFirst()
{
  memmove_s(FirstData(), size, Data(), GetSpaceSize());
  pos = 0;
}
void CAppendBuffer::Append(void* data, size_t size)
{
  memcpy_s((void*)((size_t)buffer + pos), this->size - pos, data, size);
  pos += size;
}
void CAppendBuffer::AppendNoIncrease(void* data, size_t size)
{
  memcpy_s((void*)((size_t)buffer + pos), this->size - pos, data, size);
}
void CAppendBuffer::Increase(size_t size)
{
  pos += size;
}

void* CAppendBuffer::FirstData()
{
  return buffer;
}
void* CAppendBuffer::Data()
{
  return (void*)((size_t)buffer + pos);
}

size_t CAppendBuffer::GetFilledSize()
{
  return pos;
}
size_t CAppendBuffer::GetSpaceSize()
{
  return size - pos;
}
size_t CAppendBuffer::GetSize()
{
  return size;
}
