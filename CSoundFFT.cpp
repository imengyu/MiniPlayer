#include "stdafx.h"
#include "CSoundFFT.h"
#include <math.h>


CSoundFFT::CSoundFFT()
{
	memset(&first_fft_real, 0, sizeof(first_fft_real));
	memset(&first_fft_imag, 0, sizeof(first_fft_imag));
	memset(&second_fft_real, 0, sizeof(second_fft_real));
	memset(&second_fft_imag, 0, sizeof(second_fft_imag));

	memset(&real, 0, sizeof(real));
	memset(&imag, 0, sizeof(imag));

}
CSoundFFT::~CSoundFFT()
{

}

int CSoundFFT::db(const unsigned char *pcmdata, size_t startpos, size_t size) {

	int db = 0;
	short int value = 0;
	double sum = 0;
	for (UINT i = startpos; i < startpos + size; i += 2)
	{
		memcpy(&value, pcmdata + i, 2); //��ȡ2���ֽڵĴ�С��ֵ��  
		sum += abs(value); //����ֵ���  
	}
	sum = sum / (size / 2); //��ƽ��ֵ��2���ֽڱ�ʾһ������������������Ϊ��size/2����  
	if (sum > 0)
	{
		db = (int)(20.0*log10(sum));
	}
	return db;
}
int CSoundFFT::getPcmDB(const unsigned char *pcmdata, size_t size) {

	int db = 0;
	short int value = 0;
	double sum = 0;
	for (UINT i = 0; i < size; i += 2)
	{
		memcpy(&value, pcmdata + i, 2); //��ȡ2���ֽڵĴ�С��ֵ��  
		sum += abs(value); //����ֵ���  
	}
	sum = sum / (size / 2); //��ƽ��ֵ��2���ֽڱ�ʾһ������������������Ϊ��size/2����  
	if (sum > 0)
	{
		db = (int)(20.0*log10(sum));
	}
	return db;
}
void CSoundFFT::spectrogram(BYTE buf[], double samplerate, int startpos) {
	first_fft_real[0] = buf[startpos];
	first_fft_imag[0] = 0;

	second_fft_real[0] = buf[startpos];
	second_fft_imag[0] = 0;

	for (int i = 0; i < FFT_SIZE; i++) {
		first_fft_real[i] = buf[i + startpos];
		first_fft_imag[i] = 0;
		// �˷�Ƶ(�൱�ڽ�����8��������)������1024�������е�fftƵ���ܶȾ�Խ��������ȡ��Ƶ  
		//second_fft_real[i] = (buf[i * 8 + startpos] + buf[i * 8 + 1] + buf[i * 8 + 2 + startpos]
		//	+ buf[i * 8 + 3 + startpos] + buf[i * 8 + 4 + startpos] + buf[i * 8 + 5 + startpos]
		//	+ buf[i * 8 + 6 + startpos] + buf[i * 8 + 7 + startpos]) / 8.0;
		second_fft_real[i] = buf[i + startpos];
		second_fft_imag[i] = 0;
	}
	// ��Ƶ���ִ�ԭʼ����ȡ  
	fft(first_fft_real, first_fft_imag, FFT_SIZE);

	// �˷�Ƶ���1024�����ݵ�FFT,Ƶ�ʼ��Ϊ5.512Hz(samplerate / 8)��ȡ��Ƶ����  
	fft(second_fft_real, second_fft_imag, FFT_SIZE);
	//�����������ÿһ��Ƶ������꣬��Ӧ�������ֵ����Ϊ�Ƕ�ֵ������ֻ��Ҫ��һ��  
	if (!locinited) {
		double F = pow(20000 / 20, 1.0 / SPECTROGRAM_COUNT);//������20Ϊ��Ƶ���20HZ��31Ϊ����  
		for (int i = 0; i < SPECTROGRAM_COUNT; i++) {
			//20000��ʾ�����Ƶ��20KHZ,�����20-20K֮����������ݳɶ�����ϵ,������Ƶ��׼  
			sampleratePoint[i] = 20 * pow(F, i);//�˷���30Ϊ��Ƶ���   
												//�����samplerateΪ������(samplerate / (1024 * 8))��8��Ƶ���FFT�ĵ��ܶ�  
			loc[i] = (int)(sampleratePoint[i] / (samplerate / (1024.0  *8.0)));//�����ÿһ��Ƶ���λ��  
		}
		locinited = true;
	}
	const int LowFreqDividing = 14;
	//��Ƶ����  
	for (int j = 0; j < LowFreqDividing; j++) {
		int k = loc[j];
		// ��Ƶ���֣��˷�Ƶ������,ȡ31�Σ��Ե�14��Ϊ�ֽ�㣬С��Ϊ��Ƶ���֣�����Ϊ��Ƶ����  
		// �����14����Ҫȡ�������ȷ���ģ�ȷ����Ƶ���㹻������ȡ  
		real[j] = second_fft_real[k]; //�����real��imag��Ӧfft�����ʵ�����鲿  
		imag[j] = second_fft_imag[k];
	}
	// ��Ƶ���֣���Ƶ���ֲ���Ҫ��Ƶ  
	for (int m = LowFreqDividing; m < SPECTROGRAM_COUNT; m++) {
		int k = loc[m];
		real[m] = first_fft_real[k / 8];
		imag[m] = first_fft_imag[k / 8];
	}
}
void CSoundFFT::spectrogram_nostanderd(BYTE buf[], double samplerate, int startpos) {
	first_fft_real[0] = buf[startpos];
	first_fft_imag[0] = 0;

	second_fft_real[0] = buf[startpos];
	second_fft_imag[0] = 0;

	for (int i = 0; i < FFT_SIZE; i++) {
		first_fft_real[i] = buf[i + startpos];
		first_fft_imag[i] = 0;
		// �˷�Ƶ(�൱�ڽ�����8��������)������1024�������е�fftƵ���ܶȾ�Խ��������ȡ��Ƶ  
		//second_fft_real[i] = (buf[i * 8 + startpos] + buf[i * 8 + 1] + buf[i * 8 + 2 + startpos]
		//	+ buf[i * 8 + 3 + startpos] + buf[i * 8 + 4 + startpos] + buf[i * 8 + 5 + startpos]
		//	+ buf[i * 8 + 6 + startpos] + buf[i * 8 + 7 + startpos]) / 8.0;
		second_fft_real[i] = buf[i + startpos];
		second_fft_imag[i] = 0;
	}
	// ��Ƶ���ִ�ԭʼ����ȡ  
	fft(first_fft_real, first_fft_imag, FFT_SIZE);

	// �˷�Ƶ���1024�����ݵ�FFT,Ƶ�ʼ��Ϊ5.512Hz(samplerate / 8)��ȡ��Ƶ����  
	fft(second_fft_real, second_fft_imag, FFT_SIZE);
	//�����������ÿһ��Ƶ������꣬��Ӧ�������ֵ����Ϊ�Ƕ�ֵ������ֻ��Ҫ��һ��  
	if (!locinited) {
		double F = pow(20000 / 20, 1.0 / SPECTROGRAM_COUNT);//������20Ϊ��Ƶ���20HZ��31Ϊ����  
		for (int i = 0; i < SPECTROGRAM_COUNT; i++) {
			//20000��ʾ�����Ƶ��20KHZ,�����20-20K֮����������ݳɶ�����ϵ,������Ƶ��׼  
			sampleratePoint[i] = 20 * pow(F, i);//�˷���30Ϊ��Ƶ���   
												//�����samplerateΪ������(samplerate / (1024 * 8))��8��Ƶ���FFT�ĵ��ܶ�  
			loc[i] = (int)(sampleratePoint[i] / (samplerate / (1024.0  *8.0)));//�����ÿһ��Ƶ���λ��  
		}
		locinited = true;
	}
	const int LowFreqDividing = 14;
	//��Ƶ����  
	for (int j = 0; j < LowFreqDividing; j++) {
		int k = loc[j];
		// ��Ƶ���֣��˷�Ƶ������,ȡ31�Σ��Ե�14��Ϊ�ֽ�㣬С��Ϊ��Ƶ���֣�����Ϊ��Ƶ����  
		// �����14����Ҫȡ�������ȷ���ģ�ȷ����Ƶ���㹻������ȡ  
		real[j] = second_fft_real[k]; //�����real��imag��Ӧfft�����ʵ�����鲿  
		imag[j] = second_fft_imag[k];
	}
	// ��Ƶ���֣���Ƶ���ֲ���Ҫ��Ƶ  
	for (int m = LowFreqDividing; m < SPECTROGRAM_COUNT; m++) {
		int k = loc[m];
		real[m] = first_fft_real[k / 8];
		imag[m] = first_fft_imag[k / 8];
	}
}
int CSoundFFT::fft(double real[], double imag[], int n)
{
	int i, j, l, k, ip;
	int M = 0;
	int le, le2;
	double sR, sI, tR, tI, uR, uI;

	M = (int)(log(n) / log(2));

	// bit reversal sorting  
	l = n / 2;
	j = l;
	// ���Ʒ�ת�����򣬴ӵ�Ƶ����Ƶ  
	for (i = 1; i <= n - 2; i++) {
		if (i < j) {
			tR = real[j];
			tI = imag[j];
			real[j] = real[i];
			imag[j] = imag[i];
			real[i] = tR;
			imag[i] = tI;
		}
		k = l;
		while (k <= j) {
			j = j - k;
			k = k / 2;
		}
		j = j + k;
	}
	// For Loops  
	for (l = 1; l <= M; l++) { /* loop for ceil{log2(N)} */
		le = (int)pow(2, l);
		le2 = (int)(le / 2);
		uR = 1;
		uI = 0;
		sR = cos(PI / le2);// cos��sin���Ĵ�����ʱ�䣬�����ö�ֵ  
		sI = -sin(PI / le2);
		for (j = 1; j <= le2; j++) { // loop for each sub DFT  
									 // jm1 = j - 1;  
			for (i = j - 1; i <= n - 1; i += le) {// loop for each butterfly  
				ip = i + le2;
				tR = real[ip] * uR - imag[ip] * uI;
				tI = real[ip] * uI + imag[ip] * uR;
				real[ip] = real[i] - tR;
				imag[ip] = imag[i] - tI;
				real[i] += tR;
				imag[i] += tI;
			} // Next i  
			tR = uR;
			uR = tR * sR - uI * sI;
			uI = tR * sI + uI * sR;
		} // Next j  
	} // Next l  

	return 0;
}
