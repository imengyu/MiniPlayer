#pragma once
#include "pch.h"

class CAppendBuffer
{
public: 
  CAppendBuffer(void* source, size_t size);
  CAppendBuffer(size_t size);
  ~CAppendBuffer();

  void Empty();
  void Reset();
  void MoveToFirst();
  void Append(void* data, size_t size);
  void AppendNoIncrease(void* data, size_t size);
  void Increase(size_t size);
  void* FirstData();
  void* Data();
  size_t GetFilledSize();
  size_t GetSpaceSize();
  size_t GetSize();

private:
  void* buffer;
  bool holder;
  size_t size;
  size_t pos;
};

