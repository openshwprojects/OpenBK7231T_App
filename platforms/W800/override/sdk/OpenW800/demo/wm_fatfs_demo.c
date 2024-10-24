
#include "wm_include.h"
#include "wm_gpio_afsel.h"
#include "ff.h"
#include "wm_demo.h"

#if DEMO_FATFS

#define TEST_DEBUG_EN           1
#if TEST_DEBUG_EN
#define TEST_DEBUG(fmt, ...)    printf("%s: "fmt, __func__, ##__VA_ARGS__)
#else
#define TEST_DEBUG(fmt, ...)
#endif

#define FATFS_TASK_SIZE        1024
#define FATFS_TASK_PRIO	       1

static OS_STK FatfsTaskStk[FATFS_TASK_SIZE];
static int console_cmd = 0;


//FAT���ܲ��ԣ���ʽ�����ԣ��ļ�д����ԣ��ļ���ȡ���ԣ��������ܣ�
int fatfs_func(void)
{
	FATFS fs; //FatFs�ļ�ϵͳ����
	FIL fnew; //�ļ�����
	FRESULT res_sd;//�ļ��������
	UINT fnum; //�ļ��ɹ���д����
	BYTE ReadBuffer[256] = {0};
	BYTE work[FF_MAX_SS];
	BYTE WriteBuffer[] = "�ɹ���ֲ��FatFs�ļ�ϵͳ��\r\n"; //д������
	

 	wm_sdio_host_config(0);

	//����SD��
	res_sd = f_mount(&fs, "0:", 1);
	
	//***********************��ʽ������****************************
	if(res_sd == FR_NO_FILESYSTEM)
	{
		while(1)
		{
			TEST_DEBUG("SD��û���ļ�ϵͳ���������и�ʽ��...\r\n");
			//��ʽ��
			
			res_sd = f_mkfs("0:", 0, work, sizeof(work));
			
			if(res_sd == FR_OK)
			{
				TEST_DEBUG("SD���ɹ���ʽ����\r\n");
				//��ʽ������ȡ������
				res_sd = f_mount(NULL, "0:", 1);
				//�����¹���
				res_sd = f_mount(&fs, "0:", 1);
				break;
			}
			else
			{
				TEST_DEBUG("�ļ���ʽ��ʧ�ܣ�������룺%d; will try again...\r\n",res_sd);
			}
		}
	}
	else if(res_sd != FR_OK)
	{
		TEST_DEBUG("�����ļ�ϵͳʧ�ܣ���������Ϊ�ļ���ʼ��ʧ�ܣ�������룺%d\r\n", res_sd);
	}
	else
	{
		TEST_DEBUG("�ļ�ϵͳ���سɹ��� �ɽ��ж�д���ԣ�\r\n");
	}
	
	//***********************д����****************************
	//���ļ�������ļ��������򴴽���
	TEST_DEBUG("���������ļ�д�����....\r\n");
	//���ļ����������ھʹ���
	res_sd = f_open(&fnew, "0:FatFs��д�����ļ�.txt", FA_CREATE_ALWAYS | FA_WRITE);
	//�ļ��򿪳ɹ�
	if(res_sd == FR_OK)
	{
		TEST_DEBUG("���ļ��ɹ�����ʼд�����ݣ�\r\n");
		res_sd= f_write(&fnew, WriteBuffer, sizeof(WriteBuffer), &fnum);
		
		if(res_sd == FR_OK)
		{
			TEST_DEBUG("����д��ɹ�����д��%d���ַ���\r\n", fnum);
			TEST_DEBUG("���ݣ�%s", WriteBuffer);
		}
		else
		{
			TEST_DEBUG("����д��ʧ�ܣ�\r\n");
		}
		
		//�ر��ļ�
		f_close(&fnew);
	}
	
	//***********************������****************************
	//���ļ�������ļ��������򴴽���
	TEST_DEBUG("���������ļ���ȡ����....\r\n");
	//���ļ����������ھʹ���
	res_sd = f_open(&fnew, "0:FatFs��д�����ļ�.txt", FA_OPEN_EXISTING | FA_READ);
	//�ļ��򿪳ɹ�
	if(res_sd == FR_OK)
	{
		TEST_DEBUG("���ļ��ɹ�����ʼ��ȡ���ݣ�\r\n");
		res_sd= f_read(&fnew, ReadBuffer, sizeof(ReadBuffer), &fnum);
		
		if(res_sd == FR_OK)
		{
			TEST_DEBUG("���ݶ�ȡ�ɹ���\r\n");
			TEST_DEBUG("���ݣ�%s\r\n", ReadBuffer);
		}
		else
		{
			TEST_DEBUG("���ݶ�ȡʧ�ܣ�\r\n");
		}
		
		//�ر��ļ�
		f_close(&fnew);
	}
	
	//ȡ�������ļ�ϵͳ
	f_mount(NULL, "0:", 1);

	return 0;
}


static void fatfs_task(void)
{

	while(1)
	{
		if( console_cmd==1 )
		{
			fatfs_func();
			console_cmd = 0;
		}
		tls_os_time_delay(HZ);
	}
}

int fatfs_test(void)
{
	static int flag = 0;

	console_cmd = 1;
	if( flag == 0 )
	{
		flag = 1;
	    tls_os_task_create(NULL, NULL,
	                       ( void (*))fatfs_task,
	                       NULL,
	                       (void *)FatfsTaskStk,          /* task's stack start address */
	                       FATFS_TASK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
	                       FATFS_TASK_PRIO,
	                       0);
	}
	return 0;
}

#endif

