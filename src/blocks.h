/******************************************************
 * 功能：管理硬盘上的块
 *
 *****************************************************/
#ifndef __BLOCKS_H__
#define __BLOCKS_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <iostream>
/*测试程序逻辑是不是正常*/
//#define DEBUG_LOG
/*测试*/
#define DEBUG

/*硬盘从第一块开始写*/
#define HD_START 1
/*----------------------------------------------*
 * const defination                             *
 *----------------------------------------------*/
typedef enum
{
	FALSE = 0, TRUE = 1,
} BOOL;
#include "media_api.h"
#define MEDIA_VIDEO		0x01		//视频数据
#define MEDIA_AUDIO	0x02		//音频数据
struct NCHUNK_HDR
{ //avi格式的数据块头标志结构
#define IDX1_VID  		0x63643030	//AVI的视频包标记
#define IDX1_AID  		0x62773130	//AVI的音频报的标记
	unsigned long chk_id;
	unsigned long chk_siz;
};
typedef struct
{
	///压缩后的视频帧
	///使用这个结构时要先分配一个大缓冲区,然后将本结构的指针指向缓冲区

#define MEDIA_VIDEO		0x01		//视频数据
#define MEDIA_AUDIO	0x02		//音频数据
#define FRAMETYPE_I		0x0		// frame flag - I Frame
#define FRAMETYPE_P		0x1		// frame flag - P Frame
#define FRAMETYPE_B		0x2
#define FRAMETYPE_PCM	0x5		// frame flag - Audio Frame
	struct timeval tv; ///<数据产生时的时间戳
	unsigned long channel; ///<压缩通道
	unsigned short media; ///<media type 音频或视频
	unsigned short type; ///<frame type	I/P/声音...
	long len; ///<frame_buf中的有效字节数
	struct NCHUNK_HDR chunk; ///<数据块头标志，目前使用avi格式
	unsigned char frame_buf[4]; ///<存放编码后的视频数据的起始地址
} enc_frame_t;

/*硬盘默认块大小512字节  这个值可以说是不能改的*/
#define BLOCKSIZE 512

/*本程序最多能管理800天*/
#define MAXDAY 800
/*年循环队列中头的偏移*/
#define YEAR_OFFSET 8
/*天循环队列中头的偏移*/
#define DATE_OFFSET 8

#ifdef DEBUG_LOG
/*60*12/512=2*/
#define YEAR_HEAD_BLOCK_SIZE 20
#define DATE_HEAD_BLOCK_SIZE 2
/*测试程序逻辑时，就让硬盘有100000*512，也就50M左右那么大，4Mbit码率可以写100S，也就是一天多一点了*/
#define MAXBLOCKS 100000
#else
/*
 * 表示年的数据占20块，表示天的数据点1350块
 * 最大800天 20->800*12/512
 * 1350->24*3600*8/512
 */
#define YEAR_SEEK 1
#define YEAR_HEAD_BLOCK_SIZE 20
#define DATE_HEAD_BLOCK_SIZE 1350
/*160G的硬盘有这么多个块*/
#define MAXBLOCKS 312581808
#endif

extern media_source_t media;
#define BUFFER_SIZE 400*1024 //最大4M，但个帧应该不会超过100K

/******************************************************************
 * 年块
 * *****************************************************************/

/****************************************************************
 * 天块
 *
 ******************************************************************/
struct day_block_in_year
{
	int time;
	long long seek;
}__attribute__ ((packed));
struct day_block
{
	long long seek;
};
long long get_current_block();

/***********************************************
 * 时间
 **********************************************/
#ifdef DEBUG_LOG
/*如果测试程序逻辑，可以让天变短点，就一天只有60秒*/
#define SECOFDAY (1*60)
#else
#define SECOFDAY (24*3600)
#endif   /*DEBUG_LOG*/

/***************debug***************************************************/
#define DP(fmt...)	\
	do{\
		printf(fmt);\
		printf("function:[%s],line:%d: \n",__FUNCTION__,__LINE__);\
	}while(0)

/***************debug***************************************************/

extern inline void set_seek(long long seek);
extern int get_hd_fd();
extern int init_sda();
extern int hd_write(int fd, long long  seek, unsigned int blk_num,unsigned char *write_buf,unsigned int buf_size);
extern int hd_read(int fd, long long  seek, unsigned int blk_num,unsigned char *read_buf,unsigned int buf_size);

class blocks
{
	//blocks = head + xx +databuf
public:
	blocks(int a,int b)
	{
		printf("a+b=%d\n",a+b);
		is_new_mem=false;
	};
	blocks()
	{
		printf("%s\n",__FUNCTION__);
		if(init_sda()<0)
		{
			printf("disk init error\n");
			exit(1);
		}
		printf("%s1\n",__FUNCTION__);
		is_new_mem=false;
	};
	/*
	 * 比如：年块，天块,秒块读取，写入
	 * 秒块不分配内存,直接用传入的
	 * seek:			本块在硬盘上存储的位置
	 * size:			本块数据占的字节数
	 * data_head_size:	本块数据的头的大小
	 * data_head_buf:	本块数据的头，标志本块数据的属性
	 * is_new_mem:		要不新分配内存，对了秒块来说，可以不直接分配内存。直接用buf
	 * */
	blocks(long long seek,int size,int data_head_size,char *data_head_buf=NULL,bool is_new_mem=true,char *data=NULL);

	~blocks()
	{
		printf("11%s\n",__FUNCTION__);
		/*如果分配了内存释放之*/
		if(is_new_mem)
		{
			printf("is_new_mem\n");
			free_data();
		}
		else
			printf("no is_new_mem\n");
	};

	int read_hd_data()
	{
		int ret=hd_read(get_hd_fd(),  seek, YEAR_HEAD_BLOCK_SIZE, (unsigned char *)data,data_size);
		if(ret==0)
		{
			printf("read block 1 ok\n");
			return 0;
		}
		return -1;
	}

	void free_data(){delete data;};

	int get_data_buf_size(){return data_size-data_head_size;};
	char *get_data_buf(){return data_buf;};
	char *get_data_head();
	int get_data_head_dize();
	void save()
	{
		int ret=hd_write(get_hd_fd(), seek, YEAR_HEAD_BLOCK_SIZE, (unsigned char *)data,data_size);
		if(ret!=0)
		{
			DP("write YEAR BLOCK error ,plear resove the probram,and i will exit()\n");
			exit(1);
		}
	};

private:
	/**/
	bool is_new_mem;
	/*整个块数据*/
	char *data;
	/*块数据的头*/
	char *data_head_buf;
	/*块数据中的实际数据，比如：视频，年块中的天的表示*/
	char *data_buf;

	int data_size;
	int data_head_size;

	/*data 所占有块数*/
	int block_num;

	/*块的开头在硬盘上的位置*/
	long long seek;

	/*由整个数据的长度得到此数据在硬盘上占的块数 每块512个字节*/
	inline int get_block_num_from_data(int size){return (size + BLOCKSIZE -1)/BLOCKSIZE;};
};
#endif

