#include "pch.h"
#include "CAacDecoder.h"
#include "StringHelper.h"
#include <io.h>

#define FRAME_MAX_LEN 1024*5    
#define BUFFER_MAX_LEN 1024*1024   

static unsigned char frame[FRAME_MAX_LEN];
static unsigned char frame_mono[FRAME_MAX_LEN];

CAacDecoder::CAacDecoder()
{
}
CAacDecoder::~CAacDecoder()
{
}

bool CAacDecoder::Open(const wchar_t* file)
{
	if (_waccess(file, 0) == 0)
	{
		hAac = NeAACDecOpen();
		c_config = NeAACDecGetCurrentConfiguration(hAac);
		c_config->defObjectType = LC;
		c_config->defSampleRate = 44100;
		c_config->outputFormat = FAAD_FMT_16BIT;
		c_config->dontUpSampleImplicitSBR = 1;
		NeAACDecSetConfiguration(hAac, c_config);

		_wfopen_s(&_Stream, file, L"r");
		if (_Stream) {

			long filesize = _filelength(_fileno(_Stream));
			if (filesize > 0) {
				fseek(_Stream, 0, SEEK_SET);

				_PCMBuffer = (LPBYTE)malloc(filesize);
				_AACBuffer = (LPBYTE)malloc(filesize);
				if (_AACBuffer && _PCMBuffer)
				{
					_BufferSize = filesize;

					//Read all buffer
					if (filesize < 1024) fread_s(_AACBuffer, _BufferSize, sizeof(BYTE), filesize, _Stream);
					else {
						long readedsize = 0, outsize = 0;
						while (readedsize < _BufferSize)
						{
							outsize = _BufferSize - readedsize;
							if (outsize >= 1024) {
								fread_s(_AACBuffer, _BufferSize, sizeof(BYTE), 1024, _Stream);
								readedsize += 1024;
							}
							else {
								fread_s(_AACBuffer, _BufferSize, sizeof(BYTE), outsize, _Stream);
								readedsize += outsize;
							}
						}
					}

					size_t size = 0;
					if (get_one_ADTS_frame(&_AACBuffer, (size_t*)&_BufferSize, frame, &size) < 0) {
						freebuffer();
						SetLastError(DECODER_ERROR_BAD_FORMAT, L"AAC Decoder can't get one ADTS frame\n");
						return false;
					}

					// Initialise the library using one of the initialization functions
					long bytesconsumed = NeAACDecInit(hAac, _AACBuffer, _BufferSize, &sample_rate, &channel);
					if (bytesconsumed < 0) {
						freebuffer();
						SetLastError(DECODER_ERROR_BAD_FORMAT, L"Bad file");
						return false;
					}

					int rs = convert(hAac, &_AACBuffer, (size_t*)&_BufferSize, &_PCMBuffer, (size_t*)&_BufferSize, &all_samples);
					if (rs != 0) {
						freebuffer();
						SetLastError(DECODER_ERROR_INTERNAL_ERROR, StringHelper::FormatString(L"AAC Decoder error: %d", rs).c_str());
						return false;
					}

					m_Opened = true;
				}
				else {
					SetLastError(DECODER_ERROR_NO_MEMORY, L"No enough memory.");
				}
			}
			else {
				SetLastError(DECODER_ERROR_BAD_FORMAT, L"Emepty file.");
			}
		}
		else {
			SetLastError(DECODER_ERROR_OPEN_FILE_FAILED, L"Failed open file.");
		}
	}
	else {
		SetLastError(DECODER_ERROR_FILE_NOT_FOUND, L"File not exist.");
	}
	return false;
}
bool CAacDecoder::Close()
{
	if (IsOpened())
	{
		NeAACDecClose(hAac);
		freebuffer();
		fclose(_Stream);
		m_Opened = false;
		return true;
	}
	return false;
}

int get_one_ADTS_frame(unsigned char ** buffer1, size_t *buf_size1, unsigned char * data, size_t * data_size)
{
	BYTE *buffer = *buffer1;
	size_t buf_size = *buf_size1;
	size_t size = 0;
	if (!buffer || !data || !data_size)
		return -1;
	while (1)
	{
		if (buf_size  < 7)
			return -1;
		if ((buffer[0] == 0xff) && ((buffer[1] & 0xf0) == 0xf0))
		{
			size |= (((buffer[3] & 0x03)) << 11);     //high 2 bit
			size |= (buffer[4] << 3);                 //middle 8 bit
			size |= ((buffer[5] & 0xe0) >> 5);        //low 3bit

			printf("len1=%x\n", (buffer[3] & 0x03));
			printf("len2=%x\n", buffer[4]);
			printf("len3=%x\n", (buffer[5] & 0xe0) >> 5);
			printf("size=%d\r\n", (int)size);
			break;
		}
		--*buf_size1;
		++*buffer1;
	}

	if (buf_size < size)
		return -1;

	memcpy(data, buffer, size);
	*data_size = size;

	return 0;
}
int convert(NeAACDecHandle hAac, unsigned char** bufferAAC, size_t *buf_sizeAAC, unsigned char** bufferPCM, size_t  *buf_sizePCM, DWORD*allsamples)
{
	NeAACDecFrameInfo frame_info;
	size_t size = 0;
	unsigned char* pcm_data = NULL;
	while (get_one_ADTS_frame(bufferAAC, buf_sizeAAC, frame, &size) == 0)
	{
		// printf("frame size %d\n", size);
		//decode ADTS frame
		pcm_data = (unsigned char*)NeAACDecDecode(hAac, &frame_info, frame, static_cast<unsigned long>(size));
		if (frame_info.error > 0) {
			OutputDebugStringA(NeAACDecGetErrorMessage(frame_info.error));
			return -1;
		}
		else if (pcm_data && frame_info.samples > 0)
		{
#if _DEBUG
			printf("frame info: bytesconsumed %d, channels %d, header_type %d\
				   object_type %d, samples %d, samplerate %d\n",
				frame_info.bytesconsumed,
				frame_info.channels, frame_info.header_type,
				frame_info.object_type, frame_info.samples,
				frame_info.samplerate);
#endif

			(*allsamples) += frame_info.samples;

			/*buf_sizePCM = frame_info.samples * frame_info.channels;
			
			//从双声道的数据中提取单通道
			for (int i = 0, j = 0; i<4096 && j<2048; i += 4, j += 2)
			{
			bufferPCM[j] = pcm_data[i];
			bufferPCM[j + 1] = pcm_data[i + 1];
			}
			*/
			memcpy_s(*bufferPCM, *buf_sizePCM, pcm_data, size);
			*buf_sizePCM -= size;
			*bufferPCM += size;
		}
		*bufferAAC += size;
		*buf_sizeAAC -= size;
	}
	return 0;
}

int CAacDecoder::GetChannelsCount()
{
	return static_cast<int>(channel);
}
int CAacDecoder::GetBitsPerSample()
{
	switch(c_config->outputFormat)
	{
	case FAAD_FMT_16BIT:return 16;
	case FAAD_FMT_24BIT:return 24;
	case FAAD_FMT_32BIT:return 32;
	}
	return 16;
}
DWORD CAacDecoder::GetSampleRate()
{
	return sample_rate;
}

double CAacDecoder::GetLengthSecond()
{
	return all_samples / (double)sample_rate;
}
double CAacDecoder::SeekToSecond(double sec)
{
	return cur_sample = static_cast<DWORD>(sec * sample_rate);
}
double CAacDecoder::GetCurrentPositionSecond()
{
	return cur_sample / (double)sample_rate;
}

DWORD CAacDecoder::GetLengthSample()
{
	return all_samples;
}
DWORD CAacDecoder::GetCurrentPositionSample()
{
	return cur_sample;
}
DWORD CAacDecoder::SeekToSample(DWORD sp)
{
	return cur_sample = sp;
}

size_t CAacDecoder::Read(void * _Buffer, size_t _BufferSize)
{
	size_t offest = cur_sample * sample_rate*GetBitsPerSample() / 8;
	size_t curmax = offest + _BufferSize;
	if (curmax > (size_t)this->_BufferSize) {
		size_t out = curmax - this->_BufferSize;
		memcpy_s(_Buffer, _BufferSize, _PCMBuffer + offest, out);
		return out;
	}
	else {
		memcpy_s(_Buffer, _BufferSize, _PCMBuffer + offest, _BufferSize);
		return _BufferSize;
	}
	return 0;
}

bool CAacDecoder::IsOpened()
{
	return m_Opened;
}
void CAacDecoder::freebuffer()
{
	if (_AACBuffer) free(_AACBuffer);
	if (_PCMBuffer) free(_PCMBuffer);
}
