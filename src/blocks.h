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


/*���Գ����߼��ǲ�������*/
//#define DEBUG_LOG
/*����*/
#define DEBUG

/*Ӳ�̴ӵ�һ�鿪ʼд*/
#define HD_START 1
/*----------------------------------------------*
 * const defination                             *
 *----------------------------------------------*/
typedef enum {
    FALSE    = 0,
    TRUE     = 1,
} BOOL;
#include "media_api.h"
#define MEDIA_VIDEO		0x01		//��Ƶ����
#define MEDIA_AUDIO	0x02		//��Ƶ����

struct NCHUNK_HDR {	//avi��ʽ�����ݿ�ͷ��־�ṹ
#define IDX1_VID  		0x63643030	//AVI����Ƶ�����
#define IDX1_AID  		0x62773130	//AVI����Ƶ���ı��
	unsigned long  chk_id;
	unsigned long  chk_siz;
};
typedef struct{
    ///ѹ�������Ƶ֡
    ///ʹ������ṹʱҪ�ȷ���һ���󻺳���,Ȼ�󽫱��ṹ��ָ��ָ�򻺳���

#define MEDIA_VIDEO		0x01		//��Ƶ����
#define MEDIA_AUDIO	0x02		//��Ƶ����

#define FRAMETYPE_I		0x0		// frame flag - I Frame
#define FRAMETYPE_P		0x1		// frame flag - P Frame
#define FRAMETYPE_B		0x2
#define FRAMETYPE_PCM	0x5		// frame flag - Audio Frame

	struct timeval           tv;			   ///<���ݲ���ʱ��ʱ���
	unsigned long	           channel;	          ///<ѹ��ͨ��
	unsigned short           media;		   ///<media type ��Ƶ����Ƶ
	unsigned short           type;		          ///<frame type	I/P/����...
	long                          len;	                 ///<frame_buf�е���Ч�ֽ���
	struct NCHUNK_HDR chunk;                ///<���ݿ�ͷ��־��Ŀǰʹ��avi��ʽ
	unsigned char            frame_buf[4];    ///<��ű�������Ƶ���ݵ���ʼ��ַ
}enc_frame_t;


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
 * ��ʾ�������ռ20�飬��ʾ������ݵ�2026��
 * ���800�� 20->800*12/512
 * 2026->24*3600*12/512
 */
	#define YEAR_HEAD_BLOCK_SIZE 20
	#define DATE_HEAD_BLOCK_SIZE 2026
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
struct day_block{ int time; long long seek;}__attribute__ ((packed));
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
#endif
