#include "MediaRecord.h"
#include "VideoCapture.h"
#include "AudioCapture.h"
#include "MediaFileCreate.h"
#include "MediaEncode.h"
#include "glog/logging.h"

extern "C"
{
#include "ffmpeg/libavformat/avformat.h"
}

MediaRecord::MediaRecord()
{
    std::lock_guard<std::mutex> lck(m_mutex);

    m_video = new VideoCapture;
    m_audio = new AudioCapture;
    m_file = new MediaFileCreate;
    m_rtmp = new MediaFileCreate;
}

MediaRecord::~MediaRecord()
{
    std::lock_guard<std::mutex> lck(m_mutex);

    delete m_video;
    delete m_audio;
    delete m_file;
    delete m_rtmp;
}

bool MediaRecord::init(const VidRawParam& vid, const AudRawParam& aud, bool videoIsRec, bool audioIsRec)
{
    uninit();

    std::lock_guard<std::mutex> lck(m_mutex);

    m_vCap = vid;
    m_aCap = aud;
    m_videoIsRec = videoIsRec;
    m_audioIsRec = audioIsRec;

    if (m_videoIsRec && !m_video->init(vid))
    {
        LOG(ERROR) << "video capture thread init failed!";
        return false;
    }

    if (m_audioIsRec && !m_audio->init(aud))
    {
        LOG(ERROR) << "audio capture thread init failed!";
        return false;
    }

    m_init = true;

    return true;
}

bool MediaRecord::uninit()
{
    std::lock_guard<std::mutex> lck(m_mutex);
 
    if (m_videoIsRec && !m_video->uninit())
    {
        LOG(ERROR) << "video capture thread uninit failed!";
    }
    if (m_audioIsRec && !m_audio->uninit())
    {
        LOG(ERROR) << "audio capture thread uninit failed!";
    }

    m_videoIsRec = false;
    m_audioIsRec = false;
    m_init = false;

    return true;
}

bool MediaRecord::startRecord(const VidEncodeParam& vid, const AudEncodeParam& aud)
{
    std::lock_guard<std::mutex> lck(m_mutex);

    if (!m_init)
    {
        LOG(ERROR) << "MediaRecord is not init!";
        return false;
    }

    m_vOut = vid;
    m_aOut = aud;

    if (m_audioIsRec)
    {
        m_audio->startCapture();    // ��ʼ������Ƶ
    }
    if (m_videoIsRec)
    {
        m_video->startCapture();    // ��ʼ������Ƶ
    }

    m_start = true;

    start();                        // ����QT�̣߳�д���ļ�

    return true;
}

bool MediaRecord::stopRecord()
{
    m_mutex.lock();
    
    if (m_videoIsRec)
    {
        m_video->stopCapture();     // ֹͣ������Ƶ
    }
    if (m_audioIsRec)
    {
        m_audio->stopCapture();     // ֹͣ������Ƶ
    }
    m_start = false;

    m_mutex.unlock();

    wait();                         // �ȴ�QT�߳��˳�

    return true;
}

bool MediaRecord::startWriteFile(const char * filename)
{
    std::lock_guard<std::mutex> lck(m_mutex);

    LOG(INFO) << "start write file:" << filename;
    if (!m_init)
    {
        LOG(ERROR) << "MediaRecord is not init!";
        return false;
    }
    
    if (!m_file->open(filename, "mp4"))    // ��д����ļ�
    {
        LOG(ERROR) << "MediaFileCreate create file:" << filename << " failed!";
        return false;
    }

    if (m_videoIsRec && !m_file->setVideoEncode(m_vCap, m_vOut))
    {
        LOG(ERROR) << "m_file->setVideoEncode failed!";
        return false;
    }

    if (m_audioIsRec && !m_file->setAudioEncode(m_aCap, m_aOut))
    {
        LOG(ERROR) << "m_file->setAudioEncode failed!";
        return false;
    }

    if (!m_file->writeHead())       // д��MP4�ļ�ͷ
    {
        LOG(ERROR) << "write file head failed!";
        return false;
    }
    m_filename = filename;
    m_writeFile = true;

    return true;
}

bool MediaRecord::stopWriteFile()
{
    std::lock_guard<std::mutex> lck(m_mutex);

    LOG(INFO) << "stop write file:" << m_filename.c_str();
    m_file->writeTail();        // д���ļ�β
    m_file->close();            // �ر��ļ�
    m_writeFile = false;

    return true;
}

bool MediaRecord::startWriteRtmp(const char * url)
{
    std::lock_guard<std::mutex> lck(m_mutex);

    LOG(INFO) << "start write rtmp:" << url;
    if (!m_init)
    {
        LOG(ERROR) << "MediaRecord is not init!";
        return false;
    }

    if (!m_rtmp->open(url, "flv"))    // ��д����ļ�
    {
        LOG(ERROR) << "MediaFileCreate create file:" << url << " failed!";
        return false;
    }
    
    if (m_videoIsRec && !m_rtmp->setVideoEncode(m_vCap, m_vOut))
    {
        LOG(ERROR) << "m_rtmp->setVideoEncode failed!";
        return false;
    }

    if (m_audioIsRec && !m_rtmp->setAudioEncode(m_aCap, m_aOut))
    {
        LOG(ERROR) << "m_rtmp->setAudioEncode failed!";
        return false;
    }

    if (!m_rtmp->writeHead())       // ����flv�ļ�ͷ
    {
        LOG(ERROR) << "send file head failed!";
        return false;
    }

    m_url = url;
    m_writeRtmp = true;

    return true;
}

bool MediaRecord::stopWriteRtmp()
{
    std::lock_guard<std::mutex> lck(m_mutex);

    LOG(INFO) << "stop write rtmp:" << m_url.c_str();
    m_rtmp->writeTail();        // д���ļ�β
    m_rtmp->close();            // �ر��ļ�
    m_writeRtmp = false;

    return true;
}

bool writeAudioOneFrame(MediaFileCreate *file, AudioCapture *audio)
{
    if (!file || !audio)
    {
        return false;
    }

    FrameData *frame = audio->getData();
    if (!frame)
    {
        return false;
    }
    file->writePacket(file->encodeToPacket(frame));   // д���ļ�
    audio->freeData(frame);
    return true;
}

static bool writeVideoOneFrame(MediaFileCreate *file, VideoCapture *video)
{
    if (!file || !video)
    {
        return false;
    }

    FrameData *frame = video->getData();
    if (!frame)
    {
        return false;
    }
    file->writePacket(file->encodeToPacket(frame));   // д���ļ�
    video->freeData(frame);
    return true;
}

void MediaRecord::run()
{
    FrameData *frame = nullptr;

    while (m_start)
    {
        m_mutex.lock();
        if (!m_start)   // double check
        {
            m_mutex.unlock();
            break;
        }

        if (m_writeFile && !m_writeRtmp)        // ֻд���ļ�
        {
            if (m_audioIsRec && m_file->isAudioPtsEarly())  // ͬ������Ƶʱ���
            {
                writeAudioOneFrame(m_file, m_audio);
            }
            else if (m_videoIsRec)
            {
                if (!writeVideoOneFrame(m_file, m_video))   // ����Ƶд��ʧ�ܡ���������Ϊ����δˢ�´�ʱ����ͬ����Ƶ ��д����Ƶ
                {
                    writeAudioOneFrame(m_file, m_audio);
                }
            }
        }
        else if (!m_writeFile && m_writeRtmp)   // ֻ����
        {
            if (m_audioIsRec && m_rtmp->isAudioPtsEarly())
            {
                writeAudioOneFrame(m_rtmp, m_audio);
            }
            else if (m_videoIsRec)
            {
                if (!writeVideoOneFrame(m_rtmp, m_video))
                {
                    writeAudioOneFrame(m_rtmp, m_audio);
                }
            }
        }
        else if (m_writeFile && m_writeRtmp)    // д���ļ�������
        {
            if (m_audioIsRec && m_file->isAudioPtsEarly())
            {
                frame = m_audio->getData();
                if (frame)
                {
                    m_file->writePacket(m_file->encodeToPacket(frame));   // д���ļ�
                    m_rtmp->writePacket(m_rtmp->encodeToPacket(frame));   // д��rtmp��
                    m_audio->freeData(frame);
                }
            }
            else if (m_videoIsRec)
            {
                frame = m_video->getData();
                if (frame)
                {
                    m_file->writePacket(m_file->encodeToPacket(frame));
                    m_rtmp->writePacket(m_rtmp->encodeToPacket(frame));   // д��rtmp��
                    m_video->freeData(frame);
                }
                else if (m_audioIsRec)          // ����ȡ��Ƶʧ�ܡ���������Ϊ����δˢ�´�ʱ����ͬ����Ƶ ��д����Ƶ
                {
                    frame = m_audio->getData();
                    if (frame)
                    {
                        m_file->writePacket(m_file->encodeToPacket(frame));   // д���ļ�
                        m_rtmp->writePacket(m_rtmp->encodeToPacket(frame));   // д��rtmp��
                        m_audio->freeData(frame);
                    }
                }
            }
        }

        m_mutex.unlock();
        usleep(1);  // ��1us��д�룬����cpuռ���ʹ���
    }
}