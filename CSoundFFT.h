#pragma once
#include "stdafx.h"

#define PI_2 6.283185F
#define PI   3.1415925F
#define FFT_SIZE 512
#define SPECTROGRAM_COUNT 31
#define SPECTROGRAM_COUNT_SMALL 21

class CSoundFFT
{
public:
	CSoundFFT();
	~CSoundFFT();

	/*
	* ��ȡ�������֮ƽ��ֵ ����db (������ֵ 2^16-1 = 65535 ���ֵ�� 96.32db) 
	*
	* @param pcmdata
	* @param size ��С
	*/
	int db(const unsigned char * pcmdata, size_t startpos, size_t size);
	int getPcmDB(const unsigned char * pcmdata, size_t size);
	/**
	* ��ʾƵ��ʱ����FFT����
	*
	* @param buf
	* @param samplerate ������
	*/	
	void spectrogram(BYTE buf[], double samplerate, int startpos = 0);

	void spectrogram_nostanderd(BYTE buf[], double samplerate, int startpos);

	/**
	* ���ٸ���Ҷ�任�������� x �任���Ա����� x ��(����㷨���Բ�����⣬ֱ����)��ת��Ƶ��������������Էֲ���
	* �����ÿһ������ź�ǿ�ȣ�����Ƶǿ��
	*
	* @param real ʵ��
	* @param imag �鲿
	* @param n    ���ٸ�������FFT,n����Ϊ2��ָ������
	* @return
	*/
	int fft(double real[], double imag[], int n);

	bool locinited = false;

	int loc[SPECTROGRAM_COUNT];
	double sampleratePoint[SPECTROGRAM_COUNT];
	double real[FFT_SIZE];
	double imag[FFT_SIZE];

	double first_fft_real[FFT_SIZE];
	double first_fft_imag[FFT_SIZE];

	double second_fft_real[FFT_SIZE];
	double second_fft_imag[FFT_SIZE];
};

