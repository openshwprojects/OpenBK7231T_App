
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


//FAT功能测试：格式化测试，文件写入测试，文件读取测试（基本功能）
int fatfs_func(void)
{
	FATFS fs; //FatFs文件系统对象
	FIL fnew; //文件对象
	FRESULT res_sd;//文件操作结果
	UINT fnum; //文件成功读写数量
	BYTE ReadBuffer[256] = {0};
	BYTE work[FF_MAX_SS];
	BYTE WriteBuffer[] = "成功移植了FatFs文件系统！\r\n"; //写缓存区
	

 	wm_sdio_host_config(0);

	//挂载SD卡
	res_sd = f_mount(&fs, "0:", 1);
	
	//***********************格式化测试****************************
	if(res_sd == FR_NO_FILESYSTEM)
	{
		while(1)
		{
			TEST_DEBUG("SD卡没有文件系统，即将进行格式化...\r\n");
			//格式化
			
			res_sd = f_mkfs("0:", 0, work, sizeof(work));
			
			if(res_sd == FR_OK)
			{
				TEST_DEBUG("SD卡成功格式化！\r\n");
				//格式化后先取消挂载
				res_sd = f_mount(NULL, "0:", 1);
				//再重新挂载
				res_sd = f_mount(&fs, "0:", 1);
				break;
			}
			else
			{
				TEST_DEBUG("文件格式化失败！错误代码：%d; will try again...\r\n",res_sd);
			}
		}
	}
	else if(res_sd != FR_OK)
	{
		TEST_DEBUG("挂载文件系统失败！可能是因为文件初始化失败！错误代码：%d\r\n", res_sd);
	}
	else
	{
		TEST_DEBUG("文件系统挂载成功， 可进行读写测试！\r\n");
	}
	
	//***********************写测试****************************
	//打开文件，如果文件不存在则创建它
	TEST_DEBUG("即将进行文件写入测试....\r\n");
	//打开文件，若不存在就创建
	res_sd = f_open(&fnew, "0:FatFs读写测试文件.txt", FA_CREATE_ALWAYS | FA_WRITE);
	//文件打开成功
	if(res_sd == FR_OK)
	{
		TEST_DEBUG("打开文件成功！开始写入数据！\r\n");
		res_sd= f_write(&fnew, WriteBuffer, sizeof(WriteBuffer), &fnum);
		
		if(res_sd == FR_OK)
		{
			TEST_DEBUG("数据写入成功，共写入%d个字符！\r\n", fnum);
			TEST_DEBUG("数据：%s", WriteBuffer);
		}
		else
		{
			TEST_DEBUG("数据写入失败！\r\n");
		}
		
		//关闭文件
		f_close(&fnew);
	}
	
	//***********************读测试****************************
	//打开文件，如果文件不存在则创建它
	TEST_DEBUG("即将进行文件读取测试....\r\n");
	//打开文件，若不存在就创建
	res_sd = f_open(&fnew, "0:FatFs读写测试文件.txt", FA_OPEN_EXISTING | FA_READ);
	//文件打开成功
	if(res_sd == FR_OK)
	{
		TEST_DEBUG("打开文件成功！开始读取数据！\r\n");
		res_sd= f_read(&fnew, ReadBuffer, sizeof(ReadBuffer), &fnum);
		
		if(res_sd == FR_OK)
		{
			TEST_DEBUG("数据读取成功！\r\n");
			TEST_DEBUG("数据：%s\r\n", ReadBuffer);
		}
		else
		{
			TEST_DEBUG("数据读取失败！\r\n");
		}
		
		//关闭文件
		f_close(&fnew);
	}
	
	//取消挂载文件系统
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

