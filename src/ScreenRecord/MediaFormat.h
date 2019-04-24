#pragma once

// ��Ӧffmpeg version 4.0.2

enum VidPixFmt
{
    VID_PIX_FMT_YUV420P = 0,
    VID_PIX_FMT_BGRA = 28
};

enum VidCodecID
{
    VID_CODEC_ID_H264 = 27,
    VID_CODEC_ID_H265 = 173
};

enum VidCapMode
{
    VID_CAP_MODE_NONE = 0,
    VID_CAP_MODE_DIRECTX,
    VID_CAP_MODE_QT,
    VID_CAP_MODE_CNT
};

enum AudCodecID
{
    AUD_CODEC_ID_AAC = 86018,
};

enum AudSampleFmt
{
    AUD_SMP_FMT_S16 = 1,
    AUD_SMP_FMT_FLATP = 8
};

struct VidRecordParam
{
    int width;
    int height;
    int fps;
    int bitsize;
    VidCapMode mode;
    //VidPixFmt format;
};

struct VidSwsParam
{
    int width;
    int height;
    //int fps;
    //int bitsize;
    int bitrate;
    VidCodecID codecId;
};

struct AudRecordParam
{
    int channels;
    int sampleRate;
    int sampleSize;
};

struct AudSwrParam
{
    int channels;
    int sampleRate;
    int bitrate;
    AudCodecID codecId;
};