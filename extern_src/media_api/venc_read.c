
#include <stdio.h>
//#include <devinfo.h>
//#include <devres.h>
#include <errno.h>
#include <mshmpool.h>
#include <media_api.h>
#include <venc_read.h>
#include <syslog.h>

#include <syslog.h>

//����־��¼����
//name��ʾ��־��Ϣ�е�����
#define gtopenlog(name) openlog(name,LOG_CONS|LOG_NDELAY|LOG_PID,LOG_LOCAL0 );//LOG_USER);

//#define gtlog  syslog		//ϵͳ��־��Ϣ��¼
#define gtlog syslog
//һ������Ϣ
#define gtloginfo(args...) syslog(LOG_INFO,##args)	//��¼һ����Ϣ
//���صĴ�����Ϣ
#define gtlogfault(args...) syslog(LOG_CRIT,##args)	//
//������Ϣ
#define gtlogerr(args...) syslog(LOG_ERR,##args)	//
//������Ϣ
#define gtlogwarn(args...) syslog(LOG_WARNING,##args)

#define gtlogdebug(args...) syslog(LOG_DEBUG,##args)



static media_source_t video_enc[MAX_VIDEO_ENCODER];			//��Ƶ������ʵ��
static media_source_t video_enc_keyframe[MAX_VIDEO_ENCODER];	
static media_source_t video_enc_playback[MAX_VIDEO_ENCODER];//zsk add �طŻ����
/**********************************************************************************************
 * ������	:init_video_enc()
 * ����	:��ʼ����Ƶ����������ؽṹ
 * ����	:��
 * ���	:�ڿ����Ѿ�������һ����̬��������Ƶ�������ṹ
 * ����ֵ	:0��ʾ�ɹ�����ֵ��ʾʧ��
 * ע		:�ڳ�������ʱ����һ��
 **********************************************************************************************/
int init_video_enc(void)
{
	int i,ret,total;
	total=get_videoenc_num();
	if(total>MAX_VIDEO_ENCODER)
	{
		total=MAX_VIDEO_ENCODER;
		syslog(LOG_ERR,"get_videoenc_num=%d MAX_VIDEO_ENCODER=%d!!!\n",get_videoenc_num(),MAX_VIDEO_ENCODER);
		printf("get_videoenc_num=%d MAX_VIDEO_ENCODER=%d!!!\n",get_videoenc_num(),MAX_VIDEO_ENCODER);
	}

	for(i=0;i<total;i++)
	
	{
		ret=init_media_rw(&video_enc[i],MEDIA_TYPE_VIDEO,i,MAX_FRAME_SIZE);
		//ret=init_media_rw(&video_enc_keyframe[i],MEDIA_TYPE_VIDEO,i,MAX_FRAME_SIZE);
		ret= init_media_rw(&video_enc_playback[i],MEDIA_TYPE_VIDEO,i,MAX_FRAME_SIZE);
	}
	
	return 0;	
}


/**********************************************************************************************
 * ������	:get_video_enc()
 * ����	:��ȡ��Ƶ������������Ϣ�ṹָ��
 * ����	:no:��Ҫ���ʵ���Ƶ���������
 * ����ֵ	:ָ���Ӧ�����Ƶ�������Ľṹָ��,������NULL
 **********************************************************************************************/
media_source_t *get_video_enc(IN int no)
{

	if(no>=get_videoenc_num())
		return NULL;	
	return &video_enc[no];
}
/**********************************************************************************************
 * ������	:get_video_enc_playback()
 * ����	:��ȡ��Ƶ������ �������Ϣ�ṹ�ط�ָ��
 * ����	:no:��Ҫ���ʵ���Ƶ���������
 * ����ֵ	:ָ���Ӧ�����Ƶ�������Ľṹָ��,������NULL
 **********************************************************************************************/

media_source_t *get_video_enc_playback(IN int no)
{
	if(no>=get_videoenc_num())
		return NULL;	
	return &video_enc_playback[no];
}

media_source_t *get_video_enc_keyframe(IN int no)
{
	if(no>=get_videoenc_num())
		return NULL;	
	return &video_enc_keyframe[no];
}



/**********************************************************************************************
 * ������	:get_video_read_buf()
 * ����	:��ȡָ�����������ڶ�ȡ���ݵ���ʱ������ָ��
 * ����	:no:��Ҫ���ʵ���Ƶ���������
 * ����ֵ	:ָ���Ӧ�����Ƶ�������Ľṹָ��,������NULL
 **********************************************************************************************/
void *get_video_read_buf(IN int no)
{
	if(no>=get_videoenc_num())
		return NULL;	
	return video_enc[no].temp_buf;
}

void *get_video_read_keyframe_buf(IN int no)
{
	if(no>=get_videoenc_num())
		return NULL;	
	return video_enc_keyframe[no].temp_buf;
}


/**********************************************************************************************
 * ������	:get_video_read_buf_len()
 * ����	:��ȡָ�����������ڶ�ȡ���ݵ���ʱ����������
 * ����	:no:��Ҫ���ʵ���Ƶ���������
 * ����ֵ	:��ʱ�������ĳ���
 **********************************************************************************************/
int get_video_read_buf_len(IN int no)
{
	if(no>=get_videoenc_num())
		return -EINVAL;	
	return video_enc[no].buflen;
}
int get_video_read_keyframe_buf_len(IN int no)
{
	if(no>=get_videoenc_num())
		return -EINVAL;	
	return video_enc_keyframe[no].buflen;
}
/**********************************************************************************************
 * ������	:connect_video_enc()
 * ����	:���ӵ�ָ����ŵ���Ƶ������
 * ����	:no:��Ҫ���ӵ���Ƶ���������
 *			name:�û���
 *			pre_sec:��ǰ����pre_sec���λ�ÿ�ʼ����
 *					0:��ʾ�����µ�Ԫ������
 *				      >0:��ʾҪ��ǰ����
 *					   ���pre_sec��ֵ���ڻ�����е���֡����������Ԫ�ؽ�������
 * ����ֵ	:0��ʾ�ɹ�����ֵ��ʾʧ��
 **********************************************************************************************/
int connect_video_enc(IN int no,IN char *name,IN int pre_sec)
{
	int ret;
	int rc;
	int stat;
	int frate=0;
	media_attrib_t *attrib=NULL;
	video_format_t *video=NULL;
	if(no>=get_videoenc_num())
		return -EINVAL;
	ret=connect_media_read(&video_enc[no], get_video_enc_key(no),name,MSHMPOOL_NET_USR);
	//ret=connect_media_read(&video_enc[no], key,name,MSHMPOOL_NET_USR);
	if(ret>=0)
	{
		stat=get_venc_stat(no);
		if(stat==ENC_STAT_OK)
		{//������״̬����
			attrib=get_venc_attrib(no);
			if(attrib!=NULL)
			{
				video=&attrib->fmt.v_fmt;
				frate=video->v_frate;
				rc=move_media_place(&video_enc[no],-(frate*pre_sec));				
			}
		}
	}
	return ret;
}
/** 
 *   @brief     ������Ƶ������ֱ���ɹ�
 *   @param  no ��Ƶ���������
 *   @param  name Ӧ�ó�����
 *   @return   �Ǹ���ʾ�ɹ�,��ֵ��ʾʧ��
 */ 
int connect_video_enc_succ(int no,char *name,int pre_sec)
{
    int ret;
    int fail_cnt=0;         //ʧ�ܴ���
    if(no>get_videoenc_num())
        return -EINVAL;

    ///������Ƶ������
    while(1)
    {
    	ret=connect_video_enc(no,name,pre_sec);
    	if(ret==0)
    	{
    		printf     ("connect videoenc%d success\n",no);
              gtloginfo("connect videoenc%d success\n",no);
    		break;
    	}
    	else
    	{
    	    if(fail_cnt==0)
    	    {
    	        printf     ("������Ƶ������%dʧ��\n",no);
    	        gtloginfo("������Ƶ������%dʧ��\n",no);
    	        
    	    }
           fail_cnt++;
           printf("connect video enc(%d) failed(%d), ret=%d!!\n",no,fail_cnt,ret);
           if(fail_cnt==40)
           {
                printf    ("������Ƶ������%dʧ��%d��!!!",no,fail_cnt);
                gtlogerr("������Ƶ������%dʧ��%d��!!!",no,fail_cnt);
           }
    		sleep(2);
    	}
    }	

    ///�ȴ���Ƶ����������
    fail_cnt=0;
    while(1)
    {
    	ret=get_venc_stat(no);
    	if(ret==ENC_STAT_OK)
    	{
    		printf("��Ƶ������%d״̬����!\n",no);
    		break;
    	}
    	else
    	{
    		if(++fail_cnt==15)
    		{
    			printf    ("��Ƶ������%d״̬�쳣,stat=%d!\n",no,ret);
    			gtlogerr("��Ƶ������%d״̬�쳣,stat=%d!\n",no,ret);
    		}
    		printf("videoenc%d state=%d!!!\n",no,ret);	
    	}
    	sleep(1);
    }    
    return ret;

}
/**********************************************************************************************
 * ������	:connect_video_enc_playback()
 * ����	:���ӵ�ָ����ŵ���Ƶ��������¼�����¼��طŻ�����
 * ����	:no:��Ҫ���ӵ���Ƶ���������
 *			name:�û���
 *			pre_sec:��ǰ����pre_sec���λ�ÿ�ʼ����
 *					0:��ʾ�����µ�Ԫ������
 *				      >0:��ʾҪ��ǰ����
 *					   ���pre_sec��ֵ���ڻ�����е���֡����������Ԫ�ؽ�������
 * ����ֵ	:0��ʾ�ɹ�����ֵ��ʾʧ��
 **********************************************************************************************/
int connect_video_enc_playback(IN int no,IN char *name,IN int pre_sec)
{
	int ret;
	int rc;
	int stat;
	int frate=0;
	media_attrib_t *attrib=NULL;
	video_format_t *video=NULL;
	if(no>=get_videoenc_num())
		return -EINVAL;
	ret=connect_media_read(&video_enc_playback[no], get_video_enc_key(no),name,MSHMPOOL_NET_USR);
	if(ret>=0)
	{
		stat=get_venc_stat(no);
		if(stat==ENC_STAT_OK)
		{//������״̬����
			attrib=get_venc_attrib(no);
			if(attrib!=NULL)
			{
				video=&attrib->fmt.v_fmt;
				frate=video->v_frate;
				rc=move_media_place(&video_enc_playback[no],-(frate*pre_sec));	
				
			}
		}
	}
	return ret;
}

/**********************************************************************************************
 * ������	:connect_video_enc_keyframe()
 * ����	:I֡������߳�ר��,���ӵ�ָ����ŵ���Ƶ������
 * ����	:no:��Ҫ���ӵ���Ƶ���������
 *	name:�û���
 * ����ֵ	:0��ʾ�ɹ�����ֵ��ʾʧ��
 **********************************************************************************************/
int connect_video_enc_keyframe(IN int no,char *name)
{
	int ret;
	
	if(no>=get_videoenc_num())
		return -EINVAL;
	ret=connect_media_read(&video_enc_keyframe[no], get_video_enc_key(no),name,MSHMPOOL_NET_USR);
	return ret;
}


/**********************************************************************************************
 * ������	:disconnect_video_enc()
 * ����	:�Ͽ���ָ��������������
 * ����	:no:��Ҫ�Ͽ�����Ƶ���������
 * ����ֵ	:0��ʾ�ɹ�����ֵ��ʾʧ��
 **********************************************************************************************/
int disconnect_video_enc(IN int no)
{
	if(no>=get_videoenc_num())
		return -EINVAL;	
	return disconnect_media_read(&video_enc[no]);
}
int disconnect_video_enc_keyframe(IN int no)
{
	if(no>=get_videoenc_num())
		return -EINVAL;	
	return disconnect_media_read(&video_enc_keyframe[no]);
}
/**********************************************************************************************
 * ������	:reactive_video_enc()
 * ����	:���¼����Ƶ������������
 * ����	:no:��Ҫ���¼������Ƶ���������
 * ����ֵ	:0��ʾ�ɹ�����ֵ��ʾʧ��
 * ע		:Ӧ�ó���Ӧ���ڵ���,��ֹ�����Լ�һ��ʱ��û����Ӧ����
 *			 videoenc����Ͽ�
 **********************************************************************************************/
int reactive_video_enc(IN int no)
{
	if(no>=get_videoenc_num())
		return -EINVAL;	
	return reactive_media_usr(&video_enc[no]);
}

int get_video_enc_remain(IN int no)
{
    return read_media_remain(&video_enc[no]);
}
int get_video_enc_playback_remain(IN int no)
{
    return read_media_remain(&video_enc_playback[no]);
}

int reactive_video_enc_keyframe(IN int no)
{
	if(no>=get_videoenc_num())
		return -EINVAL;	
	return reactive_media_usr(&video_enc_keyframe[no]);
}

/**********************************************************************************************
 * ������	:read_video_frame()
 * ����	:��ָ����ŵı������ж�ȡһ֡����
 * ����	:no:��Ҫ��ȡ���ݵ���Ƶ���������
 *			:buf_len:frame�������ĳ���,Ҫ��һ֡���ݳ������������ᱨ��
 * ���	:frame:׼�������Ƶ֡�Ļ�����
 *			 eleseq:��Ƶ֡�����
 *			 flag:��Ƶ֡�ı�־
 * ����ֵ	:��ֵ��ʾ��ȡ�����ֽ�������ֵ��ʾ����
 **********************************************************************************************/
int read_video_frame(IN int no,OUT void *frame,IN int buf_len,OUT int *eleseq,OUT int *flag)
{
	return read_media_resource(&video_enc[no],frame,buf_len,eleseq,flag);
}

/**********************************************************************************************
 * ������	:read_video_playback()
 * ����	:��ָ����ŵı������ж�ȡһ֡�ط�����
 * ����	:no:��Ҫ��ȡ���ݵ���Ƶ���������
 *			:buf_len:frame�������ĳ���,Ҫ��һ֡���ݳ������������ᱨ��
 * ���	:frame:׼�������Ƶ֡�Ļ�����
 *			 eleseq:��Ƶ֡�����
 *			 flag:��Ƶ֡�ı�־
 * ����ֵ	:��ֵ��ʾ��ȡ�����ֽ�������ֵ��ʾ����
 **********************************************************************************************/

int read_video_playback(IN int no,OUT void *frame,IN int buf_len,OUT int *eleseq,OUT int *flag)
{
	return read_media_resource(&video_enc_playback[no],frame,buf_len,eleseq,flag);
}

int read_move_media_place(IN int no,int place)
{
	return move_media_place(&video_enc[no], place);
}


/**********************************************************************************************
 * ������	:read_video_keyframe()
 * ����	:��ָ����ŵı������ж�ȡһ֡����,��I֡�����ר��
 * ����	:no:��Ҫ��ȡ���ݵ���Ƶ���������
 *			:buf_len:frame�������ĳ���,Ҫ��һ֡���ݳ������������ᱨ��
 * ���	:frame:׼�������Ƶ֡�Ļ�����
 *			 eleseq:��Ƶ֡�����
 *			 flag:��Ƶ֡�ı�־
 * ����ֵ	:��ֵ��ʾ��ȡ�����ֽ�������ֵ��ʾ����
 **********************************************************************************************/
int read_video_keyframe(IN int no,OUT void *frame,IN int buf_len,OUT int *eleseq,OUT int *flag)
{
	return read_media_resource(&video_enc_keyframe[no],frame,buf_len,eleseq,flag);
}


//#define 	ENC_NO_INIT		0		//������û�г�ʼ��
//#define	ENC_STAT_OK		1		//��������������
//#define	ENC_STAT_ERR		2		//����������	
/**********************************************************************************************
 * ������	:get_venc_stat()
 * ����	:��ȡָ����ŵ���Ƶ������״̬
 * ����	:no:��Ҫ��ȡ״̬����Ƶ���������
 * ����ֵ	:��ֵ��ʾ����-EINVAL:�������� -ENOENT:�豸��û������
 *					ENC_NO_INIT:δ��ʼ��
 *					ENC_STAT_OK:״̬����
 *					ENC_STAT_ERR:����������
 **********************************************************************************************/
//#include <file_def.h>//test !!!
int get_venc_stat(IN int no)
{
	media_source_t 	*media=&video_enc[no];
	media_attrib_t	*attrib=NULL;
	if(no>=get_videoenc_num())
		return -EINVAL;
	if(media->dev_stat<0)
		return -ENOENT;
	attrib=media->attrib;
	if(attrib==NULL)
		return -ENOENT;
	return attrib->stat;
}


int get_venc_stat_keyframe(IN int no)
{
	media_source_t 	*media=&video_enc_keyframe[no];
	media_attrib_t	*attrib=NULL;
	if(no>=get_videoenc_num())
		return -EINVAL;
	if(media->dev_stat<0)
		return -ENOENT;
	attrib=media->attrib;
	return attrib->stat;
}

/**********************************************************************************************
 * ������	:get_venc_attrib()
 * ����	:��ȡָ����Ƶ�������ĸ�����Ϣ�ṹָ��
 * ����	:no:��Ҫ���ʵ���Ƶ���������
 * ����ֵ	:ָ��������Ƶ�������������ԵĽṹָ��
 *			 NULL��ʾ���� ���������󣬱�����δ����
 **********************************************************************************************/
media_attrib_t *get_venc_attrib(int no)
{
	media_source_t 	*media=&video_enc[no];
	if(no>=get_videoenc_num())
		return NULL;
	if(media->dev_stat<0)
		return NULL;
	return media->attrib;
}
media_attrib_t *get_venc_attrib_keyframe(int no)
{
	media_source_t 	*media=&video_enc_keyframe[no];
	if(no>=get_videoenc_num())
		return NULL;
	if(media->dev_stat<0)
		return NULL;
	return media->attrib;
}
