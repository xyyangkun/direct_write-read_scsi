/*by wsy,for audio_pool connect and read etc @Dec 2006*/

#include <stdio.h>
#include <devinfo.h>
#include <devres.h>
#include <errno.h>
#include <mshmpool.h>
#include <media_api.h>
#include <aenc_read.h>
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


static media_source_t audio_enc[MAX_AUDIO_ENCODER];			//��Ƶ������ʵ��
/**********************************************************************************************
 * ������	:init_audio_enc()
 * ����	:��ʼ����Ƶ����������ؽṹ
 * ����	:��
 * ���	:�ڿ����Ѿ�������һ����̬��������Ƶ�������ṹ
 * ����ֵ	:0��ʾ�ɹ�����ֵ��ʾʧ��
 * ע		:�ڳ�������ʱ����һ��
 **********************************************************************************************/
int init_audio_enc(void)
{
	
	int i,ret,total;
	total = get_audio_num();
	if(total>MAX_AUDIO_ENCODER)
	{
		total=MAX_AUDIO_ENCODER;
		syslog(LOG_ERR,"get_audio_num=%d MAX_AUDIO_ENCODER=%d!!!\n",get_audio_num(),MAX_AUDIO_ENCODER);
		printf("get_audio_num=%d MAX_AUDIO_ENCODER=%d!!!\n",get_audio_num(),MAX_AUDIO_ENCODER);
	}
	
	for(i=0;i<total;i++)
	{
		ret=init_media_rw(&audio_enc[i],MEDIA_TYPE_AUDIO,i,1024);

	}
	
	return 0;	
}


/**********************************************************************************************
 * ������	:get_audio_enc()
 * ����	:��ȡ��Ƶ������������Ϣ�ṹָ��
 * ����	:no:��Ҫ���ʵ���Ƶ���������
 * ����ֵ	:ָ���Ӧ�����Ƶ�������Ľṹָ��,������NULL
 **********************************************************************************************/
media_source_t *get_audio_enc(IN int no)
{
	//if(no>=1)//fixme ���ж�·��ƵоƬʱ
	//	return NULL;	
	return &audio_enc[no];
}

/**********************************************************************************************
 * ������	:get_audio_read_buf()
 * ����	:��ȡָ�����������ڶ�ȡ���ݵ���ʱ������ָ��
 * ����	:no:��Ҫ���ʵ���Ƶ���������
 * ����ֵ	:ָ���Ӧ�����Ƶ�������Ľṹָ��,������NULL
 **********************************************************************************************/
void *get_audio_read_buf(IN int no)
{
	if(no>=1)//fixme ���ж�·��ƵоƬʱ
		return NULL;	
	return audio_enc[no].temp_buf;
}
/**********************************************************************************************
 * ������	:get_audio_read_buf_len()
 * ����	:��ȡָ�����������ڶ�ȡ���ݵ���ʱ����������
 * ����	:no:��Ҫ���ʵ���Ƶ���������
 * ����ֵ	:��ʱ�������ĳ���
 **********************************************************************************************/
int get_audio_read_buf_len(IN int no)
{
	if(no>=1)//fixme ���ж�·��ƵоƬʱ
		return -EINVAL;	
	return audio_enc[no].buflen;
}

/**********************************************************************************************
 * ������	:connect_audio_enc()
 * ����	:���ӵ�ָ����ŵ���Ƶ������
 * ����	:no:��Ҫ���ӵ���Ƶ���������
 *			name:�û���
 *			pre_sec:��ǰ����pre_sec���λ�ÿ�ʼ����
 *					0:��ʾ�����µ�Ԫ������
 *				      >0:��ʾҪ��ǰ����
 *					   ���pre_sec��ֵ���ڻ�����е���֡����������Ԫ�ؽ�������
 * ����ֵ	:0��ʾ�ɹ�����ֵ��ʾʧ��
 **********************************************************************************************/
int connect_audio_enc(IN int no,IN char *name,IN int pre_sec)
{
	int ret;
	int rc;
	int stat;
	int frate=0;
	media_attrib_t *attrib=NULL;
	media_format_t *media=NULL;
	audio_format_t *audio=NULL;
	if(no>=get_audio_num())
		return -EINVAL;
	ret=connect_media_read(&audio_enc[no], get_audio_enc_key(no),name,MSHMPOOL_NET_USR);
	if(ret>=0)
	{
		stat=get_aenc_stat(no);
		if(stat==ENC_STAT_OK)
		{//������״̬����
			attrib=get_aenc_attrib(no);
			if(attrib!=NULL)
			{
				media=&attrib->fmt;
				audio=&media->a_fmt;
				frate=(audio->a_frate);
				rc=move_media_place(&audio_enc[no],-(frate*pre_sec));
			}
		}
	}
	return ret;
}
/** 
 *   @brief     ������Ƶ����ֱ���ɹ�
 *   @param  no ��Ƶ���������
 *   @param  name Ӧ�ó�����
 *   @return   �Ǹ���ʾ�ɹ�,��ֵ��ʾʧ��
 */ 
int connect_audio_enc_succ(int no,char *name)
{
    int ret;
    int fail_cnt=0;         //ʧ�ܴ���
    if(no>get_audio_num())
        return -EINVAL;

    ///������Ƶ������
    while(1)
    {
    	ret=connect_audio_enc(no,name,0);
    	if(ret==0)
    	{
    		printf     ("connect auidoenc%d success\n",no);
              gtloginfo("connect auidoenc%d success\n",no);
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
           printf("connect auido enc(%d) failed(%d), ret=%d!!\n",no,fail_cnt,ret);
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
    	ret=get_aenc_stat(no);
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
    		printf("auidoenc%d state=%d!!!\n",no,ret);	
    	}
    	sleep(1);
    }    
    return ret;

}

/**********************************************************************************************
 * ������	:disconnect_audio_enc()
 * ����	:�Ͽ���ָ��������������
 * ����	:no:��Ҫ�Ͽ�����Ƶ���������
 * ����ֵ	:0��ʾ�ɹ�����ֵ��ʾʧ��
 **********************************************************************************************/
int disconnect_audio_enc(IN int no)
{
	if(no>=get_audio_num())
		return -EINVAL;	
	return disconnect_media_read(&audio_enc[no]);
}

/**********************************************************************************************
 * ������	:reactive_audio_enc()
 * ����	:���¼����Ƶ������������
 * ����	:no:��Ҫ���¼������Ƶ���������
 * ����ֵ	:0��ʾ�ɹ�����ֵ��ʾʧ��
 * ע		:Ӧ�ó���Ӧ���ڵ���,��ֹ�����Լ�һ��ʱ��û����Ӧ����
 *			 audioenc����Ͽ�
 **********************************************************************************************/
int reactive_audio_enc(IN int no)
{
	if(no>=1)//fixme ���ж�·��ƵоƬʱ
		return -EINVAL;	
	return reactive_media_usr(&audio_enc[no]);
}

/**********************************************************************************************
 * ������	:read_audio_frame()
 * ����	:��ָ����ŵı������ж�ȡһ֡����
 * ����	:no:��Ҫ��ȡ���ݵ���Ƶ���������
 *			:buf_len:frame�������ĳ���,Ҫ��һ֡���ݳ������������ᱨ��
 * ���	:frame:׼�������Ƶ֡�Ļ�����
 *			 eleseq:��Ƶ֡�����
 *			 flag:��Ƶ֡�ı�־
 * ����ֵ	:��ֵ��ʾ��ȡ�����ֽ�������ֵ��ʾ����
 **********************************************************************************************/
int read_audio_frame(IN int no,OUT void *frame,IN int buf_len,OUT int *eleseq,OUT int *flag)
{
	return read_media_resource(&audio_enc[no],frame,buf_len,eleseq,flag);
}

//#define 	ENC_NO_INIT		0		//������û�г�ʼ��
//#define	ENC_STAT_OK		1		//��������������
//#define	ENC_STAT_ERR		2		//����������	
/**********************************************************************************************
 * ������	:get_aenc_stat()
 * ����	:��ȡָ����ŵ���Ƶ������״̬
 * ����	:no:��Ҫ��ȡ״̬����Ƶ���������
 * ����ֵ	:��ֵ��ʾ����-EINVAL:�������� -ENOENT:�豸��û������
 *					ENC_NO_INIT:δ��ʼ��
 *					ENC_STAT_OK:״̬����
 *					ENC_STAT_ERR:����������
 **********************************************************************************************/
#include <file_def.h>//test !!!
int get_aenc_stat(IN int no)
{
	media_source_t 	*media=&audio_enc[no];
	media_attrib_t	*attrib=NULL;
	if(no>=get_audio_num())
		return -EINVAL;
	if(media->dev_stat<0)
		return -ENOENT;
	attrib=media->attrib;
	if(attrib==NULL)
		return -ENOENT;
	return attrib->stat;
}



/**********************************************************************************************
 * ������	:get_aenc_attrib()
 * ����	:��ȡָ����Ƶ�������ĸ�����Ϣ�ṹָ��
 * ����	:no:��Ҫ���ʵ���Ƶ���������
 * ����ֵ	:ָ��������Ƶ�������������ԵĽṹָ��
 *			 NULL��ʾ���� ���������󣬱�����δ����
 **********************************************************************************************/
media_attrib_t *get_aenc_attrib(int no)
{
	media_source_t 	*media=&audio_enc[no];
	if(no>=get_audio_num())
		return NULL;
	if(media->dev_stat<0)
		return NULL;
	return media->attrib;
}

