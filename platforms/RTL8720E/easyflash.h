#include "kv.h"
#define ef_get_env_blob(a,b,c,d) rt_kv_get(a,b,c)
#define ef_set_env_blob rt_kv_set
#define EF_ENV_INIT_FAILED -1
#define EF_ENV_NAME_ERR -1
#define easyflash_init()
typedef int EfErrCode;
