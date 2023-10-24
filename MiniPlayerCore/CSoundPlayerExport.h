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
	//���ڼ�����
	Loading,
};

class CSoundPlayer;

//�������¼��ص�
typedef void (*CSoundPlayerEventCallback)(CSoundPlayer* player, int message, void* customData);

//������ʵ��
class CSoundPlayer
{
public:
	CSoundPlayer() {}
	virtual ~CSoundPlayer() {}

	//�����ļ�����ǰ������
	//����ֵ�����ش��Ƿ�ɹ�
	virtual bool Load(const wchar_t* path) { return false; }
	//Ԥ�����ļ�����ǰ��������
	// * Ԥ������ɺ�ֱ�ӵ���Play���ɲ��š�
	// * ���ߵ�ǰ���ڲ��ŵ��ļ��ﵽĩβ�������Զ����ϲ�����Ԥ�ӵ�����
	// * �������������δ����״̬����ֱ�ӵ��������ļ��ط���������Ԥ����
	// * ���Ԥ����1�����ݣ��ظ����ô˷����ᵼ��֮ǰԤ��������ʧЧ
	//����ֵ�����ش��Ƿ�ɹ�
	virtual bool PreLoad(const wchar_t* path) { return false; }
	//�ر��Ѽ����ļ�
	//����ֵ�����عر��Ƿ�ɹ�
	virtual bool Close() { return false; }

	//��ȡ��ǰ�Ƿ���Ԥ��������
	//����ֵ�����ص�ǰ�Ƿ���Ԥ��������
	virtual bool IsPreLoad() { return false; }

	//�����¼�֪ͨ�ص����ص��ڷ����̴߳���
	virtual void SetEventCallback(CSoundPlayerEventCallback callback, void* customData) {}

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

	//��������
	//������
	//  * volume ������0.0-1.0��
	//  * index ����������ͨ�������������˫ͨ����������0Ϊ��������1Ϊ��������-1Ϊ����������������
	virtual void SetVolume(float volume, int index = 0) {}
	//��ȡ����
	//������
	//  * index ��ȡ������ͨ�������������˫ͨ����������0Ϊ��������1Ϊ������
	//����ֵ��������0.0-1.0��
	virtual float GetVolume(int index = 0) { return 0;  }

	//���ô���״̬
	virtual void SetLastError(int code, const wchar_t* errmsg) {}
	//���ô���״̬
	virtual void SetLastError(int code, const char* errmsg) {}
	//��ȡ�ϴδ�����
	//����ֵ���μ��·� PLAYER_ERROR_*
	virtual int GetLastError() { return 0; }
	//��ȡ�ϴδ�����Ϣ
	virtual const wchar_t* GetLastErrorMessage() { return L""; }
	//��ȡ�ϴδ�����Ϣ
	virtual const char* GetLastErrorMessageUtf8() { return ""; }

	//��ȡ��ǰ��Ƶ������
	virtual unsigned long GetSampleRate() { return 0; }
	//��ȡ��ǰ��Ƶ����λ��(����λ��) ��8/16/32��
	virtual int GetBitPerSample() { return 0; }
	//��ȡ��ǰ��Ƶͨ����
	virtual int GetChannelsCount() { return 0; }


	virtual unsigned int GetDurationSample() { return 0; }
	virtual unsigned int GetPositionSample() { return 0; }	
	virtual void SetPositionSample(unsigned int sample) {}
};

#define PLAYER_ERROR_NONE                0 //�޴���
#define PLAYER_ERROR_FILE_OPEN_FAILED    1 //�ļ��򿪴���
#define PLAYER_ERROR_DECODER_ERROR       2 //����������
#define PLAYER_ERROR_UNKNOWN_FILE_FORMAT 3 //δ֪�ļ���ʽ
#define PLAYER_ERROR_NOT_SUPPORT_FORMAT  4 //��֧�ֵĸ�ʽ
#define PLAYER_ERROR_OUTPUT_ERROR        5 //�����Ƶ�豸����
#define PLAYER_ERROR_NOT_LOAD            6 //δ���سɹ������ԵȻ��߼���״̬
#define PLAYER_ERROR_DEVICE_INVALID      7 //��Ƶ����豸���γ����߲�����
#define PLAYER_ERROR_SERVICE_NOT_RUN     8 //Windows ��Ƶ����δ���С�(AUDCLNT_E_SERVICE_NOT_RUNNING)
#define PLAYER_ERROR_LOADING             9 //�в�������ִ�У������ظ�����

#define SOUND_PLAYER_EVENT_PLAY_END      1 //���ŵ�ĩβ
#define SOUND_PLAYER_EVENT_PLAY_ERROR    2 //������;����


