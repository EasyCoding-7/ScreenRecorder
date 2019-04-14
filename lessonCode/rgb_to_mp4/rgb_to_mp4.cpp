#include <iostream>

using namespace std;

extern "C"
{
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"     // ��Ƶ��ʽת�����ֱ��ʷŴ���С
#include "libavutil/error.h"
}

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")

char *errStr(int err)
{
    static char str[512] = { 0 };
    av_strerror(err, str, 512);
    return str;
}

int main(void)
{
    //const char *infile = "../../testFile/1280x720_rgb24.rgb";
    const char *infile = "test_out.rgb";
    const char *outfile = "testout.mp4";
    FILE *fp = nullptr;

    fopen_s(&fp, infile, "rb");
    if (!fp)
    {
        cout << "open " << infile << " failed!" << endl;
        getchar();
        return -1;
    }
    
    //int width = 1280;
    //int height = 720;
    //int fps = 25;
    int width = 2560;
    int height = 1440;
    int fps = 10;

    // setp 1:create codec ������
    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec)
    {
        cout << "avcodec_find_encoder AV_CODEC_ID_H264 failed" << endl;
        getchar();
        return -1;
    }
    /// ����������
    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    if (!codecContext)
    {
        cout << "avcodec_alloc_context3 failed" << endl;
        getchar();
        return -1;
    }
    // ѹ��������
    codecContext->bit_rate = 10 * 1000 * 1000; // 524288 bps == 0.5 Mbps ����
    codecContext->width = width;
    codecContext->height = height;
    codecContext->time_base = {1, fps};
    codecContext->framerate = { fps, 1};
    codecContext->gop_size = 50;   // �ؼ�֡����� һ��ͼ��Ĵ�С
    codecContext->max_b_frames = 0; // B֡��ͬ�ȴ����� B֡Խ�࣬ͼ��Խ����������Ӱ�����Ч��
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P; // ���ظ�ʽ
    codecContext->codec_id = AV_CODEC_ID_H264;
    codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; // ȫ�ֵı�����Ϣ
    codecContext->thread_count = 2;

    int ret = avcodec_open2(codecContext, codec, nullptr);
    if (ret < 0)
    {
        cout << "avcodec_open2 failed! reason:" << errStr(ret) << endl;
        getchar();
        return -1;
    }
    cout << "avcodec_open2 success!" << endl;

    // step 2:create out context
    AVFormatContext *oc = nullptr;  // ��װ������
    ret = avformat_alloc_output_context2(&oc, nullptr, nullptr, outfile);
    if (ret < 0)
    {
        cout << "avformat_alloc_output_context2 failed! reason:" << errStr(ret) << endl;
        getchar();
        return -1;
    }

    // step3: add video stream
    AVStream *videoStream = avformat_new_stream(oc, nullptr);
    videoStream->id = AVMEDIA_TYPE_VIDEO;
    videoStream->codecpar->codec_tag = 0;   // Ĭ��ֵ0
    avcodec_parameters_from_context(videoStream->codecpar, codecContext);

    cout << "======================================================" << endl;
    av_dump_format(oc, AVMEDIA_TYPE_VIDEO, outfile, 1);
    cout << "======================================================" << endl;

    // step 4:rgb to yuv
    SwsContext *ctx = nullptr;
    ctx = sws_getCachedContext(ctx,
        width, height, AV_PIX_FMT_BGRA,
        width, height, AV_PIX_FMT_YUV420P,
        SWS_BICUBIC,
        nullptr, nullptr, nullptr);
    if (!ctx)
    {
        cout << "sws_getCachedContext failed!" << endl;
        getchar();
        return -1;
    }

    unsigned char *rgb = new unsigned char[width * height * 4];
    AVFrame *yuv = av_frame_alloc();
    yuv->format = AV_PIX_FMT_YUV420P;
    yuv->width = width;
    yuv->height = height;
    ret = av_frame_get_buffer(yuv, 0);
    if (ret < 0)
    {
        cout << "av_frame_get_buffer failed! reason:" << errStr(ret) << endl;
        getchar();
        return -1;
    }
    int len = 0;
    uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
    int inlinesize[AV_NUM_DATA_POINTERS] = { 0 };
    inlinesize[0] = width * 4;

    // step 5:write mp4 head
    ret = avio_open(&oc->pb, outfile, AVIO_FLAG_WRITE);
    if (ret < 0)
    {
        cout << "avio_open failed! reason:" << errStr(ret) << endl;
        getchar();
        return -1;
    }
    ret = avformat_write_header(oc, nullptr);
    if (ret < 0)
    {
        cout << "avformat_write_header failed! reason:" << errStr(ret) << endl;
        getchar();
        return -1;
    }

    int h = 0;
    uint64_t p = 0;
    AVPacket packet;
    av_init_packet(&packet);
    while (true)
    {
        memset(rgb, 0, sizeof(rgb));
        len = fread(rgb, 1, width * height * 4, fp);
        if (len <= 0) { break; }
        indata[0] = rgb;
        h = sws_scale(ctx, indata, inlinesize, 0, height, yuv->data, yuv->linesize);
        if (h <= 0) { break; }

        // step 6:encode frame
        yuv->pts = p;
        //p += 3600;
        // yuv->pict_type = AV_PICTURE_TYPE_I;
        p += videoStream->time_base.den / fps;
        ret = avcodec_send_frame(codecContext, yuv);
        if (ret != 0)
        {
            cout << "avcodec_send_frame failed! reason:" << errStr(ret) << endl;
            continue;
        }

        ret = avcodec_receive_packet(codecContext, &packet);
        if (ret != 0)
        {
            cout << "avcodec_receive_packet failed! reason:" << errStr(ret) << endl;
            continue;
        }
        av_write_frame(oc, &packet);
        
        cout << '.' << packet.size << endl;

        // ����packet�ڲ��ռ�
        av_interleaved_write_frame(oc, &packet);
    }

    av_write_trailer(oc);   // д����Ƶ����
    
    avio_close(oc->pb);     // �ر���Ƶ���io
    
    avformat_free_context(oc);  // �����װ���������
    
    avcodec_close(codecContext);    // �رձ�����
    
    avcodec_free_context(&codecContext);    // ������Ƶ�ز���������

    sws_freeContext(ctx);   // ������Ƶ�ز���������

    delete rgb;
    fclose(fp);

    cout << endl << "=================end======================" << endl;

    system("pause");
    return 0;
}