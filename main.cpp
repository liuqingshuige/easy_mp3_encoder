#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "easymp3encoder.h"

// 返回开机以来的时间(ms)
unsigned long GetTickCount(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec*1000+ts.tv_nsec/1000000);
}

int main(int argc, char **argv)
{
    if (argc != 3) 
    {
        printf("usage:\n\t%s <wav-filename> <mp3-filename>\n", argv[0]);
        return 0;
    }
	
	EasyMp3Encoder encoder;
	printf("==========START CONVERT: %lu========\n", GetTickCount());
	encoder.convertWav2Mp3(argv[1], argv[2]); // 转换为MP3文件
	printf("==========FINISH CONVERT: %lu========\n", GetTickCount());
    return 0;
}
