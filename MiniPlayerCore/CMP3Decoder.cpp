#include "pch.h"
#include "CMP3Decoder.h"
#include "StringHelper.h"

CMP3Decoder::CMP3Decoder()
{

}
CMP3Decoder::~CMP3Decoder()
{

}

bool CMP3Decoder::Open(LPWSTR file)
{
	int ret = -1;
	_handle = mpg123_new(NULL, &ret);
	if (_handle == NULL || ret != MPG123_OK)
		return false;

	auto str = StringHelper::UnicodeToAnsi(file);
	if (mpg123_open(_handle, str.c_str()) != MPG123_OK) {
		SetLastError(DECODER_ERROR_OPEN_FILE_FAILED, L"Open file error.");
		return false;
	}

	if (mpg123_getformat(_handle, &rate, &channel, &encoding) != MPG123_OK) {
		SetLastError(DECODER_ERROR_BAD_FORMAT, L"Bad file format.");
		return false;
	}
	if ((encoding & MPG123_ENC_16) == MPG123_ENC_16)
		encoding = 16;
	else if ((encoding & MPG123_ENC_32) == MPG123_ENC_32)
		encoding = 32;
	else
		encoding = 8;
	len = mpg123_length(_handle);
	if (len <= 0) {
		SetLastError(DECODER_ERROR_BAD_FORMAT, L"Bad file.");
		return false;
	}
	return true;
}
bool CMP3Decoder::Close()
{
	if (_handle != NULL)
	{
		mpg123_close(_handle);
		mpg123_delete(_handle);
		_handle = NULL;
	}
	return true;
}
int CMP3Decoder::GetChannelsCount()
{
	return channel;
}
int CMP3Decoder::GetBitsPerSample()
{
	return encoding;
}
ULONG CMP3Decoder::GetSampleRate()
{
	return static_cast<ULONG>(rate);
}
int CMP3Decoder::GetBitrate()
{
	return bitrate;
}
double CMP3Decoder::GetLengthSecond()
{
	return len / (double)rate;
}
DWORD CMP3Decoder::GetLengthSample()
{
	return len;
}
DWORD CMP3Decoder::GetCurrentPositionSample()
{
	off_t o = mpg123_tell(_handle);
	return o;
}
DWORD CMP3Decoder::SeekToSample(DWORD sp)
{
	fpos = sp;
	return (mpg123_seek(_handle, static_cast<off_t>(sp), SEEK_SET));
}
double CMP3Decoder::SeekToSecond(double sec)
{
	fpos = static_cast<long>(sec * (double)rate);
	return mpg123_seek(_handle, fpos, SEEK_SET);
}
size_t CMP3Decoder::Read(void * _Buffer, size_t _BufferSize)
{
	size_t t;
	fpos = mpg123_tell(_handle);
	mpg123_read(_handle, (UCHAR*)_Buffer, _BufferSize, &t);
	return t;
}
double CMP3Decoder::GetCurrentPositionSecond()
{
	return fpos / (double)rate;
}
bool CMP3Decoder::IsOpened()
{
	return _handle !=NULL;
}
