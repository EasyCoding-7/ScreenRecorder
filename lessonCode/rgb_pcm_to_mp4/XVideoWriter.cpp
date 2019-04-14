#include "XVideoWriter.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
}

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")

#include <iostream>
using namespace std;

class CXVideoWriter : public XVideoWriter
{
public:
    virtual bool open(const char* file)
    {
        // ��װ�ļ������������
        close();
        m_filename = file;

        int ret = avformat_alloc_output_context2(&m_ic, nullptr, nullptr, file);
        if (ret < 0)
        {
            cout << "avformat_alloc_output_context2 failed, reason:" << errStr(ret) << endl;
            return false;
        }
        return true;
    }

    virtual void close()
    {
        if (m_ic)
        {
            avformat_close_input(&m_ic);
            m_ic = nullptr;
        }

        if (m_vCtx)
        {
            avcodec_close(m_vCtx);
            avcodec_free_context(&m_vCtx);
            m_vCtx = nullptr;
        }

        if (m_aCtx)
        {
            avcodec_close(m_aCtx);
            avcodec_free_context(&m_aCtx);
            m_aCtx = nullptr;
        }

        if (m_vSwsCtx)
        {
            sws_freeContext(m_vSwsCtx);
            m_vSwsCtx = nullptr;
        }

        if (m_aSwrCtx)
        {
            swr_free(&m_aSwrCtx);
            m_aSwrCtx = nullptr;
        }

        if (m_yuv)
        {
            av_frame_free(&m_yuv);
            m_yuv = nullptr;
        }

        if (m_pcm)
        {
            av_frame_free(&m_pcm);
            m_pcm = nullptr;
        }
    }

    virtual bool addVideoStream()
    {
        if (!m_ic) { return false; }

        // step 1: ��Ƶ����������
        AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec)
        {
            cout << "avcodec_find_encoder AV_CODEC_ID_H264 failed" << endl;
            return false;
        }

        m_vCtx = avcodec_alloc_context3(codec);
        if (!m_vCtx)
        {
            cout << "avcodec_alloc_context3 failed" << endl;
            return false;
        }

        m_vCtx->bit_rate = m_vBitrate;  // bitrate, ѹ����ÿ���С
        m_vCtx->width = m_outWidth;
        m_vCtx->height = m_outHeight;
        m_vCtx->time_base = { 1, m_outFps };    // ʱ�����
        m_vCtx->framerate = { m_outFps, 1 };    
        m_vCtx->gop_size = 50;          // �������С������֡һ���ؼ�֡
        m_vCtx->max_b_frames = 0;       // B֡Ϊ0
        m_vCtx->pix_fmt = (AVPixelFormat)m_outPixFmt;
        m_vCtx->codec_id = AV_CODEC_ID_H264;
        m_vCtx->thread_count = 4;
        av_opt_set(m_vCtx->priv_data, "preset", "superfast", 0);
        m_vCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        // �򿪱�����
        int ret = avcodec_open2(m_vCtx, codec, nullptr);
        if (ret < 0)
        {
            cout << "avcodec_open2 failed, reason:" << errStr(ret) << endl;
            return false;
        }
        cout << "avcodec_open2 AV_CODEC_ID_H264 success!" << endl;

        // �����Ƶ�������������
        m_vStream = avformat_new_stream(m_ic, codec);
        //m_vStream = avformat_new_stream(m_ic, nullptr);
        if (!m_vStream)
        {
            cout << "avformat_new_stream failed" << endl;
            return false;
        }
        m_vStream->id = AVMEDIA_TYPE_VIDEO;
        m_vStream->codecpar->codec_tag = 0;
        avcodec_parameters_from_context(m_vStream->codecpar, m_vCtx);

        av_dump_format(m_ic, 0, m_filename.c_str(), 1);

        // ���أ��ߴ磩ת�������� rgb->yuv
        m_vSwsCtx = sws_getCachedContext(m_vSwsCtx,
            m_inWidth, m_inHeight, (AVPixelFormat)m_inPixFmt,
            m_outWidth, m_outHeight, (AVPixelFormat)m_outPixFmt,
            SWS_BICUBIC,
            nullptr, nullptr, nullptr);
        if (!m_vSwsCtx)
        {
            cout << "sws_getCachedContext failed" << endl;
            return false;
        }

        m_yuv = av_frame_alloc();
        m_yuv->format = (AVPixelFormat)m_outPixFmt;
        m_yuv->width = m_outWidth;
        m_yuv->height = m_outHeight;
        m_yuv->pts = 0;
        ret = av_frame_get_buffer(m_yuv, 0);
        if (ret < 0)
        {
            cout << "av_frame_get_buffer failed, reason:" << errStr(ret) << endl;
            return false;
        }
        cout << "video AVFrame create success!" << endl;

        return true;
    }

    virtual bool addAudioStream()
    {
        if (!m_ic) return false;
        // 1.�ҵ���Ƶ������
        AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
        if (!codec)
        {
            cout << "avcodec_find_encoder AV_CODEC_ID_AAC failed" << endl;
            return false;
        }
        // 2.����������Ƶ������
        m_aCtx = avcodec_alloc_context3(codec);
        if (!m_aCtx)
        {
            cout << "avcodec_alloc_context3 failed" << endl;
            return false;
        }

        m_aCtx->bit_rate = m_aBitrate;
        m_aCtx->sample_rate = m_outSampleRate;
        m_aCtx->sample_fmt = (AVSampleFormat)m_outSampleFmt;
        m_aCtx->channels = m_outChannel;
        m_aCtx->channel_layout = av_get_default_channel_layout(m_outChannel);
        m_aCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        m_aCtx->time_base = { 1, m_aCtx->sample_rate };    // ʱ�����
        m_aCtx->framerate = { m_aCtx->sample_rate, 1 };

        int ret = avcodec_open2(m_aCtx, codec, nullptr);
        if (ret < 0)
        {
            cout << "avcodec_open2 failed, reason:" << errStr(ret) << endl;
            return false;
        }
        cout << "avcodec_open2 AV_CODEC_ID_AAC success!" << endl;

        // �����Ƶ��
        m_aStream = avformat_new_stream(m_ic, nullptr);
        if (!m_aStream)
        {
            cout << "avformat_new_stream failed!" << endl;
            return false;
        }
        m_aStream->codecpar->codec_tag = 0;
        avcodec_parameters_from_context(m_aStream->codecpar, m_aCtx);

        av_dump_format(m_ic, 0, m_filename.c_str(), 1);

        // ��Ƶ�ز��������Ĵ���
        m_aSwrCtx = swr_alloc_set_opts(m_aSwrCtx,
            m_aCtx->channel_layout, m_aCtx->sample_fmt, m_aCtx->sample_rate,    // �������
            av_get_default_channel_layout(m_inChannel), (AVSampleFormat)m_inSampleFmt, m_inSampleRate,
            0, nullptr);
        ret = swr_init(m_aSwrCtx);
        if (ret < 0)
        {
            cout << "swr_init failed, reason:" << errStr(ret) << endl;
            return false;
        }

        // ��Ƶ�ز��������AVFrame
        m_pcm = av_frame_alloc();
        m_pcm->format = m_aCtx->sample_fmt;
        m_pcm->channels = m_aCtx->channels;
        m_pcm->channel_layout = m_aCtx->channel_layout;
        m_pcm->nb_samples = m_nbSample;   // һ֡��Ƶ������������һ��ͨ������������

        ret = av_frame_get_buffer(m_pcm, 0);
        if (ret < 0)
        {
            cout << "av_frame_get_buffer failed, reason:" << errStr(ret) << endl;
            return false;
        }
        cout << "audio AVFrame create success!" << endl;

        return true;
    }

    virtual AVPacket* encodeVideo(const uint8_t *rgb)
    {  
        const uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
        indata[0] = rgb;
        int insize[AV_NUM_DATA_POINTERS] = { 0 };
        insize[0] = m_inWidth * 4;  // BRGA, a Ϊ͸��ͨ��

        // rgb->yuv
        int h = sws_scale(m_vSwsCtx, indata, insize, 0, m_inHeight, m_yuv->data, m_yuv->linesize);
        if (h <= 0)
        {
            cout << "sws_scale failed" << endl;
            return nullptr;
        }

        // encode
        m_yuv->pts = m_vpts++;
        int ret = avcodec_send_frame(m_vCtx, m_yuv);
        if (ret < 0)
        {
            cout << "avcodec_send_frame failed, reason:" << errStr(ret) << endl;
            return nullptr;
        }

        // ���ձ���������
        AVPacket *packet = av_packet_alloc();
        av_init_packet(packet);
        ret = avcodec_receive_packet(m_vCtx, packet);
        if (ret < 0 || packet->size <= 0)
        {
            av_packet_free(&packet);
            cout << "avcodec_receive_packet failed, reason:" << errStr(ret) << endl;
            return nullptr;
        }
        
        av_packet_rescale_ts(packet, m_vCtx->time_base, m_vStream->time_base);  // ת��timebase
        packet->stream_index = m_vStream->index;

        return packet;
    }

    virtual AVPacket* encodeAudio(const uint8_t *pcm)
    {
        // 1.��Ƶ�ز���
        const uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
        indata[0] = pcm;
        
        int ret = swr_convert(m_aSwrCtx, m_pcm->data, m_pcm->nb_samples, indata, m_pcm->nb_samples);
        if (ret < 0)
        {
            cout << "swr_convert failed, reason:" << errStr(ret) << endl;
            return nullptr;
        }

        // 2.��Ƶ����
        ret = avcodec_send_frame(m_aCtx, m_pcm);
        if (ret < 0)
        {
            cout << "avcodec_send_frame failed, reason:" << errStr(ret) << endl;
            return nullptr;
        }

        AVPacket *packet = av_packet_alloc();
        ret = avcodec_receive_packet(m_aCtx, packet);
        if (ret < 0)
        {
            av_packet_free(&packet);
            cout << "avcodec_receive_packet failed, reason:" << errStr(ret) << endl;
            return nullptr;
        }

        packet->pts = m_apts;
        m_apts += av_rescale_q(m_pcm->nb_samples, { 1, m_aCtx->sample_rate }, m_aCtx->time_base);

        //av_packet_rescale_ts(packet, m_aCtx->time_base, m_aStream->time_base);  // ת��timebase
        packet->stream_index = m_aStream->index;

        return packet;
    }

    virtual bool writeHead()
    {
        if (!m_ic) return false;
        // ��io
        int ret = avio_open(&m_ic->pb, m_filename.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            cout << "avio_open failed, reason:" << errStr(ret) << endl;
            return false;
        }

        // д���װͷ
        ret = avformat_write_header(m_ic, nullptr);
        if (ret < 0)
        {
            cout << "avformat_write_header failed, reason:" << errStr(ret) << endl;
            return false;
        }
        cout << "write " << m_filename << " success!" << endl;

        return true;
    }

    virtual bool writeFrame(AVPacket *packet)
    {
        if (!m_ic || !packet || packet->size <= 0) return false;
        int ret = av_interleaved_write_frame(m_ic, packet);     // �˺������ͷ�AVPacket
        if (ret < 0)
        {
            cout << "av_interleaved_write_frame failed, reason:" << errStr(ret) << endl;
            return false;
        }
        return true;
    }

    virtual bool writeTail()
    {
        if (!m_ic || !m_ic->pb) return false;

        // д��β����Ϣ����
        int ret = av_write_trailer(m_ic);
        if (ret != 0)
        {
            cout << "av_write_trailer failed, reason:" << errStr(ret) << endl;
            return false;
        }
        ret = avio_close(m_ic->pb);
        if (ret < 0)
        {
            cout << "avio_close failed, reason:" << errStr(ret) << endl;
            return false;
        }
        cout << "write tail success!" << endl;
        return true;
    }

    virtual bool isVideoFront()
    {
        if (!m_ic || !m_aCtx || !m_vCtx) return false;
        int ret = av_compare_ts(m_vpts, m_vCtx->time_base, m_apts, m_aCtx->time_base);
        if (ret <= 0)
        {
            return true;
        }
        return false;
    }

private:
    char *errStr(int err)
    {
        static char str[512] = { 0 };
        av_strerror(err, str, 512);
        return str;
    }
private:
    AVFormatContext *m_ic = nullptr;        // ��װmp4���������
    AVCodecContext  *m_vCtx = nullptr;      // ��Ƶ������������
    AVCodecContext  *m_aCtx = nullptr;      // ��Ƶ������������
    AVStream        *m_vStream = nullptr;   // ��Ƶ��
    AVStream        *m_aStream = nullptr;   // ��Ƶ��
    SwsContext      *m_vSwsCtx = nullptr;   // ��Ƶ����ת��������
    SwrContext      *m_aSwrCtx = nullptr;   // ��Ƶ�ز���������
    AVFrame         *m_yuv = nullptr;       // ��Ƶ�ز������yuv frame
    AVFrame         *m_pcm = nullptr;       // ��Ƶ�ز������pcm
    int64_t m_vpts = 0;                     // ��Ƶ��pts
    int64_t m_apts = 0;                     // ��Ƶ��pts
};

XVideoWriter::XVideoWriter()
{
}

XVideoWriter::~XVideoWriter()
{
}

XVideoWriter * XVideoWriter::getInstance(uint16_t index)
{
    static CXVideoWriter wrs[65535];
    return &wrs[index];
}
