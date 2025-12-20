/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: RT-Thread Finsh/MSH command for EasyFlash.
 * Created on: 2018-05-19
 */

#include <easyflash.h>
#include <rtthread.h>

#if defined(RT_USING_FINSH) && defined(FINSH_USING_MSH) && defined(EF_USING_ENV)
#include <finsh.h>
#if defined(EF_USING_ENV)
static void __setenv(uint8_t argc, char **argv) {
    uint8_t i;

    if (argc > 3) {
        /* environment variable value string together */
        for (i = 0; i < argc - 2; i++) {
            argv[2 + i][rt_strlen(argv[2 + i])] = ' ';
        }
    }
    if (argc == 1) {
        rt_kprintf("Please input: setenv <key> [value]\n");
    } else if (argc == 2) {
        ef_set_env(argv[1], NULL);
    } else {
        ef_set_env(argv[1], argv[2]);
    }
}
MSH_CMD_EXPORT_ALIAS(__setenv, setenv, Set an envrionment variable.);

static void printenv(uint8_t argc, char **argv) {
    ef_print_env();
}
MSH_CMD_EXPORT(printenv, Print all envrionment variables.);

static void saveenv(uint8_t argc, char **argv) {
    ef_save_env();
}
MSH_CMD_EXPORT(saveenv, Save all envrionment variables to flash.);

static void getvalue(uint8_t argc, char **argv) {
    char *value = NULL;
    value = ef_get_env(argv[1]);
    if (value) {
        rt_kprintf("The %s value is %s.\n", argv[1], value);
    } else {
        rt_kprintf("Can't find %s.\n", argv[1]);
    }
}
MSH_CMD_EXPORT(getvalue, Get an envrionment variable by name.);

static void resetenv(uint8_t argc, char **argv) {
    ef_env_set_default();
}
MSH_CMD_EXPORT(resetenv, Reset all envrionment variable to default.);
#endif /* defined(EF_USING_ENV) */
#endif /* defined(RT_USING_FINSH) && defined(FINSH_USING_MSH) */
