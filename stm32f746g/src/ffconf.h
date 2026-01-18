#define _FFCONF 68300   /* Revision ID */
#define _FS_READONLY    1
#define _FS_MINIMIZE    1
#define _USE_STRFUNC    0
#define _USE_FIND               0
#define _USE_MKFS               0
#define _USE_FASTSEEK   1
#define _USE_EXPAND             0
#define _USE_CHMOD              0
#define _USE_LABEL              0
#define _USE_FORWARD    0
#define _CODE_PAGE      850
#define _USE_LFN        3
#define _MAX_LFN        255
#define _LFN_UNICODE    0
#define _STRF_ENCODE    3
#define _FS_RPATH       0
#define _VOLUMES        2
#define _STR_VOLUME_ID  0
#define _VOLUME_STRS    "RAM","NAND","CF","SD","SD2","USB","USB2","USB3"
#define _MULTI_PARTITION        0
#define _MIN_SS         512
#define _MAX_SS         512
#define _USE_TRIM       0
#define _FS_NOFSINFO    0
#define _FS_TINY        0
#define _FS_EXFAT       0
#define _FS_NORTC       0
#define _NORTC_MON      1
#define _NORTC_MDAY     1
#define _NORTC_YEAR     2016
#define _FS_LOCK        0
#define _FS_REENTRANT   0

#if _FS_REENTRANT
#include "cmsis_os.h"
#define _FS_TIMEOUT             1000
#define _SYNC_t         osSemaphoreId
#endif

#if _USE_LFN == 3
#if !defined(ff_malloc) || !defined(ff_free)
#include <stdlib.h>
#endif

#if !defined(ff_malloc)
#define ff_malloc malloc
#endif

#if !defined(ff_free)
#define ff_free free
#endif
#endif
