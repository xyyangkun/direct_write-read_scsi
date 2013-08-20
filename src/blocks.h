/******************************************************
 * ���ܣ�����Ӳ���ϵĿ�
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
/*���Գ����߼��ǲ�������*/
//#define DEBUG_LOG
/*����*/
#define DEBUG

/*Ӳ�̴ӵ�һ�鿪ʼд*/
#define HD_START 1
/*----------------------------------------------*
 * const defination                             *
 *----------------------------------------------*/
typedef enum
{
	FALSE = 0, TRUE = 1,
} BOOL;
#include "media_api.h"
#define MEDIA_VIDEO		0x01		//��Ƶ����
#define MEDIA_AUDIO	0x02		//��Ƶ����
struct NCHUNK_HDR
{ //avi��ʽ�����ݿ�ͷ��־�ṹ
#define IDX1_VID  		0x63643030	//AVI����Ƶ�����
#define IDX1_AID  		0x62773130	//AVI����Ƶ���ı��
	unsigned long chk_id;
	unsigned long chk_siz;
};
typedef struct
{
	///ѹ�������Ƶ֡
	///ʹ������ṹʱҪ�ȷ���һ���󻺳���,Ȼ�󽫱��ṹ��ָ��ָ�򻺳���

#define MEDIA_VIDEO		0x01		//��Ƶ����
#define MEDIA_AUDIO	0x02		//��Ƶ����
#define FRAMETYPE_I		0x0		// frame flag - I Frame
#define FRAMETYPE_P		0x1		// frame flag - P Frame
#define FRAMETYPE_B		0x2
#define FRAMETYPE_PCM	0x5		// frame flag - Audio Frame
	struct timeval tv; ///<���ݲ���ʱ��ʱ���
	unsigned long channel; ///<ѹ��ͨ��
	unsigned short media; ///<media type ��Ƶ����Ƶ
	unsigned short type; ///<frame type	I/P/����...
	long len; ///<frame_buf�е���Ч�ֽ���
	struct NCHUNK_HDR chunk; ///<���ݿ�ͷ��־��Ŀǰʹ��avi��ʽ
	unsigned char frame_buf[4]; ///<��ű�������Ƶ���ݵ���ʼ��ַ
} enc_frame_t;

/*Ӳ��Ĭ�Ͽ��С512�ֽ�  ���ֵ����˵�ǲ��ܸĵ�*/
#define BLOCKSIZE 512

/*����������ܹ���800��*/
#define MAXDAY 800
/*��ѭ��������ͷ��ƫ��*/
#define YEAR_OFFSET 8
/*��ѭ��������ͷ��ƫ��*/
#define DATE_OFFSET 8

#ifdef DEBUG_LOG
/*60*12/512=2*/
#define YEAR_HEAD_BLOCK_SIZE 20
#define DATE_HEAD_BLOCK_SIZE 2
/*���Գ����߼�ʱ������Ӳ����100000*512��Ҳ��50M������ô��4Mbit���ʿ���д100S��Ҳ����һ���һ����*/
#define MAXBLOCKS 100000
#else
/*
 * ��ʾ�������ռ20�飬��ʾ������ݵ�1350��
 * ���800�� 20->800*12/512
 * 1350->24*3600*8/512
 */
#define YEAR_SEEK 1
#define YEAR_HEAD_BLOCK_SIZE 20
#define DATE_HEAD_BLOCK_SIZE 1350
/*160G��Ӳ������ô�����*/
#define MAXBLOCKS 312581808
#endif

extern media_source_t media;
#define BUFFER_SIZE 400*1024 //���4M������֡Ӧ�ò��ᳬ��100K

/******************************************************************
 * ���
 * *****************************************************************/

/****************************************************************
 * ���
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
 * ʱ��
 **********************************************/
#ifdef DEBUG_LOG
/*������Գ����߼������������̵㣬��һ��ֻ��60��*/
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
	 * ���磺��飬���,����ȡ��д��
	 * ��鲻�����ڴ�,ֱ���ô����
	 * seek:			������Ӳ���ϴ洢��λ��
	 * size:			��������ռ���ֽ���
	 * data_head_size:	�������ݵ�ͷ�Ĵ�С
	 * data_head_buf:	�������ݵ�ͷ����־�������ݵ�����
	 * is_new_mem:		Ҫ���·����ڴ棬���������˵�����Բ�ֱ�ӷ����ڴ档ֱ����buf
	 * */
	blocks(long long seek,int size,int data_head_size,char *data_head_buf=NULL,bool is_new_mem=true,char *data=NULL);

	~blocks()
	{
		printf("11%s\n",__FUNCTION__);
		/*����������ڴ��ͷ�֮*/
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
	/*����������*/
	char *data;
	/*�����ݵ�ͷ*/
	char *data_head_buf;
	/*�������е�ʵ�����ݣ����磺��Ƶ������е���ı�ʾ*/
	char *data_buf;

	int data_size;
	int data_head_size;

	/*data ��ռ�п���*/
	int block_num;

	/*��Ŀ�ͷ��Ӳ���ϵ�λ��*/
	long long seek;

	/*���������ݵĳ��ȵõ���������Ӳ����ռ�Ŀ��� ÿ��512���ֽ�*/
	inline int get_block_num_from_data(int size){return (size + BLOCKSIZE -1)/BLOCKSIZE;};
};
#endif

