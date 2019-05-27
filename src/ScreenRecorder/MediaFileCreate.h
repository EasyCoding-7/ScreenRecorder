#pragma once
#include <string>
#include <mutex>
#include <QTime>
#include "MediaFormat.h"

class AVFormatContext;
class AVOutputFormat;
class AVCodecContext;
class AVStream;
class SwsContext;
class SwrContext;
class AVFrame;

class MediaFileCreate
{
public:
    MediaFileCreate() {};
    virtual ~MediaFileCreate() {};
    
    bool open(const std::string& filename,
        const VidCapParam& vidin, const VidOutParam& vidout,
        const AudCapParam& audin, const AudOutParam& audout);
    void close();
    bool addVideoStream();
    bool addAudioStream();
    void* encodeVideo(const FrameData *frame);
    void* encodeAudio(const FrameData *frame);
    bool writeHead();
    bool writeFrame(void *packet);
    bool writeTail();
    bool isAudioPtsEarly();

    void inWidth(int width) { m_inWidth = width; }
    void inHeight(int height) { m_inHeight = height; }
    void outWidth(int width) { m_outWidth = width; }
    void outHeight(int height) { m_outHeight = height; }
    void outFps(int fps) { m_outFps = fps; }

private:
    std::string errStr(int err);
protected:
    std::mutex  m_lock;
    std::string m_filename;

    AVFormatContext *m_oc = nullptr;        // ��װ���������
    AVOutputFormat  *m_ofmt = nullptr;      // �����ʽ
    AVCodecContext  *m_vCtx = nullptr;      // ��Ƶ������������
    AVCodecContext  *m_aCtx = nullptr;      // ��Ƶ������������
    AVStream        *m_vStream = nullptr;   // ��Ƶ��
    AVStream        *m_aStream = nullptr;   // ��Ƶ��
    SwsContext      *m_vSwsCtx = nullptr;   // ��Ƶ����ת��������
    SwrContext      *m_aSwrCtx = nullptr;   // ��Ƶ�ز���������
    AVFrame         *m_yuv = nullptr;       // ��Ƶ�ز������yuv frame
    AVFrame         *m_pcm = nullptr;       // ��Ƶ�ز������pcm

    int64_t m_timestamp = 0;
    int64_t m_vpts = 0;     // ��Ƶ��pts
    int64_t m_apts = 0;     // ��Ƶ��pts
    int m_vThdNum = 2;      // ��Ƶ�����߳���
    int m_aThdNum = 1;      // ��Ƶ�����߳���

    // ��Ƶ�����������
    int m_inWidth = 1920;
    int m_inHeight = 1080;
    int m_inPixFmt = VID_PIX_FMT_BGRA;    // AV_PIX_FMT_BGRA
    int m_bitsize = 4;

    // ��Ƶ�����������
    int m_vBitrate = 10 * 1024 * 1000;  // 10 Mbps
    int m_outWidth = 1920;
    int m_outHeight = 1080;
    int m_outPixFmt = VID_PIX_FMT_YUV420P;    // AV_PIX_FMT_YUV420P
    int m_outFps = 25;
    VidCodecID m_vcodecId = VID_CODEC_ID_H264;

    // ��Ƶ�������
    int m_inChannels = 2;                           // ����ͨ��
    int m_inSampleRate = 44100;                     // ������
    int m_nbSample = 1024;    // �������ͨ����ÿ֡����ÿͨ���������� 
    AudSampleFmt m_inSampleFmt = AUD_SMP_FMT_S16;   // ������ʽ

    // ��Ƶ�������
    int m_aBitrate = 2 * 64 * 1000; // 128 Kbps
    int m_outChannels = 2;
    int m_outSampleRate = 44100;                      // ������
    AudSampleFmt m_outSampleFmt = AUD_SMP_FMT_FLATP;  // ������ʽ
    AudCodecID m_acodecId = AUD_CODEC_ID_AAC;   // AV_CODEC_ID_AAC
};

