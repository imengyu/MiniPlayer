#pragma once

//��Ƶ��ʽ��Ŀǰֻ֧�ּ��֣�
enum TStreamFormat
{
	sfUnknown = 0,
	sfMp3 = 1,//��֧��MP3
	sfOgg = 2,//��֧��OGG
	sfWav = 3,
	sfPCM = 4,
	sfFLAC = 5,//��֧��FLAC
	sfFLACOgg = 6,//��֧��FLAC  Vobis
	sfAC3 = 7,
	sfAacADTS = 8,
	sfWaveIn = 9,
	sfMidi = 10,
	sfWma = 11,
	sfM4a = 12,
	sfAutodetect = 1000//not used
};

//������״̬
enum TPlayerStatus
{
	Unknow,
	//δ���ļ�
	NotOpen,
	//�ļ��Ѵ򿪾���
	Opened,
	//���ڲ���
	Playing,
	//����ͣ
	Paused,
	//���ŵ����ֽ�β��
	PlayEnd,
};

//������ʵ��
class CSoundPlayerBase
{
public:
	//�����ļ�����ǰ������
	//����ֵ�����ش��Ƿ�ɹ�
	virtual bool Load(const wchar_t* path) { return false; }
	//�ر��Ѽ����ļ�
	//����ֵ�����عر��Ƿ�ɹ�
	virtual bool Close() { return false; }

	//��ʼ���߼�������
	//����ֵ�����֮ǰ���ڲ����򷵻�false�����򷵻�true
	virtual bool Play() { return false; }
	//��ͣ����
	//����ֵ�����֮ǰ����ͣ�򷵻�false�����򷵻�true
	virtual bool Pause() { return false; }
	//ֹͣ���ţ���û�йر��ļ���
	virtual bool Stop() { return false; }
	//��ͷ��ʼ����
	virtual bool Restart() { return false; }

	//��ȡ��ǰ���Ž���
	//����ֵ�����Ž��ȣ���λ�룩
	virtual double GetPosition() { return 0; }
	//���õ�ǰ���Ž���
	//������
	//  * second ���Ž��ȣ���λ�룩
	virtual void SetPosition(double second) {}

	//��ȡ��ǰ����ʱ��
	//����ֵ������ʱ������λ�룩
	virtual double GetDuration() { return 0; }
	/*��ȡ������״̬
	* ����ֵ��
	* TPlayerStatus::Unknow,
	* TPlayerStatus::NotOpen, δ���ļ�
	* TPlayerStatus::Opened, �ļ��Ѵ򿪾���
	* TPlayerStatus::Playing, ���ڲ���
	* TPlayerStatus::Paused, ��ͣ��
	* TPlayerStatus::PlayEnd, ���ŵ����ֽ�β��
	*/
	virtual TPlayerStatus GetState() { return TPlayerStatus::Unknow; }
	/*
	* ��ȡ��ǰ�򿪵��ļ���ʽ
	* ����ֵ��TStreamFormat �ļ���ʽ
	*/
	virtual TStreamFormat GetFormat() { return TStreamFormat::sfUnknown; }

	//����������0-100��
	virtual void SetVolume(int volume) {}
	//��ȡ������0-100��
	virtual int GetVolume() { return 0; }

	//���ô���״̬
	virtual void SetLastError(int code, const wchar_t* errmsg) {}
	//���ô���״̬
	virtual void SetLastError(int code, const char* errmsg) {}
	//��ȡ�ϴδ�����Ϣ
	virtual int GetLastError() { return 0; }
	//��ȡ�ϴδ�����Ϣ
	virtual const wchar_t* GetLastErrorMessage() { return L""; }

	//��ȡ��ǰ��Ƶ������
	virtual unsigned long GetSampleRate() { return 0; }
	//��ȡ��ǰ��Ƶ����λ��(����λ��) ��8/16/32��
	virtual int GetBitPerSample() { return 0; }
	//��ȡ��ǰ��Ƶͨ����
	virtual int GetChannelsCount() { return 0; }


	virtual unsigned int GetDurationSample() { return 0; }
	virtual unsigned int GetPositionSample() { return 0; }
	virtual void SetPosition(unsigned int second) {}
};

#define PLAYER_ERROR_NONE 0
#define PLAYER_ERROR_FILE_OPEN_FAILED 1
#define PLAYER_ERROR_DECODER_ERROR 2
#define PLAYER_ERROR_UNKNOWN_FILE_FORMAT 3
#define PLAYER_ERROR_NOT_SUPPORT_FORMAT 4



