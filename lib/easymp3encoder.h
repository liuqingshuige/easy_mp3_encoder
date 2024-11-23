/*
 * An easy mp3 encoder implement by shine_mp3
 * Copyright FreeCode. All Rights Reserved.
 * MIT License (https://opensource.org/licenses/MIT)
 * 2024 by liuqingshuige
 */
#ifndef __LIB_EASY_MP3_ENCODER_H__
#define __LIB_EASY_MP3_ENCODER_H__

class EasyMp3Encoder
{
public:
    EasyMp3Encoder();
    virtual ~EasyMp3Encoder();

	/*
	 * start the encoder
	 * bitrate: 比特率，单位kbps，取值如32，64，128等
	 * samplerate：采样率，如8000，16000，44100等
	 * channel：声道数，1：单声道，2：双声道
	 * return：创建成功返回0，-1表示采样率与比特率不匹配
	 */
    int start(int bitrate, int samplerate, int channel);

	/*
	 * destroy the encoder
	 */
    void stop(void);

	/*
	 * encoder PCM data
	 * pData: 16bit有符号PCM数据
	 * dlen：pdata数据长度，单位short
	 * pOut：编码后的MP3数据
	 * return：MP3编码数据长度，单位short，失败返回-1
	 */
    int encode(const short *pData, int dlen, short **pOut);

	/*
	 * 将WAV文件转换为MP3文件
	 * wavFileName: 待转换的wav文件名
	 * mp3FileName：转换后的MP3文件名
	 * return：成功返回0，失败返回-1
	 */
	int convertWav2Mp3(const char *wavFileName, const char *mp3FileName);

private:
    void convert8BitTo16Bit(const unsigned char *input, int insize, short *output);
    void convert16BitTo8Bit(const short *input, int insize, unsigned char *output);

private:
    int m_samplesPerPass; // 576 or 1152, ect
    void *m_encoder;
    void *m_config;
};



















#endif

