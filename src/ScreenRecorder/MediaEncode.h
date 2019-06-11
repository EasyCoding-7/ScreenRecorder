#pragma once
#include <mutex>
#include "MediaFormat.h"

class AVFrame;
class AVPacket;
class AVCodecContext;
class AVFormatContext;
class AVStream;
class SwsContext;
class SwrContext;

class MediaEncode
{
public:
    MediaEncode();
    virtual ~MediaEncode();

    bool initVideoCodec(const VidRawParam& raw, const VidEncodeParam& param);
    bool deinitVideoCodec();
    bool initAudioCodec(const AudRawParam& raw, const AudEncodeParam& param);
    bool deinitAudioCodec();
    bool addVideoStream(AVFormatContext *oc);
    bool addAudioStream(AVFormatContext *oc);
    bool start();
    bool stop();
    AVPacket* encodeVideo(const FrameData *frame);
    AVPacket* encodeAudio(const FrameData *frame);
    bool isAudioPtsEarly();

    AVCodecContext *videoCodecContext() const { return m_vCtx; }
    AVCodecContext *audioCodecContext() const { return m_aCtx; }
    AVStream *videoStream() const { return m_vStream; }
    AVStream *audioStream() const { return m_aStream; }
    int64_t videoPts() const { return m_vpts; }
    int64_t audioPts() const { return m_apts; }
    
private:
    std::mutex  m_lock;
    AVCodecContext  *m_vCtx = nullptr;      // ��Ƶ������������
    AVCodecContext  *m_aCtx = nullptr;      // ��Ƶ������������
    AVStream        *m_vStream = nullptr;   // ��Ƶ��
    AVStream        *m_aStream = nullptr;   // ��Ƶ��
    SwsContext      *m_vSwsCtx = nullptr;   // ��Ƶ����ת��������
    SwrContext      *m_aSwrCtx = nullptr;   // ��Ƶ�ز���������
    AVFrame         *m_yuv = nullptr;       // ��Ƶ�ز������yuv frame
    AVFrame         *m_pcm = nullptr;       // ��Ƶ�ز������pcm

    VidRawParam     m_vRaw;     // ��Ƶ�������
    VidEncodeParam  m_vEncode;  // ��Ƶ�������
    AudRawParam     m_aRaw;     // ��Ƶ�������
    AudEncodeParam  m_aEncode;  // ��Ƶ�������

    int64_t m_timestamp = 0;    // ��ʼ����ʱ��ʱ���
    int64_t m_vpts = 0;         // ��Ƶ��pts
    int64_t m_apts = 0;         // ��Ƶ��pts
    int     m_vThdNum = std::thread::hardware_concurrency();      // ��Ƶ�����߳���
    int     m_aThdNum = 2;      // ��Ƶ�����߳���
};