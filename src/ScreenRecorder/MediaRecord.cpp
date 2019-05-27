#include "MediaRecord.h"
#include "MediaFileCreate.h"
#include "VideoCapture.h"
#include "AudioCapture.h"
#include "glog/logging.h"

MediaRecord::MediaRecord()
{
    std::lock_guard<std::mutex> lck(m_mutex);

    m_file = new MediaFileCreate();
    m_video = new VideoCapture();
    m_audio = new AudioCapture();
}

MediaRecord::~MediaRecord()
{
    std::lock_guard<std::mutex> lck(m_mutex);

    delete m_file;
    delete m_video;
    delete m_audio;
}

bool MediaRecord::init(const VidCapParam& vid, const AudCapParam& aud, bool videoIsRec, bool audioIsRec)
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

bool MediaRecord::startRecord(const std::string& filename, const VidOutParam& vid, const AudOutParam& aud)
{
    std::lock_guard<std::mutex> lck(m_mutex);

    LOG(INFO) << "start record, file:" << filename.c_str();
    if (!m_init)
    {
        LOG(ERROR) << "MediaRecord is not init!";
        return false;
    }

    m_filename = filename;
    m_vOut = vid;
    m_aOut = aud;

    if (!m_file->open(filename, m_vCap, m_vOut, m_aCap, m_aOut))
    {
        LOG(ERROR) << "MediaFileCreate create file:" << filename.c_str() << "failed!";
        return false;
    }

    if (m_videoIsRec && !m_file->addVideoStream())
    {
        LOG(ERROR) << "add video stream failed!";
        return false;
    }

    if (m_audioIsRec && !m_file->addAudioStream())
    {
        LOG(ERROR) << "add audio stream failed!";
        return false;
    }

    if (m_audioIsRec)
    {
        m_audio->startCapture();    // ��ʼ������Ƶ
    }
    if (m_videoIsRec)
    {
        m_video->startCapture();    // ��ʼ������Ƶ
    }
    
    // �Կ�ʼд��ͷ�ļ�Ϊʱ�����׼ ֮ǰ��֡����
    if (!m_file->writeHead())    // д��MP4�ļ�ͷ
    {
        LOG(ERROR) << "write file head failed!";
        return false;
    }
    m_start = true;

    start();                        // ����QT�̣߳�д���ļ�

    return true;
}

bool MediaRecord::stopRecord()
{
    m_mutex.lock();
    LOG(INFO) << "stop record, file:" << m_filename.c_str();
    m_file->writeTail();        // д���ļ�β
    m_file->close();            // �ر��ļ�
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

void MediaRecord::run()
{
    FrameData *frame = nullptr;
    void *packet = nullptr;

    while (m_start)
    {
        m_mutex.lock();
        if (!m_start)   // double check
        {
            m_mutex.unlock();
            break;
        }

        if (m_file->isAudioPtsEarly())
        {
            frame = m_audioIsRec ? m_audio->getData() : nullptr;
            if (frame)
            {
                packet = m_file->encodeAudio(frame);
                if (packet)
                {
                    m_file->writeFrame(packet); // д���ļ�
                    LOG(DEBUG) << "write audio one frame success!";
                }
                else
                {
                    LOG(ERROR) << "encode audio failed, filename:" << m_filename.c_str();
                }
            }
            m_audioIsRec ? m_audio->freeData(frame) : nullptr;
        }
        else
        {        
            frame = m_videoIsRec ? m_video->getData() : nullptr;
            if (frame)
            {
                packet = m_file->encodeVideo(frame);  // ����ΪYUV420P
                if (packet)
                {
                    m_file->writeFrame(packet); // д���ļ�
                    LOG(DEBUG) << "write video one frame success!";
                }
                else
                {
                    LOG(ERROR) << "encode video failed, filename:" << m_filename.c_str();
                }
            }
            m_videoIsRec ? m_video->freeData(frame) : nullptr;
        }

        m_mutex.unlock();
        msleep(1);  // ��1ms��д�룬����cpuռ���ʹ���
    }
}