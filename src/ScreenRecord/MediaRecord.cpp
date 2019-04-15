#include "MediaRecord.h"
#include "MediaFileCreate.h"
#include "VideoCapture.h"
#include "AudioCapture.h"
#include "glog/logging.h"

#include <iostream>

MediaRecord::MediaRecord()
{
    m_file = new MediaFileCreate();
    m_video = new VideoCapture();
    m_audio = new AudioCapture();
}

MediaRecord::~MediaRecord()
{
    delete m_file;
    delete m_video;
    delete m_audio;
}

bool MediaRecord::init()
{
    if (m_init)
    {
        LOG(WARNING) << "MediaRecord is already init!";
        return true;
    }

    m_mutex.lock();
    if (!m_video->init())
    {
        m_mutex.unlock();
        LOG(ERROR) << "video capture thread init failed!";
        return false;
    }
    if (!m_audio->init())
    {
        m_mutex.unlock();
        LOG(ERROR) << "audio capture thread init failed!";
        return false;
    }
    m_init = true;
    m_mutex.unlock();

    return true;
}

bool MediaRecord::uninit()
{
    m_mutex.lock();
    if (!m_video->uninit())
    {
        m_mutex.unlock();
        LOG(ERROR) << "video capture thread uninit failed!";
        return false;
    }
    if (!m_audio->uninit())
    {
        m_mutex.unlock();
        LOG(ERROR) << "audio capture thread uninit failed!";
        return false;
    }
    m_init = false;
    m_mutex.unlock();

    return true;
}

bool MediaRecord::startRecord(const char *filename, const VidRecParam& vidRecParam, const AudRecParam& audRecParam)
{
    if (!m_init)
    {
        LOG(ERROR) << "MediaRecord is not init!";
        return false;
    }
    m_mutex.lock();
    m_filename = filename;
    m_vidParam = vidRecParam;
    
    VidSwsParam vSws;
    vSws.inwidth = m_video->width();
    vSws.inheight = m_video->height();
    vSws.inbitsize = m_video->bitsize();
    vSws.recParm = vidRecParam;

    AudSwrParam aSwr;
    aSwr.channels = m_audio->channels();
    aSwr.sampleRate = m_audio->sampleRate();
    aSwr.recParm = audRecParam;

    m_video->fps(vidRecParam.fps);
    m_video->mode(VID_CAP_MODE_DIRECTX);

    if (!m_file->open(filename, vSws, aSwr))
    {
        m_mutex.unlock();
        LOG(ERROR) << "MediaFileCreate create file:" << filename << "failed!";
        return false;
    }

    m_file->addVideoStream();
    m_file->addAudioStream();
    m_file->writeHead();        // д��MP4�ļ�ͷ
    
    m_video->startCapture();    // ��ʼ������Ƶ
    m_audio->startCapture();    // ��ʼ������Ƶ

    start();                    // ����QT�̣߳�д���ļ�
    m_start = true;
    m_mutex.unlock();

    return true;
}

bool MediaRecord::stopRecord()
{
    m_mutex.lock();
    m_file->writeTail();        // д���ļ�β
    m_file->close();            // �ر��ļ�

    m_video->stopCapture();     // ֹͣ������Ƶ
    m_audio->stopCapture();     // ֹͣ������Ƶ

    terminate();                // ����QT�߳�
    wait();
    m_start = false;
    m_mutex.unlock();

    return true;
}

void MediaRecord::run()
{
    uint8_t *rgb = nullptr;
    uint8_t *pcm = nullptr;
    void *packet = nullptr;

    while (m_start)
    {
        m_mutex.lock();

        rgb = (uint8_t *)m_video->getData();    // �ӻ�����л�ȡһ֡RGB����
        if (rgb)
        { 
            packet = m_file->encodeVideo(rgb);  // ����ΪYUV420P
            m_file->writeFrame(packet);         // д���ļ�
            delete rgb;
        }

        pcm = (uint8_t *)m_audio->getData();
        if (pcm)
        {
            packet = m_file->encodeAudio(pcm);
            m_file->writeFrame(packet);
            delete pcm;
        }

        m_mutex.unlock();

        msleep(5);  // ��5ms��д�룬����cpuռ���ʹ���
    }
}
