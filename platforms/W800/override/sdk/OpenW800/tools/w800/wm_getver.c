#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LINE_LEN_MAX    1024

int main(int argc, char *argv[])
{
    FILE *fp;
    char buf[LINE_LEN_MAX];
    char *pos;
    unsigned char major = 0;
    unsigned char minor = 0;
    unsigned char patch = 0;
    int done = 0;

    if (2 != argc)
    {
        printf("error: parameters please pass wm_main.c path\r\n");
        return -1;
    }

    fp = fopen(argv[1], "rb");
    if (!fp)
    {
        printf("error: failed to open %s\r\n", argv[1]);
        return -2;
    }

    while (fgets(buf, LINE_LEN_MAX, fp))
    {
        if (0 == done)
        {
            pos = strstr(buf, "FW_MAJOR_VER");
            if (pos)
            {
                pos = strchr(pos, '0');
                if (pos)
                {
                    major = strtol(pos, NULL, 16);
                    done = 1;
                }
            }
        }

        if (1 == done)
        {
            pos = strstr(buf, "FW_MINOR_VER");
            if (pos)
            {
                pos = strchr(pos, '0');
                if (pos)
                {
                    minor = strtol(pos, NULL, 16);
                    done = 2;
                }
            }
        }

        if (2 == done)
        {
            pos = strstr(buf, "FW_PATCH_VER");
            if (pos)
            {
                pos = strchr(pos, '0');
                if (pos)
                {
                    patch = strtol(pos, NULL, 16);
                    done = 3;
                    break;
                }
            }
        }
    }

    fclose(fp);

    if (3 == done)
    {
        sprintf(buf, "G%02X.%02X.%02X", major, minor, patch);
        printf("%s", buf);
        return 0;
    }

    return -4;
}

