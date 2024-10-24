#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <dirent.h>
#include <unistd.h>
#include <stdarg.h>
#ifdef __MINGW32__
#include <windows.h>
#else
#include <signal.h>
#include <pthread.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

#include "zlib-1.2.11/zlib.h"
#include "zlib-1.2.11/gzguts.h"
void gz_compress      OF((FILE   *in, gzFile out));
void file_compress    OF((char  *file, char *mode));

#ifndef GZ_SUFFIX
#  define GZ_SUFFIX ".gz"
#endif
#define SUFFIX_LEN (sizeof(GZ_SUFFIX)-1)

#define BUFLEN      4096
#define MAX_NAME_LEN 1024
#ifndef local
#  define local
#endif
#define WM_TOOL_PATH_MAX                      256
#define WM_TOOL_ONCE_READ_LEN	              1024

#define WM_TOOL_RUN_IMG_HEADER_LEN            0x100

#define WM_TOOL_SECBOOT_IMG_ADDR             (0x2100)
#define WM_TOOL_SECBOOT_HEADER_LEN           (0x100)
#define WM_TOOL_SECBOOT_HEADER_POS           (WM_TOOL_SECBOOT_IMG_ADDR - WM_TOOL_SECBOOT_HEADER_LEN)
//#define WM_TOOL_SECBOOT_IMG_AREA_TOTAL_LEN    9216//(56 * 1024)

#define WM_TOOL_IMG_HEAD_MAGIC_NO            (0xA0FFFF9F)

#define WM_TOOL_DEFAULT_BAUD_RATE             115200

#define WM_TOOL_DOWNLOAD_TIMEOUT_SEC          (60 * 1)

#define WM_TOOL_USE_1K_XMODEM                  1  /* 1 for use 1k_xmodem 0 for xmodem */

#define WM_TOOL_IMAGE_VERSION_LEN              16

/* Xmodem Frame form: <SOH><blk #><255-blk #><--128 data bytes--><CRC hi><CRC lo> */
#define XMODEM_SOH 0x01
#define XMODEM_STX 0x02
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_CAN 0x18
#define XMODEM_CRC_CHR 'C'
#define XMODEM_CRC_SIZE 2      /* Crc_High Byte + Crc_Low Byte */
#define XMODEM_FRAME_ID_SIZE 2 /* Frame_Id + 255-Frame_Id */
#define XMODEM_DATA_SIZE_SOH 128   /* for Xmodem protocol */
#define XMODEM_DATA_SIZE_STX 1024 /* for 1K xmodem protocol */

#if (WM_TOOL_USE_1K_XMODEM)
#define XMODEM_DATA_SIZE  XMODEM_DATA_SIZE_STX
#define XMODEM_HEAD XMODEM_STX
#else
#define XMODEM_DATA_SIZE XMODEM_DATA_SIZE_SOH
#define XMODEM_HEAD XMODEM_SOH
#endif

#ifdef WM_DEBUG
#define WM_TOOL_DBG_PRINT       wm_tool_printf
#else
#define WM_TOOL_DBG_PRINT(...)
#endif

typedef enum {
    WM_TOOL_DL_ACTION_NONE = 0,
    WM_TOOL_DL_ACTION_AT,
    WM_TOOL_DL_ACTION_RTS
} wm_tool_dl_action_e;

typedef enum {
    WM_TOOL_DL_ERASE_NONE = 0,
    WM_TOOL_DL_ERASE_ALL
} wm_tool_dl_erase_e;

typedef enum {
    WM_TOOL_DL_TYPE_IMAGE = 0,
    WM_TOOL_DL_TYPE_FLS
} wm_tool_dl_type_e;

typedef enum {
    WM_TOOL_LAYOUT_TYPE_1M = 0,
    WM_TOOL_LAYOUT_TYPE_2M = 3
} wm_tool_layout_type_e;

typedef enum {
    WM_TOOL_ZIP_TYPE_UNCOMPRESS = 0,
    WM_TOOL_ZIP_TYPE_COMPRESS
} wm_tool_zip_type_e;

typedef enum {
    WM_TOOL_CRC32_REFLECT_OUTPUT = 1,
    WM_TOOL_CRC32_REFLECT_INPUT  = 2
} wm_tool_crc32_reflect_e;

typedef enum {
    WM_TOOL_SHOW_LOG_NONE = 0,
    WM_TOOL_SHOW_LOG_STR,
    WM_TOOL_SHOW_LOG_HEX
} wm_tool_show_log_e;

typedef struct {
    unsigned int   magic_no;
    unsigned short img_type;
    unsigned short zip_type;
    unsigned int   run_img_addr;
    unsigned int   run_img_len;
    unsigned int   img_header_addr;
    unsigned int   upgrade_img_addr;
    unsigned int   run_org_checksum;
    unsigned int   upd_no;
    unsigned char  ver[WM_TOOL_IMAGE_VERSION_LEN];
    unsigned int   reserved0;
    unsigned int   reserved1;
    unsigned int   next_boot;	
    unsigned int   hd_checksum;
} wm_tool_firmware_booter_t;

const static char *wm_tool_version = "1.0.4";

static int wm_tool_show_usage = 0;
static int wm_tool_list_com   = 0;
static int wm_tool_show_ver   = 0;

static char wm_tool_serial_path[WM_TOOL_PATH_MAX] = "/dev/ttyS0";
static unsigned int wm_tool_download_serial_rate  = WM_TOOL_DEFAULT_BAUD_RATE;
static unsigned int wm_tool_normal_serial_rate    = WM_TOOL_DEFAULT_BAUD_RATE;

static wm_tool_dl_action_e wm_tool_dl_action = WM_TOOL_DL_ACTION_NONE;
static wm_tool_dl_erase_e  wm_tool_dl_erase  = WM_TOOL_DL_ERASE_NONE;
static wm_tool_dl_type_e   wm_tool_dl_type   = WM_TOOL_DL_TYPE_IMAGE;
static char *wm_tool_download_image = NULL;

static char *wm_tool_input_binary = NULL;
static char *wm_tool_output_image = NULL;
static char *wm_tool_secboot_image = NULL;
static unsigned int wm_tool_src_binary_len = 0;
static unsigned int wm_tool_src_binary_crc = 0;
static int wm_tool_is_debug = 0;
static char wm_tool_image_version[WM_TOOL_IMAGE_VERSION_LEN];
static wm_tool_layout_type_e wm_tool_image_type = WM_TOOL_LAYOUT_TYPE_1M;
static wm_tool_zip_type_e wm_tool_zip_type = WM_TOOL_ZIP_TYPE_COMPRESS;
static unsigned int wm_tool_upd_addr = 0x8090000;
static unsigned int wm_tool_run_addr = 0x8002400;

static unsigned int wm_tool_image_header = 0x8002000;
static unsigned int wm_tool_next_image_header = 0x0;
static unsigned int wm_tool_image_upd_no = 0x0;

static unsigned int wm_tool_file_crc = 0xFFFFFFFF;

static wm_tool_show_log_e wm_tool_show_log_type = WM_TOOL_SHOW_LOG_NONE;

#ifdef __MINGW32__

#ifndef CBR_2000000
#define CBR_2000000 2000000
#endif

#ifndef CBR_1000000
#define CBR_1000000 1000000
#endif

#ifndef CBR_921600
#define CBR_921600 921600
#endif

#ifndef CBR_460800
#define CBR_460800 460800
#endif

static DWORD wm_tool_uart_block = 0;

static HANDLE wm_tool_uart_handle = NULL;

const static int wm_tool_uart_speed_array[] = {CBR_2000000, CBR_1000000, CBR_921600, CBR_460800, CBR_115200, CBR_38400,
                                               CBR_19200,   CBR_9600,    CBR_4800,   CBR_2400,   CBR_1200};
#else /* __MINGW32__ */

#ifndef B2000000
#define B2000000 2000000
#endif

#ifndef B1000000
#define B1000000 1000000
#endif

#ifndef B921600
#define B921600 921600
#endif

#ifndef B460800
#define B460800 460800
#endif

static int wm_tool_uart_fd = -1;

static struct termios wm_tool_saved_serial_cfg;


const static int wm_tool_uart_speed_array[] = {B2000000, B1000000, B921600, B460800, B115200, B38400,
                                               B19200,   B9600,    B4800,   B2400,   B1200};

#endif /* __MINGW32__ */

const static int wm_tool_uart_name_array[] = {2000000, 1000000, 921600, 460800, 115200, 38400,
                                              19200,   9600,    4800,   2400,   1200};

const static unsigned char wm_tool_chip_cmd_b115200[]  = {0x21, 0x0a, 0x00, 0x97, 0x4b, 0x31, 0x00, 0x00, 0x00, 0x00, 0xc2, 0x01, 0x00};
const static unsigned char wm_tool_chip_cmd_b460800[]  = {0x21, 0x0a, 0x00, 0x07, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00, 0x08, 0x07, 0x00};
const static unsigned char wm_tool_chip_cmd_b921600[]  = {0x21, 0x0a, 0x00, 0x5d, 0x50, 0x31, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0e, 0x00};
const static unsigned char wm_tool_chip_cmd_b1000000[] = {0x21, 0x0a, 0x00, 0x5e, 0x3d, 0x31, 0x00, 0x00, 0x00, 0x40, 0x42, 0x0f, 0x00};
const static unsigned char wm_tool_chip_cmd_b2000000[] = {0x21, 0x0a, 0x00, 0xef, 0x2a, 0x31, 0x00, 0x00, 0x00, 0x80, 0x84, 0x1e, 0x00};

const static unsigned char wm_tool_chip_cmd_get_mac[]  = {0x21, 0x06, 0x00, 0xea, 0x2d, 0x38, 0x00, 0x00, 0x00};

static const unsigned int wm_tool_crc32_tab[] = {0x00000000L, 0x77073096L, 0xee0e612cL,
        0x990951baL, 0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
        0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL,
        0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L, 0x1db71064L, 0x6ab020f2L,
        0xf3b97148L, 0x84be41deL, 0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L,
        0x83d385c7L, 0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL,
        0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L,
        0x4c69105eL, 0xd56041e4L, 0xa2677172L, 0x3c03e4d1L, 0x4b04d447L,
        0xd20d85fdL, 0xa50ab56bL, 0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L,
        0xacbcf940L, 0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
        0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L,
        0x56b3c423L, 0xcfba9599L, 0xb8bda50fL, 0x2802b89eL, 0x5f058808L,
        0xc60cd9b2L, 0xb10be924L, 0x2f6f7c87L, 0x58684c11L, 0xc1611dabL,
        0xb6662d3dL, 0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL,
        0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L,
        0x0f00f934L, 0x9609a88eL, 0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL,
        0x91646c97L, 0xe6635c01L, 0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L,
        0xf262004eL, 0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
        0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL,
        0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L, 0x4db26158L, 0x3ab551ceL,
        0xa3bc0074L, 0xd4bb30e2L, 0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL,
        0xd3d6f4fbL, 0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L,
        0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL,
        0x270241aaL, 0xbe0b1010L, 0xc90c2086L, 0x5768b525L, 0x206f85b3L,
        0xb966d409L, 0xce61e49fL, 0x5edef90eL, 0x29d9c998L, 0xb0d09822L,
        0xc7d7a8b4L, 0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
        0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL, 0xead54739L,
        0x9dd277afL, 0x04db2615L, 0x73dc1683L, 0xe3630b12L, 0x94643b84L,
        0x0d6d6a3eL, 0x7a6a5aa8L, 0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L,
        0x7d079eb1L, 0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL,
        0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L, 0xfed41b76L,
        0x89d32be0L, 0x10da7a5aL, 0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L,
        0x17b7be43L, 0x60b08ed5L, 0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L,
        0x4fdff252L, 0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
        0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L, 0xdf60efc3L,
        0xa867df55L, 0x316e8eefL, 0x4669be79L, 0xcb61b38cL, 0xbc66831aL,
        0x256fd2a0L, 0x5268e236L, 0xcc0c7795L, 0xbb0b4703L, 0x220216b9L,
        0x5505262fL, 0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L,
        0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L,
        0xec63f226L, 0x756aa39cL, 0x026d930aL, 0x9c0906a9L, 0xeb0e363fL,
        0x72076785L, 0x05005713L, 0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL,
        0x0cb61b38L, 0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
        0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL,
        0xf6b9265bL, 0x6fb077e1L, 0x18b74777L, 0x88085ae6L, 0xff0f6a70L,
        0x66063bcaL, 0x11010b5cL, 0x8f659effL, 0xf862ae69L, 0x616bffd3L,
        0x166ccf45L, 0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L,
        0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL,
        0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L,
        0x47b2cf7fL, 0x30b5ffe9L, 0xbdbdf21cL, 0xcabac28aL, 0x53b39330L,
        0x24b4a3a6L, 0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
        0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L,
        0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL};

static void wm_tool_signal_proc_entry(void);
static void wm_tool_stdin_to_uart(void);


static int wm_tool_printf(const char *format, ...)
{
    int ret;
    va_list ap;

	va_start(ap, format);
	ret = vprintf(format, ap);
	va_end(ap);

	fflush(stdout);

	return ret;
}

static unsigned long long wm_tool_crc32_reflect(unsigned long long ref, unsigned char ch)
{
	int i;
	unsigned long long value = 0;

	for ( i = 1; i < ( ch + 1 ); i++ )
	{
		if( ref & 1 )
			value |= 1 << ( ch - i );
		ref >>= 1;
	}

	return value;
}

static unsigned int wm_tool_crc32(unsigned int crc, unsigned char *buffer, int size, wm_tool_crc32_reflect_e mode)
{
	int i;
	unsigned char temp;

	for (i = 0; i < size; i++)
	{
		if (mode & WM_TOOL_CRC32_REFLECT_INPUT)
		{
			temp = wm_tool_crc32_reflect(buffer[i], 8);
		}
		else
		{
			temp = buffer[i];
		}
		crc = wm_tool_crc32_tab[(crc ^ temp) & 0xff] ^ (crc >> 8);
	}

	return crc ;
}

static unsigned int wm_tool_get_crc32(unsigned char *buffer, int size, wm_tool_crc32_reflect_e mode)
{
	wm_tool_file_crc = wm_tool_crc32(wm_tool_file_crc, buffer, size, mode);
	if (mode & WM_TOOL_CRC32_REFLECT_OUTPUT)
	{
		wm_tool_file_crc = wm_tool_crc32_reflect(wm_tool_file_crc, 32);
	}
	return wm_tool_file_crc;
}

static unsigned short wm_tool_get_crc16(unsigned char *ptr, unsigned short count)
{
	unsigned short crc, i;

	crc = 0;

	while (count--)
	{
		crc = crc ^ (int) *ptr++ << 8;

		for (i = 0; i < 8; i++)
		{
			if (crc & 0x8000)
			    crc = crc << 1 ^ 0x1021;
			else
			    crc = crc << 1;
		}
	}

	return (crc & 0xFFFF);
}

static int wm_tool_char_to_hex(char ch)
{
	int hex;

	hex = -1;

	if ((ch >= '0') && (ch <= '9'))
	{
	    hex = ch - '0';
	}
	else if ((ch >= 'a') && (ch <= 'f'))
	{
	    hex = ch - 'a' + 0xa;
	}
	else if ((ch >= 'A') && (ch <= 'F'))
	{
	    hex = ch - 'A' + 0xa;
	}

	return hex;
}

static int wm_tool_str_to_hex_array(char *str, int cnt, unsigned char array[])
{
	int hex;
	unsigned char tmp;
	unsigned char *des;

	des = array;

	while (cnt-- > 0)
	{
		hex = wm_tool_char_to_hex(*str++);

		if (hex < 0)
		{
		    return -1;
		}
		else
		{
		    tmp = (hex << 4) & 0xf0;
		}

		hex = wm_tool_char_to_hex(*str++);

		if (hex < 0)
		{
		    return -1;
		}
		else
		{
		    tmp = tmp | (hex & 0x0f);
		}

		*des++ = (unsigned char) tmp;
	}

	return ((*str == 0) ? 0 : -1);
}

static char *wm_tool_strcasestr(const char *str1, const char *str2)
{
  char *cp = (char *) str1;
  char *s1, *s2;

  if (!*str2) return (char *) str1;

  while (*cp) {
    s1 = cp;
    s2 = (char *) str2;

    while (*s1 && *s2 && !(tolower((int)*s1) - tolower((int)*s2))) s1++, s2++;
    if (!*s2) return cp;
    cp++;
  }

  return NULL;
}

static int wm_tool_get_file_size(const char* filename)
{
    FILE *fp = fopen(filename, "r");
    if(!fp) return -1;
    fseek(fp,0L,SEEK_END);
    int size = ftell(fp);
    fclose(fp);
    return size;
}

static char *wm_tool_get_name(const char *name)
{
    static char sz_name[WM_TOOL_PATH_MAX] = {0};
    char *p = (char *)name;
    char *q = (char *)name;

    do
    {
#ifdef __MINGW32__
        p = strchr(p, '\\');
#else
        p = strchr(p, '/');
#endif
        if (p)
        {
            p++;
            q = p;
        }
    } while (p);

    strncpy(sz_name, q, WM_TOOL_PATH_MAX - 1);

#ifdef __MINGW32__
    p = wm_tool_strcasestr(sz_name, ".exe");
    if (p)
        *p = '\0';
#endif

    return sz_name;
}

static void wm_tool_print_usage(const char *name)
{
    wm_tool_printf("Usage: %s [-h] [-v] [-b] [-o] [-sb] [-ct] [-it]"
                   " [-ua] [-ra] [-ih] [-nh] [-un] [-df] [-vs]"
                   " [-l] [-c] [-ws] [-ds] [-rs] [-eo] [-dl] [-sl]"
                   "\r\n"
                   "\r\n"
                   "WinnerMicro firmware packaging "
                   "and programming "
                   "tool\r\n\r\n"
                   "options:\r\n\r\n"
                   "  -h                    , show usage\r\n"
                   "  -v                    , show version\r\n"
                   "\r\n"
                   "  -b  binary            , original binary file\r\n"
                   "  -o  output_name       , output firmware file\r\n"
                   "                          the default is the same as the original binary file name\r\n"
                   "  -sb second_boot       , second boot file, used to generate fls file\r\n"
                   "  -fc compress_type     , whether the firmware is compressed, default is compressed\r\n"
                   "                          <0 | 1> or <uncompress | compress>\r\n"
                   "  -it image_type        , firmware image layout type, default is 0\r\n"
                   "                          <0 | 1>\r\n"
                   "  -ua update_address    , upgrade storage location (hexadecimal)\r\n"
                   "                          the default is 8090000\r\n"
                   "  -ra run_address       , runtime position (hexadecimal)\r\n"
                   "                          the default is 8002400\r\n"
                   "  -ih image_header      , image header storage location (hexadecimal)\r\n"
                   "                          the default is 8002000\r\n"
                   "  -nh next_image_header , next image header storage location (hexadecimal)\r\n"
                   "                          the default is 0\r\n"
                   "  -un upd_no            , upd no version number (hexadecimal)\r\n"
                   "                          the default is 0\r\n"
                   "  -df                   , generate debug firmware for openocd\r\n"
                   "  -vs version_string    , firmware version string, cannot exceed 16 bytes\r\n"
                   "\r\n"
                   "  -l                    , list the local serial port\r\n"
                   "  -c  serial_name       , connect a serial port\r\n"
#if defined(__APPLE__) && defined(__MACH__)
                   "                          e.g: tty.usbserial0 tty.usbserial3 tty.usbserial7\r\n"
#elif defined(__MINGW32__) || defined(__CYGWIN__)
                   "                          e.g: COM0 COM3 COM7\r\n"
#elif defined(__linux__)
                   "                          e.g: ttyUSB0 ttyUSB3 ttyUSB7\r\n"
#endif
                   "  -ws baud_rate         , set the serial port speed during normal work, default is 115200\r\n"
                   "                          <1200 - 2000000> or <1M | 2M>\r\n"
                   "  -ds baud_rate         , set the serial port speed when downloading, default is 115200\r\n"
                   "                          <115200 | 460800 | 921600 | 1000000 | 2000000> or <1M | 2M>\r\n"
                   "  -rs reset_action      , set device reset method, default is manual control\r\n"
                   "                          <none | at | rts>\r\n"
                   "                           none - manual control device reset\r\n"
                   "                           at   - use the at command to control the device reset\r\n"
                   "                           rts  - use the serial port rts pin to control the device reset\r\n"
                   "  -eo erase_option      , firmware area erase option\r\n"
                   "                          <all>\r\n"
                   "                           all  - erase all areas\r\n"
                   "  -dl download_firmware , firmware file to be downloaded, default download compressed image\r\n"
                   "  -sl display_format    , display the log information output from the serial port\r\n"
                   "                          <0 | 1> or <str | hex>\r\n"
                   "                           str - string mode display\r\n"
                   "                           hex - hexadecimal format\r\n"
                   , wm_tool_get_name(name));

    return;
}

static void wm_tool_print_version(const char *name)
{
    wm_tool_printf("%s %s for w800\r\nCopyright (C) 2013 - 2020 WinnerMicro, Inc.\r\n", wm_tool_get_name(name), wm_tool_version);

    return;
}

static int wm_tool_parse_arv(int argc, char *argv[])
{
    int opt;
    int option_index = 0;
    char *opt_string = "hvlc:b:o:";
    int cnt = 0;

    opterr = 1;/* show err info */

    struct option long_options[] = {
       {"dl", required_argument, NULL, 'd'},
       {"ws", required_argument, NULL, 'w'},
       {"ds", required_argument, NULL, 's'},
       {"sb", required_argument, NULL, 'S'},
       {"it", required_argument, NULL, 'i'},
       {"fc", required_argument, NULL, 'C'},
       {"ua", required_argument, NULL, 'u'},
       {"ra", required_argument, NULL, 'r'},
       {"ih", required_argument, NULL, 'H'},
       {"nh", required_argument, NULL, 'n'},
       {"un", required_argument, NULL, 'U'},
       {"df", no_argument,       NULL, 'D'},
       {"vs", required_argument, NULL, 'V'},
       {"rs", required_argument, NULL, 'a'},
       {"eo", required_argument, NULL, 'e'},
       {"sl", required_argument, NULL, 'g'},
       {0,    0,                 NULL, 0}
    };

    while ( (opt = getopt_long_only(argc, argv, opt_string, long_options, &option_index)) != -1)
    {
        WM_TOOL_DBG_PRINT("%c-%s\r\n", opt, optarg);

        switch (opt)
        {
            case '?':
            case 'h':
            {
                wm_tool_show_usage = 1;
                break;
            }
            case 'v':
            {
                wm_tool_show_ver = 1;
                break;
            }
            case 'l':
            {
                wm_tool_list_com = 1;
                break;
            }
            case 'c':
            {
#if defined(__MINGW32__)
                strcpy(wm_tool_serial_path, optarg);
#elif defined(__CYGWIN__)
                sprintf(wm_tool_serial_path, "/dev/ttyS%d", atoi(optarg + strlen("COM")) - 1);
#else
                sprintf(wm_tool_serial_path, "/dev/%s", optarg);
#endif
                break;
            }
            case 'w':
            {
                if ('M' == optarg[1])
                    wm_tool_normal_serial_rate = (optarg[0] - 0x30) * 1000000;
                else
                    wm_tool_normal_serial_rate = strtol(optarg, NULL, 10);
                break;
            }
            case 's':
            {
                if ('M' == optarg[1])
                    wm_tool_download_serial_rate = (optarg[0] - 0x30) * 1000000;
                else
                    wm_tool_download_serial_rate = strtol(optarg, NULL, 10);
                break;
            }
            case 'a':
            {
                if (0 == strncmp(optarg, "none", strlen("none")))
                    wm_tool_dl_action = WM_TOOL_DL_ACTION_NONE;
                else if (0 == strncmp(optarg, "at", strlen("at")))
                    wm_tool_dl_action = WM_TOOL_DL_ACTION_AT;
                else if (0 == strncmp(optarg, "rts", strlen("rts")))
                    wm_tool_dl_action = WM_TOOL_DL_ACTION_RTS;
                else
                    wm_tool_show_usage = 1;
                break;
            }
            case 'e':
            {
                if (0 == strncmp(optarg, "all", strlen("all")))
                    wm_tool_dl_erase = WM_TOOL_DL_ERASE_ALL;
                else
                    wm_tool_show_usage = 1;
                break;
            }
            case 'd':
            {
                wm_tool_download_image = strdup(optarg);
                if (wm_tool_strcasestr(wm_tool_download_image, ".fls"))
                    wm_tool_dl_type = WM_TOOL_DL_TYPE_FLS;
                break;
            }
            case 'o':
            {
                wm_tool_output_image = strdup(optarg);
                break;
            }
            case 'b':
            {
                wm_tool_input_binary = strdup(optarg);
                break;
            }
            case 'S':
            {
                wm_tool_secboot_image = strdup(optarg);
                break;
            }
            case 'i':
            {
#if 0
                if ('0' == optarg[0])
                    wm_tool_image_type = WM_TOOL_LAYOUT_TYPE_1M;
                else if ('3' == optarg[0])
                    wm_tool_image_type = WM_TOOL_LAYOUT_TYPE_2M;
                else if (0 == strncmp(optarg, "1M", strlen("1M")))
                    wm_tool_image_type = WM_TOOL_LAYOUT_TYPE_1M;
                else if (0 == strncmp(optarg, "2M", strlen("2M")))
                    wm_tool_image_type = WM_TOOL_LAYOUT_TYPE_2M;
                else
#endif
                {
                    if (isdigit((int)optarg[0]))
                        wm_tool_image_type = atoi(optarg);//optarg[0] - 0x30;
                    else
                        wm_tool_show_usage = 1;
                }
                break;
            }
            case 'C':
            {
                if ('0' == optarg[0])
                    wm_tool_zip_type = WM_TOOL_ZIP_TYPE_UNCOMPRESS;
                else if ('1' == optarg[0])
                    wm_tool_zip_type = WM_TOOL_ZIP_TYPE_COMPRESS;
                else if (0 == strncmp(optarg, "compress", strlen("compress")))
                    wm_tool_zip_type = WM_TOOL_ZIP_TYPE_COMPRESS;
                else if (0 == strncmp(optarg, "uncompress", strlen("uncompress")))
                    wm_tool_zip_type = WM_TOOL_ZIP_TYPE_UNCOMPRESS;
                else
                    wm_tool_show_usage = 1;
                break;
            }
            case 'u':
            {
                wm_tool_upd_addr = strtol(optarg, NULL, 16);
                break;
            }
            case 'r':
            {
                wm_tool_run_addr = strtol(optarg, NULL, 16);
                break;
            }
            case 'D':
            {
                wm_tool_is_debug = 1;
                break;
            }
            case 'V':
            {
                strncpy(wm_tool_image_version, optarg, WM_TOOL_IMAGE_VERSION_LEN);
                wm_tool_image_version[WM_TOOL_IMAGE_VERSION_LEN - 1] = '\0';
                break;
            }
            case 'g':
            {
                if ('0' == optarg[0])
                    wm_tool_show_log_type = WM_TOOL_SHOW_LOG_STR;
                else if ('1' == optarg[0])
                    wm_tool_show_log_type = WM_TOOL_SHOW_LOG_HEX;
                else if (0 == strncmp(optarg, "str", strlen("str")))
                    wm_tool_show_log_type = WM_TOOL_SHOW_LOG_STR;
                else if (0 == strncmp(optarg, "hex", strlen("hex")))
                    wm_tool_show_log_type = WM_TOOL_SHOW_LOG_HEX;
                else
                    wm_tool_show_usage = 1;
                break;
                break;
            }
            case 'H':
            {
                wm_tool_image_header = strtol(optarg, NULL, 16);
                break;
            }
            case 'n':
            {
                wm_tool_next_image_header = strtol(optarg, NULL, 16);
                break;
            }
            case 'U':
            {
                wm_tool_image_upd_no = strtol(optarg, NULL, 16);
                break;
            }
            default:
            {
                wm_tool_show_usage = 1;
                break;
            }
        }

        cnt++;
    }

    return cnt;
}

static int wm_tool_pack_image(const char *outfile)
{
    FILE *fpbin = NULL;
	FILE *fpimg = NULL;
	int readlen = 0;
	int filelen = 0;
	int patch = 0;
	wm_tool_firmware_booter_t fbooter;
	unsigned char buf[WM_TOOL_ONCE_READ_LEN + 1];

    fpbin = fopen(wm_tool_input_binary, "rb");
	if (NULL == fpbin)
	{
		wm_tool_printf("can not open input file [%s].\r\n", wm_tool_input_binary);
		return -2;
	}

	fpimg = fopen(outfile, "wb+");
	if (NULL == fpimg)
	{
		wm_tool_printf("open img file error: [%s].\r\n", outfile);
		fclose(fpbin);
		return -3;
	}

    /* --------deal with upgrade image's CRC begin---- */
    wm_tool_file_crc = 0xFFFFFFFF;
	while (!feof(fpbin))
	{
		memset(buf, 0, sizeof(buf));
		readlen = fread(buf, 1, WM_TOOL_ONCE_READ_LEN, fpbin);
		if(readlen % 4 != 0)
		{
			patch = 4 - readlen%4;
			readlen += patch;
		}
		filelen += readlen;
		wm_tool_get_crc32((unsigned char*)buf, readlen, 0);
	}
	/* --------deal with upgrade image's CRC end---- */

	wm_tool_src_binary_len = filelen;
	wm_tool_src_binary_crc = wm_tool_file_crc;

    memset(&fbooter, 0, sizeof(wm_tool_firmware_booter_t));
    strcpy((char *)fbooter.ver, wm_tool_image_version);
	fbooter.magic_no = WM_TOOL_IMG_HEAD_MAGIC_NO;

    fbooter.run_org_checksum = wm_tool_file_crc;
	fbooter.img_type         = wm_tool_image_type;
	fbooter.run_img_len      = filelen;
	fbooter.run_img_addr     = wm_tool_run_addr;
	fbooter.zip_type         = WM_TOOL_ZIP_TYPE_UNCOMPRESS;
    fbooter.img_header_addr  = wm_tool_image_header;
    fbooter.upgrade_img_addr = wm_tool_upd_addr;
    fbooter.upd_no           = wm_tool_image_upd_no;
    fbooter.next_boot        = wm_tool_next_image_header;

    /* calculate image's header's CRC */
	wm_tool_file_crc = 0xFFFFFFFF;
	wm_tool_get_crc32((unsigned char *)&fbooter, sizeof(wm_tool_firmware_booter_t) - 4, 0);
    fbooter.hd_checksum = wm_tool_file_crc;

    /* write image's header to output file */
	fwrite(&fbooter, 1, sizeof(wm_tool_firmware_booter_t), fpimg);

	/* write image to output file */
	fseek(fpbin, 0, SEEK_SET);
	while (!feof(fpbin))
	{
		readlen = fread(buf, 1, WM_TOOL_ONCE_READ_LEN, fpbin);
		fwrite(buf, 1, readlen, fpimg);
	}

	/* write dummy data to pad 4byte-aligned */
	if (patch > 0)
	{
		memset(buf, 0, patch);
		fwrite(buf, 1, patch, fpimg);
	}

	if (fpbin)
	{
		fclose(fpbin);
	}

	if (fpimg)
	{
		fclose(fpimg);
	}

    wm_tool_printf("generate normal image completed.\r\n");

    return 0;
}

static int wm_tool_pack_gz_image(const char *gzbin, const char *outfile)
{
    FILE *fpbin = NULL;
	FILE *fpimg = NULL;
	int readlen = 0;
	int filelen = 0;
	int patch = 0;
	wm_tool_firmware_booter_t fbooter;
	unsigned char buf[WM_TOOL_ONCE_READ_LEN + 1];

    fpbin = fopen(gzbin, "rb");
	if (NULL == fpbin)
	{
		wm_tool_printf("can not open input file [%s].\r\n", gzbin);
		return -2;
	}

	fpimg = fopen(outfile, "wb+");
	if (NULL == fpimg)
	{
		wm_tool_printf("create img file error: [%s].\r\n", outfile);
		fclose(fpbin);
		return -3;
	}

    /* --------deal with upgrade image's CRC begin---- */
    wm_tool_file_crc = 0xFFFFFFFF;
	while (!feof(fpbin))
	{
		memset(buf, 0, sizeof(buf));
		readlen = fread(buf, 1, WM_TOOL_ONCE_READ_LEN, fpbin);
		if(readlen % 4 != 0)
		{
			patch = 4 - readlen%4;
			readlen += patch;
		}
		filelen += readlen;
		wm_tool_get_crc32((unsigned char*)buf, readlen, 0);
	}
	/* --------deal with upgrade image's CRC end---- */

    memset(&fbooter, 0, sizeof(wm_tool_firmware_booter_t));
    strcpy((char *)fbooter.ver, wm_tool_image_version);
	fbooter.magic_no = WM_TOOL_IMG_HEAD_MAGIC_NO;

    fbooter.run_org_checksum = wm_tool_file_crc;
	fbooter.img_type         = wm_tool_image_type;
	fbooter.run_img_len      = filelen;
	fbooter.run_img_addr     = wm_tool_run_addr;
	fbooter.zip_type         = wm_tool_zip_type;
	fbooter.img_header_addr  = wm_tool_image_header;
    fbooter.upgrade_img_addr = wm_tool_upd_addr;
    fbooter.upd_no           = wm_tool_image_upd_no;
    fbooter.next_boot        = wm_tool_next_image_header;

    /* calculate image's header's CRC */
	wm_tool_file_crc = 0xFFFFFFFF;
	wm_tool_get_crc32((unsigned char *)&fbooter, sizeof(wm_tool_firmware_booter_t) - 4, 0);
    fbooter.hd_checksum = wm_tool_file_crc;

    /* write image's header to output file */
	fwrite(&fbooter, 1, sizeof(wm_tool_firmware_booter_t), fpimg);

	/* write image to output file */
	fseek(fpbin, 0, SEEK_SET);
	while (!feof(fpbin))
	{
		readlen = fread(buf, 1, WM_TOOL_ONCE_READ_LEN, fpbin);
		fwrite(buf, 1, readlen, fpimg);
	}

	/* write dummy data to pad 4byte-aligned */
	if (patch > 0)
	{
		memset(buf, 0, patch);
		fwrite(buf, 1, patch, fpimg);
	}

	if (fpbin)
	{
		fclose(fpbin);
	}

	if (fpimg)
	{
		fclose(fpimg);
	}

    wm_tool_printf("generate compressed image completed.\r\n");

    return 0;
}

static int wm_tool_pack_dbg_image(const char *image, const char *outfile)
{
    FILE *fpimg = NULL;
	FILE *fout = NULL;
	unsigned char buf[WM_TOOL_ONCE_READ_LEN + 1];
	int readlen = 0;
	int i;
	wm_tool_firmware_booter_t fbooter;
	int appimg_len = 0;
	int final_len = 0;
	int magic_word = 0;

	fpimg = fopen(image, "rb");
	if (NULL == fpimg)
	{
		wm_tool_printf("open img file error: [%s].\r\n", image);
		return -4;
	}

	magic_word = 0;
	readlen = fread(&magic_word, 1, 4, fpimg);
	if (magic_word != WM_TOOL_IMG_HEAD_MAGIC_NO)
	{
		wm_tool_printf("input [%s] file magic error.\n", image);
		fclose(fpimg);
		return -5;
	}

	fout = fopen(outfile, "wb+");
	if (NULL == fout)
	{
		wm_tool_printf("create img file error [%s].\r\n", outfile);
		fclose(fpimg);
		return -6;
	}

	appimg_len = wm_tool_get_file_size(image);

	/* write 0xFF to output file */
	final_len = WM_TOOL_RUN_IMG_HEADER_LEN + (appimg_len - sizeof(wm_tool_firmware_booter_t));
	for (i = 0; i < (final_len /WM_TOOL_ONCE_READ_LEN); i++)
	{
		memset(buf, 0xff, WM_TOOL_ONCE_READ_LEN);
		fwrite(buf, 1, WM_TOOL_ONCE_READ_LEN, fout);
	}
	memset(buf, 0xff, final_len % WM_TOOL_ONCE_READ_LEN);
	fwrite(buf, 1, final_len % WM_TOOL_ONCE_READ_LEN, fout);

	memset(&fbooter, 0, sizeof(wm_tool_firmware_booter_t));

	/* write sec img to output file */
	fseek(fpimg, 0, SEEK_SET);
	readlen = fread((unsigned char *)&fbooter, 1, sizeof(wm_tool_firmware_booter_t), fpimg);

	fseek(fout, 0, SEEK_SET);
	fwrite(&fbooter, 1, sizeof(wm_tool_firmware_booter_t), fout);
	fseek(fout, WM_TOOL_RUN_IMG_HEADER_LEN, SEEK_SET);

	while (!feof(fpimg))
	{
		readlen = fread(buf, 1, WM_TOOL_ONCE_READ_LEN, fpimg);
		fwrite(buf, 1, readlen, fout);
	}

	if (fpimg)
	{
		fclose(fpimg);
	}

	if (fout)
	{
		fclose(fout);
	}

    wm_tool_printf("generate debug image completed.\r\n");

    return 0;
}

static int wm_tool_pack_fls(const char *image, const char *outfile)
{
	FILE *fpsec = NULL;
	FILE *fpimg = NULL;
	FILE *fout = NULL;
	unsigned char buf[WM_TOOL_ONCE_READ_LEN + 1];
	int readlen = 0;
	int magic_word = 0;

	fpsec = fopen(wm_tool_secboot_image,"rb");
	if (NULL == fpsec)
	{
		wm_tool_printf("can not open input file [%s].\r\n", wm_tool_secboot_image);
		return -2;
	}

	magic_word = 0;
	readlen = fread(&magic_word, 1, 4, fpsec);
	if (magic_word != WM_TOOL_IMG_HEAD_MAGIC_NO)
	{
		wm_tool_printf("input [%s] file magic error.\r\n", wm_tool_secboot_image);
		fclose(fpsec);
		return -3;
	}

	fpimg = fopen(image, "rb");
	if (NULL == fpimg)
	{
		wm_tool_printf("open img file error [%s].\r\n", image);
		fclose(fpsec);
		return -4;
	}

	magic_word = 0;
	readlen = fread(&magic_word, 1, 4, fpimg);
	if (magic_word != WM_TOOL_IMG_HEAD_MAGIC_NO)
	{
		wm_tool_printf("input [%s] file magic error.\r\n", image);
		fclose(fpsec);
		fclose(fpimg);
		return -5;
	}

	fout = fopen(outfile, "wb+");
	if (NULL == fout)
	{
		wm_tool_printf("create img file error [%s].\r\n", outfile);
		fclose(fpsec);
		fclose(fpimg);
		return -6;
	}

	fseek(fpsec, 0, SEEK_SET);

    while (!feof(fpsec))
	{
		readlen = fread(buf, 1, WM_TOOL_ONCE_READ_LEN, fpsec);
		fwrite(buf, 1, readlen, fout);
	}

    fseek(fpimg, 0, SEEK_SET);

    while (!feof(fpimg))
	{
		readlen = fread(buf, 1, WM_TOOL_ONCE_READ_LEN, fpimg);
		fwrite(buf, 1, readlen, fout);
	}

	if (fpsec)
	{
		fclose(fpsec);
	}

	if (fpimg)
	{
		fclose(fpimg);
	}

	if (fout)
	{
		fclose(fout);
	}

    wm_tool_printf("generate flash file completed.\r\n");

	return 0;
}

static int wm_tool_gzip_bin(const char *binary, const char *gzbin)
{
#if 1
	FILE  *in;
	gzFile out;
    local char buf[BUFLEN];
    int len;

	in = fopen(binary, "rb");
	if (in == NULL) {
		perror(binary);
		exit(1);
	}
	out = gzopen(gzbin, "wb9");
	if (out == NULL) {
		printf("can't gzopen %s\n", gzbin);
		exit(1);
	}

    for (;;) {
        len = (int)fread(buf, 1, sizeof(buf), in);
        if (len < 0) {
            printf("file read err\r\n");
            exit(1);
        }
        if (len == 0) break;

        if (gzwrite(out, buf, (unsigned)len) != len) 
		{
			printf("gzip write wrong\r\n");
		}
    }
    fclose(in);
    if (gzclose(out) != Z_OK) 
	{	
		printf("failed gzclose");
	}
#else
    FILE *bfp;
    gzFile gzfp;
    int ret;
    char buf[Z_BUFSIZE];

    bfp = fopen(binary, "rb");
    gzfp = gzopen((char *)gzbin, "wb+");

    if (!bfp || !gzfp)
    {
        wm_tool_printf("can not gzip binary.\r\n");
        return -1;
    }

    do {
        ret = fread(buf, 1, sizeof(buf), bfp);
		if (ret == 0)
		{	
			printf("@@@@@@@@@@@@@ret:%d\n", ret);
			break;
		}
        if (ret > 0)
        {
            ret = gzwrite(gzfp, (voidp)buf, ret);
            if (ret <= 0)
            {
                wm_tool_printf("can not write gzip binary.\r\n");
                return -2;
            }
        }
    } while (ret > 0);
	printf("====================================\n");
    gzclose(gzfp);
    fclose(bfp);
#endif
    wm_tool_printf("compress binary completed.\r\n");

    return 0;
}

static int wm_tool_pack_firmware(void)
{
    int ret;
    char *path;
    char *name;
    char *image;
    char *gzbin;
    char *gzimg;
    char *dbgimg;
    char *fls;

    if (!wm_tool_input_binary)
        return 1;

    if (!wm_tool_output_image)
    {
        char *r = wm_tool_input_binary;
        char *t = wm_tool_input_binary;
        do
        {
            r = strchr(r, '.');
            if (r)
            {
                t = r;
                r++;
            }
        } while (r);
        wm_tool_output_image = malloc(t - wm_tool_input_binary + 1);
        if (!wm_tool_output_image)
            return -1;
        memcpy(wm_tool_output_image, wm_tool_input_binary, t - wm_tool_input_binary);
        wm_tool_output_image[t - wm_tool_input_binary] = '\0';
    }

    char *p = wm_tool_output_image;
    char *q = wm_tool_output_image;

    do
    {
        p = strchr(p, '/');
        if (p)
        {
            p++;
            q = p;
        }
    } while (p);

    /* output file */
    name = strdup(q);
    *q = '\0';
    path = strdup(wm_tool_output_image);

    image = malloc(strlen(path) + strlen(name) + strlen(".img") + 1);
    if (!image)
        return -1;
    sprintf(image, "%s%s.img", path, name);
	if (WM_TOOL_ZIP_TYPE_UNCOMPRESS == wm_tool_zip_type)
	{
        ret = wm_tool_pack_image(image);
        if (ret)
            return ret;
	}

    if (WM_TOOL_ZIP_TYPE_COMPRESS == wm_tool_zip_type)
    {
        gzbin = malloc(strlen(path) + strlen(name) + strlen("_gz.bin") + 1);
        gzimg = malloc(strlen(path) + strlen(name) + strlen("_gz.img") + 1);
        if (!gzbin || !gzimg)
            return -1;
        sprintf(gzbin, "%s%s.bin.gz", path, name);
        sprintf(gzimg, "%s%s_gz.img", path, name);

        ret = wm_tool_gzip_bin(wm_tool_input_binary, gzbin);
        if (ret)
            return ret;

        ret = wm_tool_pack_gz_image(gzbin, gzimg);
        free(gzbin);
        free(gzimg);
        if (ret)
            return ret;
    }

    if (wm_tool_is_debug)
    {
        dbgimg = malloc(strlen(path) + strlen(name) + strlen("_dbg.img") + 1);
        if (!dbgimg)
            return -1;
        sprintf(dbgimg, "%s%s_dbg.img", path, name);

        ret = wm_tool_pack_dbg_image(image, dbgimg);
        free(dbgimg);
        if (ret)
            return ret;
    }

    if (wm_tool_secboot_image)
    {
        fls = malloc(strlen(path) + strlen(name) + strlen(".fls") + 1);
        if (!fls)
            return -1;
        sprintf(fls, "%s%s.fls", path, name);

        ret = wm_tool_pack_fls(image, fls);

        free(fls);
    }

    free(image);

    return ret;
}

#ifdef __MINGW32__
static BOOL wm_tool_signal_proc(DWORD no)
{
	switch (no)
	{
    	case CTRL_C_EVENT:
    		wm_tool_signal_proc_entry();
    		return(FALSE);
    	default:
    		return FALSE;
	}
}

static void wm_tool_signal_init(void)
{
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)wm_tool_signal_proc, TRUE);
}

static void wm_tool_delay_ms(int ms)
{
    Sleep(ms);

    return;
}

static int wm_tool_uart_set_rts(int boolflag)
{
    return EscapeCommFunction(wm_tool_uart_handle,((boolflag) ? SETRTS : CLRRTS)) ? 0 : -1;
}

static int wm_tool_uart_set_dtr(int boolflag)
{
    return EscapeCommFunction(wm_tool_uart_handle,((boolflag) ? SETDTR : CLRDTR)) ? 0 : -1;
}

static int wm_tool_uart_set_timeout(void)
{
    BOOL ret;
    COMMTIMEOUTS timeout;

	timeout.ReadIntervalTimeout         = MAXDWORD;
	timeout.ReadTotalTimeoutConstant    = 0;
	timeout.ReadTotalTimeoutMultiplier  = 0;
	timeout.WriteTotalTimeoutConstant   = 0;
	timeout.WriteTotalTimeoutMultiplier = 0;

	ret = SetCommTimeouts(wm_tool_uart_handle, &timeout);
	if (ret)
	    ret = SetCommMask(wm_tool_uart_handle, EV_TXEMPTY);

    return ret ? 0 : -1;
}

static void wm_tool_uart_set_block(int bolck)
{
    if (bolck)
        wm_tool_uart_block = INFINITE;
    else
        wm_tool_uart_block = 0;

    return;
}

static int wm_tool_uart_set_speed(int speed)
{
    int i;
    int size;
	DCB cfg;

	size = sizeof(wm_tool_uart_speed_array) / sizeof(int);

    for (i= 0; i < size; i++)
    {
        if (speed == wm_tool_uart_name_array[i])
        {
            if (GetCommState(wm_tool_uart_handle, &cfg))
            {
            	cfg.DCBlength		= sizeof(DCB);
            	cfg.BaudRate		= wm_tool_uart_speed_array[i];
            	cfg.fBinary		    = TRUE;
            	cfg.fParity         = FALSE;
            	cfg.fDtrControl	    = DTR_CONTROL_DISABLE;
            	cfg.fDsrSensitivity = FALSE;
            	cfg.fRtsControl	    = RTS_CONTROL_DISABLE;
            	cfg.ByteSize		= 8;
            	cfg.StopBits 		= ONESTOPBIT;
            	cfg.fAbortOnError	= FALSE;
            	cfg.fOutX			= FALSE;
            	cfg.fInX			= FALSE;
            	cfg.fErrorChar      = FALSE;
            	cfg.fNull           = FALSE;
            	cfg.fOutxCtsFlow    = FALSE;
	            cfg.fOutxDsrFlow    = FALSE;
	            cfg.Parity          = NOPARITY;
	            cfg.fTXContinueOnXoff = FALSE;

            	return SetCommState(wm_tool_uart_handle, &cfg) ? 0 : -1;
        	}
        	else
        	{
                return -3;
        	}
        }
    }

	return -2;
}

static void wm_tool_uart_clear(void)
{
    PurgeComm(wm_tool_uart_handle, PURGE_RXCLEAR);

    return;
}

static int wm_tool_uart_open(const char *device)
{
    BOOL ret;
    char name[40];

	sprintf(name,"\\\\.\\%s", device);

	wm_tool_uart_handle = CreateFile(name, GENERIC_WRITE | GENERIC_READ,
                                     0, NULL, OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                     NULL);

	if (wm_tool_uart_handle == INVALID_HANDLE_VALUE)
	{
		return -1;
	}

    ret = SetupComm(wm_tool_uart_handle, 4096, 4096);
    if (ret)
    {
        ret  = wm_tool_uart_set_speed(WM_TOOL_DEFAULT_BAUD_RATE);
        ret |= wm_tool_uart_set_timeout();
    }
    else
    {
        ret = -1;
    }

    return ret;
}

static int wm_tool_uart_read(void *data, unsigned int size)
{
    BOOL ret;
    DWORD wait;
	unsigned long len = 0;
	COMSTAT state;
	DWORD error;
    OVERLAPPED event;

    do
    {
    	memset(&event, 0, sizeof(OVERLAPPED));

    	event.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    	ClearCommError(wm_tool_uart_handle, &error, &state);

        ret = ReadFile(wm_tool_uart_handle, data, size, &len, &event);
        if (!ret)
        {
            if (ERROR_IO_PENDING == GetLastError())
    		{
    			wait = WaitForSingleObject(event.hEvent, wm_tool_uart_block);
    			if (WAIT_OBJECT_0 == wait)
    			    ret = TRUE;
    		}
        }
    } while (wm_tool_uart_block && !len);

	if (ret)
	{
        if (len > 0)
            return (int)len;
	}

	return -1;
}

static int wm_tool_uart_write(const void *data, unsigned int size)
{
    BOOL ret;
    DWORD wait;
    OVERLAPPED event;
    COMSTAT state;
	DWORD error;
    unsigned int snd = 0;
	unsigned long len = 0;

    memset(&event, 0, sizeof(OVERLAPPED));

    event.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    do
    {
        ClearCommError(wm_tool_uart_handle, &error, &state);

        ret = WriteFile(wm_tool_uart_handle, data + snd, size - snd, &len, &event);
        if (!ret)
        {
            if (ERROR_IO_PENDING == GetLastError())
    		{
    			wait = WaitForSingleObject(event.hEvent, INFINITE);
    			if (WAIT_OBJECT_0 == wait)
			        ret = TRUE;
    		}
        }

        if (ret)
    	{
            if ((len > 0) && (snd != size))
            {
                snd += len;
            }
            else
            {
    	        return size;
    	    }
    	}
    	else
    	{
            break;
    	}
    } while (1);

	return -1;
}

static void wm_tool_uart_close(void)
{
	FlushFileBuffers(wm_tool_uart_handle);
	CloseHandle(wm_tool_uart_handle);
	wm_tool_uart_handle = NULL;

	return;
}

static DWORD WINAPI wm_tool_uart_tx_thread(LPVOID arg)
{
    wm_tool_stdin_to_uart();

    return 0;
}

static int wm_tool_create_thread(void)
{
    HANDLE thread_handle;
    thread_handle = CreateThread(0, 0, wm_tool_uart_tx_thread, NULL, 0, NULL);
    CloseHandle(thread_handle);
    return 0;
}

#else

static void wm_tool_signal_proc(int no)
{
    wm_tool_signal_proc_entry();
}

static void wm_tool_signal_init(void)
{
    signal(SIGINT, wm_tool_signal_proc);
}

static void wm_tool_delay_ms(int ms)
{
    usleep(ms * 1000);

    return;
}

static int wm_tool_uart_set_rts(int boolflag)
{
    int ret;
    int controlbits;

    ret = ioctl(wm_tool_uart_fd, TIOCMGET, &controlbits);
    if (ret < 0)
    {
        WM_TOOL_DBG_PRINT("TIOCMGET failed!\r\n");
        return ret;
    }

    if (controlbits & TIOCM_RTS)
    {
        WM_TOOL_DBG_PRINT("TIOCM_RTS!, %d!\r\n", boolflag);
    }
    else
    {
        WM_TOOL_DBG_PRINT("~TIOCM_RTS!, %d!\r\n", boolflag);
    }

    if (boolflag)
        controlbits |= TIOCM_RTS;
    else
        controlbits &= ~TIOCM_RTS;

    ret = ioctl(wm_tool_uart_fd, TIOCMSET, &controlbits);
    if (ret < 0)
    {
        WM_TOOL_DBG_PRINT("TIOCMSET failed!\r\n");
    }

    if (ret >= 0)
        ret = 0;

    return ret;
}

static int wm_tool_uart_set_dtr(int boolflag)
{
    int ret;
    int controlbits;

    ret = ioctl(wm_tool_uart_fd, TIOCMGET, &controlbits);
    if (ret < 0)
    {
        WM_TOOL_DBG_PRINT("TIOCMGET failed!\r\n");
        return ret;
    }

    if (controlbits & TIOCM_DTR)
    {
        WM_TOOL_DBG_PRINT("TIOCM_DTR, %d!\r\n", boolflag);
    }
    else
    {
        WM_TOOL_DBG_PRINT("~TIOCM_DTR, %d!\r\n", boolflag);
    }

    if (boolflag)
        controlbits |= TIOCM_DTR;
    else
        controlbits &= ~TIOCM_DTR;

    ret = ioctl(wm_tool_uart_fd, TIOCMSET, &controlbits);
    if (ret < 0)
    {
        WM_TOOL_DBG_PRINT("TIOCMSET failed!\r\n");
    }

    if (ret >= 0)
        ret = 0;

    return ret;
}

static void wm_tool_uart_set_block(int bolck)
{
    int comopt;

    comopt = fcntl(wm_tool_uart_fd, F_GETFL, 0);

    if (bolck)
        fcntl(wm_tool_uart_fd, F_SETFL, comopt & ~O_NDELAY);
    else
        fcntl(wm_tool_uart_fd, F_SETFL, comopt | O_NDELAY);

    return;
}

static int wm_tool_uart_set_speed(int speed)
{

    int i;
    int size;
    int status = -1;
    struct termios opt;

    tcgetattr(wm_tool_uart_fd, &opt);

    size = sizeof(wm_tool_uart_speed_array) / sizeof(int);

    for (i= 0; i < size; i++)
    {
        if (speed == wm_tool_uart_name_array[i])
        {
            cfsetispeed(&opt, wm_tool_uart_speed_array[i]);
            cfsetospeed(&opt, wm_tool_uart_speed_array[i]);

            status = tcsetattr(wm_tool_uart_fd, TCSANOW, &opt);

            break;
        }
    }

    return status;
}

static void wm_tool_uart_clear(void)
{
    tcflush(wm_tool_uart_fd, TCIFLUSH);

    return;
}

static int wm_tool_uart_open(const char *device)
{
    struct termios tty;

    wm_tool_uart_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (-1 == wm_tool_uart_fd)
    {
		return -1;
    }

    tcgetattr(wm_tool_uart_fd, &wm_tool_saved_serial_cfg);

    wm_tool_uart_set_dtr(0);

    tcgetattr(wm_tool_uart_fd, &tty);

    /* databit 8bit */
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;

    /* set into raw, no echo mode */
    tty.c_iflag =  IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 5;

    /* have no software flow control */
    tty.c_iflag &= ~(IXON|IXOFF|IXANY);

    /* parity none */
    tty.c_cflag &= ~(PARENB | PARODD);

    /* stopbit 1bit */
    tty.c_cflag &= ~CSTOPB;

    tcsetattr(wm_tool_uart_fd, TCSANOW, &tty);

    return wm_tool_uart_set_speed(WM_TOOL_DEFAULT_BAUD_RATE);
}

static int wm_tool_uart_read(void *data, unsigned int size)
{
	return read(wm_tool_uart_fd, data, size);
}

static int wm_tool_uart_write(const void *data, unsigned int size)
{
    int snd = 0;
    int ret = 0;

    do
    {
        ret = write(wm_tool_uart_fd, data + snd, size - snd);
        if (ret > 0)
        {
            snd += ret;

            if (snd == size)
            {
                return size;
            }
        }
        else
        {
            break;
        }
    } while (1);

	return -1;
}

static void wm_tool_uart_close(void)
{
    tcsetattr(wm_tool_uart_fd, TCSANOW, &wm_tool_saved_serial_cfg);

    tcdrain(wm_tool_uart_fd);
    tcflush(wm_tool_uart_fd, TCIOFLUSH);

	close(wm_tool_uart_fd);
    wm_tool_uart_fd = -1;

	return;
}

static void *wm_tool_uart_tx_thread(void *arg)
{
    wm_tool_stdin_to_uart();

    return NULL;
}

static int wm_tool_create_thread(void)
{
    pthread_t thread_id;
    return pthread_create(&thread_id, NULL, wm_tool_uart_tx_thread, NULL);
}
#endif

static void wm_tool_signal_proc_entry(void)
{
    WM_TOOL_DBG_PRINT("signal exit.\r\n");
    exit(0);
}

static void wm_tool_stdin_to_uart(void)
{
    int ret;
    char buf[WM_TOOL_ONCE_READ_LEN];

    while (1)
    {
        if (fgets(buf, WM_TOOL_ONCE_READ_LEN, stdin))
        {
            ret = wm_tool_uart_write(buf, strlen(buf));
            if (ret <= 0)
            {
                WM_TOOL_DBG_PRINT("failed to write [%s].\r\n", buf);
            }
        }
    }

    return;
}

static int wm_tool_show_log_from_serial(void)
{
    int i;
    int ret;
    unsigned int j = 0;
    unsigned char buf[WM_TOOL_ONCE_READ_LEN + 1];

    ret = wm_tool_uart_open(wm_tool_serial_path);
    if (ret)
    {
        wm_tool_printf("can not open serial\r\n");
        return ret;
    }

    if (WM_TOOL_DEFAULT_BAUD_RATE != wm_tool_normal_serial_rate)
    {
        ret = wm_tool_uart_set_speed(wm_tool_normal_serial_rate);
        if (ret)
        {
            wm_tool_printf("can not set serial baud rate.\r\n");
            wm_tool_uart_close();
            return ret;
        }
    }

    ret = wm_tool_create_thread();
    if (ret)
    {
        wm_tool_printf("can not create thread.\r\n");
        wm_tool_uart_close();
        return ret;
    }

    wm_tool_uart_set_block(0);

    wm_tool_signal_init();

    while (1)
    {
        ret = wm_tool_uart_read(buf, WM_TOOL_ONCE_READ_LEN);
        if (ret > 0)
        {
            if (WM_TOOL_SHOW_LOG_STR == wm_tool_show_log_type)
            {
                buf[ret] = '\0';
                wm_tool_printf("%s", (char *)buf);
            }
            else if (WM_TOOL_SHOW_LOG_HEX == wm_tool_show_log_type)
            {
                for (i = 0; i < ret; i++, j++)
                {
                    wm_tool_printf("%02X ", buf[i]);
                    if ((j + 1) % 16 == 0)
                    {
                        wm_tool_printf("\r\n");
                    }
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            wm_tool_delay_ms(1);
        }
    }

    wm_tool_uart_close();

    return ret;
}

static int wm_tool_set_wifi_chip_speed(int speed)
{
	int ret;

	if (2000000 == speed)
	{
		ret = wm_tool_uart_write(wm_tool_chip_cmd_b2000000, sizeof(wm_tool_chip_cmd_b2000000));
	}
	else if (1000000 == speed)
	{
		ret = wm_tool_uart_write(wm_tool_chip_cmd_b1000000, sizeof(wm_tool_chip_cmd_b1000000));
	}
	else if (921600 == speed)
	{
		ret = wm_tool_uart_write(wm_tool_chip_cmd_b921600, sizeof(wm_tool_chip_cmd_b921600));
	}
	else if (460800 == speed)
	{
		ret = wm_tool_uart_write(wm_tool_chip_cmd_b460800, sizeof(wm_tool_chip_cmd_b460800));
	}
	else
	{
		ret = wm_tool_uart_write(wm_tool_chip_cmd_b115200, sizeof(wm_tool_chip_cmd_b115200));
	}

	return ret;
}

static int wm_tool_send_esc2uart(int ms)
{
    int i;
	int err = 0;
    unsigned char esc_key = 27;

    for (i = 0; i < (ms / 10); i++)
	{
        err = wm_tool_uart_write(&esc_key, 1);
		wm_tool_delay_ms(10);/* 10-50ms */
	}

    return err;
}

static int wm_tool_erase_image(wm_tool_dl_erase_e type)
{
    int cnt = 0;
    int ret = -1;
    unsigned char ch;
    unsigned char rom_cmd[] = {0x21, 0x0a, 0x00, 0xc3, 0x35, 0x32, 0x00, 0x00, 0x00, 0x02, 0x00, 0xfe, 0x01}; /*2M-8k*/

    WM_TOOL_DBG_PRINT("start erase.\r\n");

    if (WM_TOOL_DL_ERASE_ALL == type)
    {
        wm_tool_printf("erase flash...\r\n");
        ret = wm_tool_uart_write(rom_cmd, sizeof(rom_cmd));
    }
    if (ret <= 0)
        return -1;

    wm_tool_uart_clear();

    wm_tool_uart_set_block(1);

    do
	{
		ret = wm_tool_uart_read(&ch, 1);
		if (ret > 0)
	    {
		    if (('C' == ch) || ('P' == ch))
		        cnt++;
		    else
		        cnt = 0;
		}
		else
		{
		    wm_tool_printf("erase error, errno = %d.\r\n", errno);
            return -2;
		}
	} while (cnt < 3);

    wm_tool_uart_set_block(0);

    wm_tool_printf("erase finish.\r\n");

    return 0;
}

static int wm_tool_query_mac(void)
{
    int ret = -1;
    int err;
    int offset = 0;
    char macstr[32] = {0};
    int len = strlen("MAC:AABBCCDDEEFF\n");/* resp format, ROM "Mac:AABBCCDDEEFF\n", SECBOOT "MAC:AABBCCDDEEFF\n" */
    unsigned char macaddr[6] = {0};

    wm_tool_uart_clear();

    wm_tool_uart_set_block(1);

	err = wm_tool_uart_write(wm_tool_chip_cmd_get_mac, sizeof(wm_tool_chip_cmd_get_mac));
    if (err > 0)
    {
        do
        {
            err = wm_tool_uart_read((unsigned char *)(macstr + offset), sizeof(macstr) - 1 - offset);
            if (err > 0)
            {
                offset += err;
                if (offset >= len)
                {
                    macstr[len - 1] = '\0';/* \n -> 0 */
                    err = wm_tool_str_to_hex_array(macstr + strlen("MAC:"), 6, macaddr);
                    if (!err)
                    {
                        wm_tool_printf("mac %02X-%02X-%02X-%02X-%02X-%02X.\r\n", macaddr[0],
                                       macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);

                        if (!strncmp(macstr, "Mac:", strlen("Mac:")) && 
                            (WM_TOOL_DL_TYPE_FLS != wm_tool_dl_type) && 
                            !wm_tool_dl_erase)
                        {
                            wm_tool_printf("please download the firmware in .fls format.\r\n");
                        }
                        else
                        {
                            ret = 0;
                        }
                    }
                    break;
                }
            }
        } while (err > 0);
    }

    wm_tool_uart_set_block(0);

    return ret;
}

static int wm_tool_xmodem_download(const char *image)
{
    FILE *imgfp;
	unsigned char packet_data[XMODEM_DATA_SIZE];
	unsigned char frame_data[XMODEM_DATA_SIZE + XMODEM_CRC_SIZE + XMODEM_FRAME_ID_SIZE + 1];
	int complete, retry_num, pack_counter, read_number, write_number = 0, i;
	unsigned short crc_value;
	unsigned char ack_id;
	int sndlen;
	int ret = -111;

	imgfp = fopen(image, "rb");
    if (!imgfp)
    {
        wm_tool_printf("can not open image to download.\r\n");
		return -1;
    }

	sndlen = 0;
	pack_counter = 0;
	complete = 0;
	retry_num = 0;

	wm_tool_printf("start download.\r\n");
	wm_tool_printf("0%% [");

    wm_tool_uart_clear();

    wm_tool_uart_set_block(1);

	ack_id = XMODEM_ACK;

	while (!complete)
	{
	    WM_TOOL_DBG_PRINT("switch ack_id = %x\r\n", ack_id);

        switch(ack_id)
        {
        	case XMODEM_ACK:
        	{
	            retry_num = 0;
	            pack_counter++;
	            read_number = fread(packet_data, sizeof(char), XMODEM_DATA_SIZE, imgfp);

	            WM_TOOL_DBG_PRINT("fread = %d\r\n", read_number);

	            if (read_number > 0)
	            {
					sndlen += read_number;

					if (read_number < XMODEM_DATA_SIZE)
	                {
	                    WM_TOOL_DBG_PRINT("start filling the last frame!\r\n");
	                    for ( ; read_number < XMODEM_DATA_SIZE; read_number++)
	                        packet_data[read_number] = 0x0;
	                }

	                frame_data[0] = XMODEM_HEAD;
	                frame_data[1] = (char)pack_counter;
	                frame_data[2] = (char)(255 - frame_data[1]);

	                for (i = 0; i < XMODEM_DATA_SIZE; i++)
	                    frame_data[i + 3] = packet_data[i];

	                crc_value = wm_tool_get_crc16(packet_data, XMODEM_DATA_SIZE);

					frame_data[XMODEM_DATA_SIZE + 3]=(unsigned char)(crc_value >> 8);
	                frame_data[XMODEM_DATA_SIZE + 4]=(unsigned char)(crc_value);

	                write_number = wm_tool_uart_write(frame_data, XMODEM_DATA_SIZE + 5);
	                if (write_number <= 0)
	                    wm_tool_printf("write serial error, errno = %d.\r\n", errno);

	                WM_TOOL_DBG_PRINT("waiting for ack, %d, %d ...\r\n", pack_counter, write_number);

	                while ((wm_tool_uart_read(&ack_id, 1)) <= 0);

	                if (ack_id == XMODEM_ACK)
	                {
	                	WM_TOOL_DBG_PRINT("Ok!\r\n");

	                	if (sndlen % 10240 == 0)
                        {
    						wm_tool_printf("#");
						}
	                }
					else
					{
                    	WM_TOOL_DBG_PRINT("error = %x!\r\n", ack_id);
					}
	            }
	            else
	            {
	                ack_id = XMODEM_EOT;
	                complete = 1;

	                WM_TOOL_DBG_PRINT("waiting for complete ack ...\r\n");

	                while (ack_id != XMODEM_ACK)
	                {
	                    ack_id = XMODEM_EOT;

	                    write_number = wm_tool_uart_write(&ack_id, 1);
	                    if (write_number <= 0)
	                        wm_tool_printf("write serial error, errno = %d.\r\n", errno);

	                    while ((wm_tool_uart_read(&ack_id, 1)) <= 0);
	                }

	                WM_TOOL_DBG_PRINT("ok\r\n");

	                wm_tool_printf("] 100%%\r\n");

	                wm_tool_printf("download completed.\r\n");

	                ret = 0;
	            }
	            break;
            }
            case XMODEM_NAK:
            {
                if ( retry_num++ > 100)
                {
                    WM_TOOL_DBG_PRINT("retry too many times, quit!\r\n");
                    wm_tool_printf("download firmware timeout.\r\n");

                    complete = 1;
                }
                else
                {
                    write_number = wm_tool_uart_write(frame_data, XMODEM_DATA_SIZE + 5);
                    if (write_number <= 0)
	                    wm_tool_printf("write serial error, errno = %d.\r\n", errno);

                    WM_TOOL_DBG_PRINT("retry for ack, %d, %d ...\r\n", pack_counter, write_number);

                    while ((wm_tool_uart_read(&ack_id, 1)) <= 0);

                    if (ack_id == XMODEM_ACK)
                    {
                    	WM_TOOL_DBG_PRINT("ok\r\n");
                    }
    				else
    				{
                    	WM_TOOL_DBG_PRINT("error!\r\n");
    				}
                }
                break;
            }
            default:
            {
                WM_TOOL_DBG_PRINT("fatal error!\r\n");
                WM_TOOL_DBG_PRINT("unknown xmodem protocal [%x].\r\n", ack_id);
                wm_tool_printf("\r\ndownload failed, please reset and try again.\r\n");

                complete = 1;
                break;
            }
        }
    }

    wm_tool_uart_set_block(0);

    fclose(imgfp);

    return ret;
}
static int wm_tool_download_firmware(void)
{
    int ret;
    int cnt = 0;
    int note = 1;
    int timeout = 0;
    float timeuse;
    time_t start, end;
    unsigned char ch;

    wm_tool_printf("connecting serial...\r\n");

    ret = wm_tool_uart_open(wm_tool_serial_path);
    if (ret)
    {
        wm_tool_printf("can not open serial\r\n");
        return ret;
    }

    /* In some cases, setting the serial port initialization setting requires a delay. */
    wm_tool_delay_ms(500);

    wm_tool_printf("serial connected.\r\n");

    if (WM_TOOL_DL_ACTION_AT == wm_tool_dl_action)
    {
        if (WM_TOOL_DEFAULT_BAUD_RATE != wm_tool_normal_serial_rate)
            wm_tool_uart_set_speed(wm_tool_normal_serial_rate);

#if 0 /* use erase option */
        if (WM_TOOL_DL_TYPE_FLS == wm_tool_dl_type)
        {
            ret = wm_tool_uart_write("AT+&FLSW=8002000,0\r\n", strlen("AT+&FLSW=8002000,0\r\n"));
            if (ret <= 0)
            {
                wm_tool_printf("destroy secboot failed.\r\n");
                wm_tool_uart_close();
                return -3;
            }
            wm_tool_delay_ms(300);
        }
#endif

        ret = wm_tool_uart_write("AT+Z\r\n", strlen("AT+Z\r\n"));
        if (ret <= 0)
        {
            wm_tool_printf("reset error.\r\n");
            wm_tool_uart_close();
            return -4;
        }

        if (WM_TOOL_DEFAULT_BAUD_RATE != wm_tool_normal_serial_rate)
            wm_tool_uart_set_speed(WM_TOOL_DEFAULT_BAUD_RATE);
    }
    else if (WM_TOOL_DL_ACTION_RTS == wm_tool_dl_action)
    {
        ret  = wm_tool_uart_set_dtr(0);
        ret |= wm_tool_uart_set_rts(1);
        wm_tool_delay_ms(50);
        ret |= wm_tool_uart_set_dtr(1);
        ret |= wm_tool_uart_set_rts(0);
        wm_tool_delay_ms(50);
		ret |= wm_tool_uart_set_dtr(0);
		if (ret < 0)
        {
            wm_tool_printf("set rts to reboot error.\r\n");
            wm_tool_uart_close();
            return -5;
        }
    }

    wm_tool_printf("wait serial sync...");

    wm_tool_send_esc2uart(500);/* used for delay */

    start = time(NULL);

    do
	{
		ret = wm_tool_uart_read(&ch, 1);

		WM_TOOL_DBG_PRINT("ret=%d, %x-%c\r\n", ret, ch, ch);

		if (ret > 0)
	    {
		    if (('C' == ch) || ('P' == ch))
		        cnt++;
		    else
		        cnt = 0;
		}
		else
		{
            wm_tool_send_esc2uart(30);
		}

		end = time(NULL);
		timeuse = difftime(end, start);
		if (timeuse >= 1)
		{
		    wm_tool_printf(".");

            timeout++;
            if ((timeout >= (WM_TOOL_DOWNLOAD_TIMEOUT_SEC / 10)) && note)
            {
                wm_tool_printf("\r\nplease manually reset the device.\r\n");
                note = 0;
            }
            else if (timeout > WM_TOOL_DOWNLOAD_TIMEOUT_SEC)
            {
                wm_tool_uart_close();
                wm_tool_printf("\r\nserial sync timeout.\r\n");
        		return -6;
            }

		    start = time(NULL);
		}
	} while (cnt < 3);

	wm_tool_printf("\r\nserial sync sucess.\r\n");

	ret = wm_tool_query_mac();
	if (ret)
    {
        wm_tool_uart_close();
        wm_tool_printf("download failed.\r\n");
        return ret;
    }

	if (WM_TOOL_DL_ERASE_ALL == wm_tool_dl_erase)
	{
        ret = wm_tool_erase_image(WM_TOOL_DL_ERASE_ALL);
        if (ret)
        {
            wm_tool_uart_close();
            wm_tool_printf("failed to erase.\r\n");
            return ret;
        }
	}

    if (!wm_tool_download_image && wm_tool_dl_erase)
    {
        return 0;
    }

    if (WM_TOOL_DEFAULT_BAUD_RATE != wm_tool_download_serial_rate)
    {
        ret = wm_tool_set_wifi_chip_speed(wm_tool_download_serial_rate);
    	if (ret > 0)
    	{
    		wm_tool_delay_ms(1 * 1000);
    		wm_tool_uart_set_speed(wm_tool_download_serial_rate);
    	}
	}

    ret = wm_tool_xmodem_download(wm_tool_download_image);

    if (WM_TOOL_DEFAULT_BAUD_RATE != wm_tool_download_serial_rate)
    {
        wm_tool_delay_ms(1 * 1000);
		wm_tool_set_wifi_chip_speed(WM_TOOL_DEFAULT_BAUD_RATE);
		wm_tool_delay_ms(1 * 1000);
    }

    if (!ret)
    {
        if (WM_TOOL_DL_TYPE_FLS == wm_tool_dl_type)
        {
            if (WM_TOOL_DL_ACTION_RTS == wm_tool_dl_action)/* auto reset */
            {
                wm_tool_uart_set_dtr(0);
                wm_tool_uart_set_rts(1);
                wm_tool_delay_ms(50);
                wm_tool_uart_set_dtr(1);
                wm_tool_uart_set_rts(0);
                wm_tool_delay_ms(50);
        		wm_tool_uart_set_dtr(0);
    		}
    		else
    		{
                wm_tool_printf("please manually reset the device.\r\n");
    		}
        }
    }

    wm_tool_uart_close();

    return ret;
}

static void wm_tool_show_local_com(void)
{
#if defined(__APPLE__) && defined(__MACH__)
    char *comstr = "tty.usbserial";
#elif defined(__CYGWIN__)
    int num;
    char *comstr = "ttyS";
#elif defined(__linux__)
    char *comstr = "ttyUSB";
#endif

#if defined(__APPLE__) || defined(__MACH__) || defined(__CYGWIN__) || defined(__linux__)

    DIR *dir;
	struct dirent *file;

	dir = opendir("/dev");

	if (dir)
	{
		while (NULL != (file = readdir(dir)))
		{

			if ((0 == strncmp(file->d_name, comstr, strlen(comstr))) && (DT_CHR == file->d_type))
			{
#if defined(__CYGWIN__)
                num = atoi(file->d_name + strlen(comstr));
	        	wm_tool_printf("COM%d ", num + 1);
#else
                wm_tool_printf("%s ", file->d_name);
#endif
			}
		}
		closedir(dir);
		wm_tool_printf("\r\n");
	}

#elif defined(__MINGW32__)

    LONG ret;
    HKEY key;
	DWORD i = 0;
	DWORD knlen;
    char kname[WM_TOOL_PATH_MAX] = {0};
	DWORD kvlen;
    char kvalue[WM_TOOL_PATH_MAX] = {0};
    REGSAM kmask = KEY_READ;

#if defined(KEY_WOW64_64KEY)
    BOOL is_64;

    if (IsWow64Process(GetCurrentProcess(), &is_64))
    {
        if (is_64)
            kmask |= KEY_WOW64_64KEY;
    }
#endif

    ret = RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, NULL, 0, kmask, NULL, &key, NULL);
    if (ret != ERROR_SUCCESS)
    {
        return;
    }

	do{
	    knlen = sizeof(kname);
	    kvlen = sizeof(kvalue);
		ret = RegEnumValue(key, i, kname, &knlen, 0, NULL, (LPBYTE)kvalue, &kvlen);
		if (ret == ERROR_SUCCESS)
		{
            if (strcmp(kvalue, "") != 0)
            {
    			wm_tool_printf("%s ", kvalue);
		    }
		}

		i++;
	} while (ret != ERROR_NO_MORE_ITEMS);

    RegCloseKey(key);

    wm_tool_printf("\r\n");

#else

    wm_tool_printf("\r\nunsupported system.\r\n");

#endif

    return;
}
static void wm_tool_free_res(void)
{
    if (wm_tool_download_image)
        free(wm_tool_download_image);

    if (wm_tool_output_image)
        free(wm_tool_output_image);

    if (wm_tool_input_binary)
        free(wm_tool_input_binary);

    if (wm_tool_secboot_image)
        free(wm_tool_secboot_image);

    return;
}

int main(int argc, char *argv[])
{
    int ret = 0;

    if (!wm_tool_parse_arv(argc, argv))
    {
        wm_tool_print_usage(argv[0]);
        wm_tool_free_res();
        return 0;
    }

    if (wm_tool_show_usage)
    {
        wm_tool_print_usage(argv[0]);
        wm_tool_free_res();
        return 0;
    }

    if (wm_tool_show_ver)
    {
        wm_tool_print_version(argv[0]);
        wm_tool_free_res();
        return 0;
    }

    if (wm_tool_list_com)
    {
        wm_tool_show_local_com();
        wm_tool_free_res();
        return 0;
    }

    if (wm_tool_input_binary)
    {
        ret = wm_tool_pack_firmware();
    }

    if (wm_tool_download_image || wm_tool_dl_erase)
    {
        ret = wm_tool_download_firmware();
    }

    if (wm_tool_show_log_type)
    {
        ret = wm_tool_show_log_from_serial();
    }

    if (ret > 0)
        wm_tool_print_usage(argv[0]);

    wm_tool_free_res();

    return ret;
}

