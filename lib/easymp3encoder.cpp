#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "easymp3encoder.h"
#include "shine_mp3.h"


static inline const char *get_time(void)
{
	static char temp[32] = {0};
	struct timeval tv;
	gettimeofday(&tv, 0);
	snprintf(temp, sizeof(temp), "%zu.%03zu", tv.tv_sec, tv.tv_usec/1000);
	return temp;
}
#define LOG(fmt, ...) printf("[%s %s:%d] " fmt, get_time(), __FUNCTION__, __LINE__, ##__VA_ARGS__)

/* wav文件读取 */
class CWavFileReader
{
#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_ADPCM 2
#define WAVE_FORMAT_IEEE_FLOAT 3
#define WAVE_FORMAT_ALAW 6
#define WAVE_FORMAT_ULAW 7
#define WAVE_FORMAT_GSM_610 49
#define WAVE_FORMAT_G721_ADPCM 64
#define WAVE_FORMAT_EXTENSIBLE 0xFFFE
public:
	CWavFileReader()
	{
		fp = 0;
		isWavFile = false;
		riffName[0] = wavName[0] = fmtName[0] = 0;
		chunkSize = subChunk1Size = audFormat = 0;
		channelNumber = sampleRate = bytesRate = 0;
		bytesPerSample = bitsPerSample = 0;
		extSize = 0;
		extData = factData = NULL;
		factName[0] = dataName[0] = 0;
		factLength = dataLength = 0;
		fileDataSize = fileHeaderSize = fileTotalSize = 0;
	}

	~CWavFileReader()
	{
		if (extData)
			delete []extData;
		if (factData)
			delete []factData;
		close();
	}

public:
	bool open(const char *filename)
	{
		if (!filename || !strcmp(filename, "-") || strlen(filename)==0)
			return false;
		else
			this->fp = fopen(filename, "rb");
		if (this->fp == NULL) {
			return false;
		}
		
		int ret = 0;
		// 读取"RIFF"
		ret = fread(riffName, 1, sizeof(riffName), fp);
		if (ret != sizeof(riffName)) {
			LOG("read [RIFF] error\n");
			return false;
		}
		
		char riff[4] = {'R','I','F','F'};
		if (memcmp(riffName, riff, 4)) {
			LOG("Not wave file\n");
			return false;
		}
		
		// 读取ChunkSize
		ret = fread(&chunkSize, 1, sizeof(chunkSize), fp);
		if (ret != sizeof(chunkSize)) {
			LOG("read ChunkSize error\n");
			return false;
		}
		LOG("chunkSize: %d\n", chunkSize);
		
		// 读取"WAVE"
		ret = fread(wavName, 1, sizeof(wavName), fp);
		if (ret != sizeof(wavName)) {
			LOG("read [WAVE] error\n");
			return false;
		}

		char wave[4] = {'W','A','V','E'};
		if (memcmp(wavName, wave, 4)) {
			return false;
		}

		// 读取"fmt "
		ret = fread(fmtName, 1, sizeof(fmtName), fp);
		if (ret != sizeof(fmtName)) {
			LOG("read [fmt ] error\n");
			return false;
		}

		char fmt[4] = {'f','m','t',' '};
		if (memcmp(fmtName, fmt, 4)) {
			LOG("Not wave file\n");
			return false;
		}
		
		// 读取Subchunk1Size
		ret = fread(&subChunk1Size, 1, sizeof(subChunk1Size), fp);
		if (ret != sizeof(subChunk1Size)) {
			LOG("read Subchunk1Size error\n");
			return false;
		}
		LOG("subChunk1Size: %d\n", subChunk1Size);
		
		// 读取AudioFormat
		ret = fread(&audFormat, 1, sizeof(audFormat), fp);
		if (ret != sizeof(audFormat)) {
			LOG("read AudioFormat error\n");
			return false;
		}
		LOG("audFormat: %d\n", audFormat);

		// 读取NumChannels
		ret = fread(&channelNumber, 1, sizeof(channelNumber), fp);
		if (ret != sizeof(channelNumber)) {
			LOG("read ChannleNumber error\n");
			return false;
		}
		LOG("channelNumber: %d\n", channelNumber);

		// 读取SampleRate
		ret = fread(&sampleRate, 1, sizeof(sampleRate), fp);
		if (ret != sizeof(sampleRate)) {
			LOG("read SampleRate error\n");
			return false;
		}
		LOG("sampleRate: %d\n", sampleRate);

		// 读取ByteRate
		ret = fread(&bytesRate, 1, sizeof(bytesRate), fp);
		if (ret != sizeof(bytesRate)) {
			LOG("read ByteRate error\n");
			return false;
		}
		LOG("bytesRate: %d\n", bytesRate);

		// 读取BlockAlign
		ret = fread(&bytesPerSample, 1, sizeof(bytesPerSample), fp);
		if (ret != sizeof(bytesPerSample)) {
			LOG("read BlockAlign error\n");
			return false;
		}
		LOG("bytesPerSample: %d\n", bytesPerSample);

		// 读取BitsPerSample
		ret = fread(&bitsPerSample, 1, sizeof(bitsPerSample), fp);
		if (ret != sizeof(bitsPerSample)) {
			LOG("read BitsPerSample error\n");
			return false;
		}
		LOG("bitsPerSample: %d\n", bitsPerSample);

		// 根据Subchunk1Size, 查看是否有扩展块
		if (subChunk1Size >= 18)
		{
			// 读取扩展块大小
			ret = fread(&extSize, 1, sizeof(extSize), fp);
			if (ret != sizeof(extSize)) {
				LOG("read ExtSize error\n");
				return false;
			}
			LOG("extSize: %d\n", extSize);

			int appendMessageLength = subChunk1Size - 18; // = extSize
			extData = new char[appendMessageLength];
			ret = fread(extData, 1, appendMessageLength, fp);
			
			LOG("extData is: \n");
			for (int i=0; i<extSize; i++)
				LOG("%02x\n", extData[i] & 0xff);
		}

		// 查看fact块是否存在
		char chunkName[5];
		ret = fread(chunkName, 1, sizeof(chunkName)-1, fp);
		chunkName[4] = '\0';
		if (strcmp(chunkName, "fact") == 0)
		{
			// 存在fact块则读取数据
			memcpy(factName, chunkName, 4);
			// 读取fact块长度
			ret = fread(&factLength, 1, sizeof(factLength), fp);
			LOG("factLength: %d\n", factLength);
			
			// 读取fact块数据
			factData = new char[factLength];
			ret = fread(factData, 1, factLength, fp);
			// 读取"data"
			ret = fread(dataName, 1, sizeof(dataName), fp);
			
			LOG("factData is: \n");
			for (int i=0; i<factLength; i++)
				LOG("%02x\n", factData[i] & 0xff);
		}
        else if (strcmp(chunkName, "LIST") == 0) // LIST 块
        {
			// 读取LIST块长度
			unsigned int listLength = 0;
			ret = fread(&listLength, 1, sizeof(listLength), fp);
			LOG("listLength: %d\n", listLength);

			// 读取LIST块数据
            unsigned char list[listLength] = {0};
            ret = fread(list, 1, listLength, fp);

			// 读取"data"
			ret = fread(dataName, 1, sizeof(dataName), fp);
			LOG("listData is: \n");
			for (int i=0; i<listLength; i++)
			{
                if (isprint(list[i]))
                {
                    LOG("%c\n", list[i] & 0xff);
                }
                else
                {
				    LOG("%02x\n", list[i] & 0xff);
                }
			}
        }
		else
		{
			memcpy(dataName, chunkName, 4);
			char data[4] = {'d','a','t','a'};
			if (memcmp(dataName, data, 4)) {
				LOG("Not wave file\n");
				return false;
			}
		}
		
		// 读取Subchunk2Size
		ret = fread(&dataLength, 1, sizeof(dataLength), fp);
		if (ret != sizeof(dataLength)) {
			LOG("read Subchunk2Size error\n");
			return false;
		}
		LOG("dataLength: %d\n", dataLength);
		
		fileHeaderSize = ftell(fp);// 文件头大小
		fileTotalSize = chunkSize + 8; // 文件总大小
		fileDataSize = fileTotalSize - fileHeaderSize; // 音频数据大小
		
		LOG("fileHeaderSize: %d\n", fileHeaderSize);
		LOG("fileTotalSize: %d\n", fileTotalSize);
		LOG("fileDataSize: %d\n", fileDataSize);
		
		int totalSize = getFileSize(filename);
		if (totalSize != fileTotalSize) // 说明 chunkSize 是错误的，这是有可能的
		{
			if (totalSize > 0)
			{
				fileTotalSize = totalSize;
				chunkSize = fileTotalSize - 8;
				fileDataSize = fileTotalSize - fileHeaderSize;
			}
		}
		
		LOG("final fileHeaderSize: %d\n", fileHeaderSize);
		LOG("final fileTotalSize: %d\n", fileTotalSize);
		LOG("final fileDataSize: %d\n", fileDataSize);
		
		isWavFile = true;
		return true;
	}
	
	void close()
	{
		if (this->fp != 0)
			fclose(this->fp);
		this->fp = 0;
	}
	
	bool isValid() const
	{
		return this->isWavFile;
	}
	
	int size() const
	{
		return this->fileDataSize;
	}
	
	unsigned int samplingFrequence() const
	{
		return this->sampleRate;
	}

	unsigned short width() const
	{
		return this->bitsPerSample;
	}
	
	unsigned short numChannels() const
	{
		return this->channelNumber;
	}

	unsigned int byteRate() const
	{
		return this->bytesRate;
	}
	
	unsigned short audioFormat() const
	{
		return this->audFormat;
	}
	
	const char *formatStr() const
	{
		switch (this->audFormat)
		{
		case WAVE_FORMAT_PCM:return "PCM";
		case WAVE_FORMAT_ADPCM:return "ADPCM";
		case WAVE_FORMAT_IEEE_FLOAT:return "IEEE_FLOAT";
		case WAVE_FORMAT_ALAW:return "ALAW";
		case WAVE_FORMAT_ULAW:return "ULAW";
		case WAVE_FORMAT_GSM_610:return "GSM_610";
		case WAVE_FORMAT_G721_ADPCM:return "G721_ADPCM";
		default: return "unknown";
		}
	}

	bool getHeader(int &format, int &channels, int &sampleRate, 
		int &bps, unsigned int &dataLength)
	{
		format = this->audFormat;
		channels = this->channelNumber;
		sampleRate = this->sampleRate;
		bps = this->bitsPerSample;
		dataLength = this->fileDataSize;
		return this->audFormat && this->sampleRate;
	}

	int read(unsigned char *data, unsigned int length)
	{
		if (!this->fp || !isWavFile)
			return -1;
		
		if (length > this->fileDataSize)
			length = this->fileDataSize;
		
		int n = fread(data, 1, length, this->fp);
		return n;
	}
	
	bool eof()
	{
		return !fp || feof(this->fp);
	}

private:
	int getFileSize(const char *filename)
	{
		if (!filename || !strcmp(filename, "-") || strlen(filename)==0)
			return -1;
		struct stat buf;
		int ret = stat(filename, &buf);
		return ret==0 ? buf.st_size : -1;
	}

private:
	bool isWavFile;
	FILE *fp;
	char riffName[4]; // "RIFF"
	unsigned int chunkSize; // File length minus 8 bytes

	char wavName[4]; // "WAVE"

	// sub-chunk "fmt"
	char fmtName[4]; // "fmt "
	unsigned int subChunk1Size; // 16 for PCM

	// 格式块中的块数据
	unsigned short audFormat; // 音频格式, 1=PCM
	unsigned short channelNumber; // Mono=1, Stereo=2
	unsigned int sampleRate; // 8000, 44100, etc
	unsigned int bytesRate; // SampleRate * NumChannels * BitsPerSample/8
	unsigned short bytesPerSample; // NumChannels * BitsPerSample/8
	unsigned short bitsPerSample; // 8bits, 16bits, etc
	
	// 附加信息(可选), 根据 subChunk1Size 来判断
	unsigned short extSize; // 扩展域大小
	char *extData; // 扩展域信息

	// fact块, 可选字段, 一般当wav文件由某些软件转化而成，则包含该Chunk
	char factName[4]; // "fact"
	unsigned int factLength;
	char *factData;

	// 数据块中的块头
	char dataName[4]; // "data"
	unsigned int dataLength;

	int fileDataSize;				// 文件音频数据大小
	int fileHeaderSize;				// 文件头大小
	int fileTotalSize;				// 文件总大小
};

/////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EasyMp3Encoder::EasyMp3Encoder()
{
    m_samplesPerPass = 576;
    m_encoder = NULL;
    m_config = NULL;
}

EasyMp3Encoder::~EasyMp3Encoder()
{
    stop();
}

/*
 * start the encoder
 * bitrate: 比特率，单位kbps，取值如32，64，128等
 * samplerate：采样率，如8000，16000，44100等
 * channel：声道数，1：单声道，2：双声道
 * return：创建成功返回0，-1表示采样率与比特率不匹配
 */
int EasyMp3Encoder::start(int bitrate, int samplerate, int channel)
{
    stop();

    shine_config_t *mp3Config = new shine_config_t();
    shine_t shine = NULL;

	shine_set_config_mpeg_defaults(&mp3Config->mpeg);
	mp3Config->mpeg.bitr = bitrate;
	mp3Config->mpeg.emph = MU50_15;
	mp3Config->wave.samplerate = samplerate;
    mp3Config->wave.channels = (enum channels)channel;
	
    if (mp3Config->wave.channels > 1)
        mp3Config->mpeg.mode = STEREO;
    else
        mp3Config->mpeg.mode = MONO;

	shine = shine_initialise(mp3Config);
    if (shine == NULL)
    {
        delete mp3Config;
        LOG("Unsupported samplerate/bitrate configuration\n");
        return -1;
    }

	m_samplesPerPass = shine_samples_per_pass(shine) * channel;
	LOG("samples_per_pass: %d\n", m_samplesPerPass);

    m_config = mp3Config;
    m_encoder = shine;
    return 0;
}

void EasyMp3Encoder::stop(void)
{
    shine_config_t *mp3Config = (shine_config_t *)m_config;
    shine_t shine = (shine_t)m_encoder;

    if (shine)
        shine_close(shine);
    m_encoder = NULL;

    if (mp3Config)
        delete mp3Config;
    m_config = NULL;
}

/*
 * encoder PCM data
 * pData: 16bit有符号PCM数据
 * dlen：pdata数据长度，单位short
 * pOut：编码后的MP3数据
 * return：MP3编码数据长度，单位short，失败返回-1
 */
int EasyMp3Encoder::encode(const short *pData, int dlen, short **pOut)
{
    if (!m_encoder || !pData || !pOut)
        return -1;

	int ret = dlen;
    unsigned char *ptr = NULL;
	ptr = shine_encode_buffer_interleaved((shine_t)m_encoder, (short *)pData, &ret);
    *pOut = (short *)ptr;
	return ptr ? ret>>1 : -1;
}

/*
 * 将WAV文件转换为MP3文件
 * wavFileName: 待转换的wav文件名
 * mp3FileName：转换后的MP3文件名
 * return：成功返回0，失败返回-1
 */
int EasyMp3Encoder::convertWav2Mp3(const char *wavFileName, const char *mp3FileName)
{
	CWavFileReader wav;
	if (wav.open(wavFileName))
	{
		LOG("format: %s\n", wav.formatStr());
		if (wav.audioFormat() == WAVE_FORMAT_PCM)
		{
			FILE *fp = fopen(mp3FileName, "wb");
			LOG("fp: %p, rate: %d\n", fp, wav.samplingFrequence());

			unsigned int samplerate = wav.samplingFrequence();
			start(samplerate <= 16000 ? 32 : 64, samplerate, wav.numChannels());

			bool flag = true;
			unsigned char data[m_samplesPerPass * wav.width()/8];
			int n = wav.read(data, sizeof(data));
			LOG("read data len: %d\n", n);
			while (n > 0)
			{
				unsigned char *ptr = 0, *pdata = data;
				unsigned char *ptrConvert = 0;
				int ret = 0;

				if (wav.width() == 8) // 转换为16bit
				{
					ptrConvert = new unsigned char[m_samplesPerPass * wav.width()/8 * 2];
					convert8BitTo16Bit(pdata, n, (short *)ptrConvert);

					if (flag)
					{
						flag = false;
						unsigned char *output = new unsigned char[n];
						convert16BitTo8Bit((short *)ptrConvert, n, output);
						for (int n=0; n<4; n++)
						{
							LOG("8bit: 0x%02x, 16bit: 0x%04x, out: 0x%02x\n",
								pdata[n], ((short *)ptrConvert)[n], output[n]);
						}
						delete []output;
					}
					
					pdata = ptrConvert;
					n <<= 1;
				}

				ret = encode((short *)pdata, n, (short **)&ptr);
				if (ret > 0 && ptr)
				{
					fwrite(ptr, ret, sizeof(short), fp);
				}

				if (ptrConvert)
					delete []ptrConvert;
				ptrConvert = 0;

				memset(data, 0, sizeof(data));
				n = wav.read(data, sizeof(data));
			}

			stop();
			fclose(fp);
            return 0;
		}
	}
    return -1;
}

void EasyMp3Encoder::convert8BitTo16Bit(const unsigned char *input, int insize, short *output)
{
	// 8bit的PCM数据是无符号的，16bit的PCM是有符号的，需要先把8bit的PCM数据转成有符号数
	// 具体是：将8bit无符号数加上0x80，再强制转换为8bit有符号数
	// 再将8it有符号数左移8位即得到16bit有符号数
	for (int i=0; i<insize; ++i)
	{
		char in = (char)(input[i] + 0x80);
		output[i] = static_cast<short>(in << 8);
	}
}

void EasyMp3Encoder::convert16BitTo8Bit(const short *input, int insize, unsigned char *output)
{
	for (int i=0; i<insize; ++i)
	{
		unsigned short in = (unsigned short)(input[i] - 0x8000);
		output[i] = static_cast<unsigned char>(in >> 8);
	}
}





