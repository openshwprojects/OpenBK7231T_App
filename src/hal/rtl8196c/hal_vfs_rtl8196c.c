#ifdef PLATFORM_RTL8196C

#define OBK_VFS_IMPLEMENTATION 1
#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../cmnds/cmd_public.h"
#include "../../littlefs/our_lfs.h"

#include "hal_rtl8196c.h"
#include "rtl8196c.h"
#include "irq.h"
#include "timer.h"
#include "usb.h"
#include "usb_msc.h"
#include "ff.h"
#include "ntfs.h"

#define VFS_MAX_DRIVES   4
#define VFS_MAX_FILES    4
#define VFS_MAX_DIRS     4
#define VFS_HANDLE_MAGIC 0x55534200u /* 'USB\0' */
#define VFS_HANDLE_MASK  0xFFFFFF00u

enum {
    VFS_FS_NONE = 0,
    VFS_FS_FAT  = 1,
    VFS_FS_NTFS = 2,
};

struct vfs_drive {
    int             fs_type;
    int             write_state; /* 0 = unchecked, 1 = writable, -1 = hw read-only */
    int             usb_addr;
    uint8_t         hub_addr;
    uint8_t         hub_port;
    uint32_t        size_mb;
    char            label[64];
    char            fs_name[12];
    struct usb_msc  msc;
    FATFS           fat_vol;
    struct ntfs_fs  ntfs_vol;
};

struct vfs_file_handle {
    int              in_use;
    int              slot;
    int              fs_type;
    int              dirty;
    FIL              fil;
    struct ntfs_file nfile;
    uint64_t         ntfs_pos;
    uint32_t         ntfs_dir_mft;
    char             ntfs_name[128];
};

struct vfs_dir_handle {
    int             in_use;
    int             slot;
    int             fs_type;
    DIR             fdir;
    uint32_t        ntfs_mft;
    uint32_t        step;      /* 0=".", 1="..", 2+=entries */
};

static struct vfs_drive       g_drives[VFS_MAX_DRIVES];
static struct vfs_file_handle g_files[VFS_MAX_FILES];
static struct vfs_dir_handle  g_dirs[VFS_MAX_DIRS];

static lfs_dir_t *g_root_dir_ptr = 0;
static int        g_root_usb_idx = 0;

void diskio_usb_attach_slot(int pdrv, struct usb_msc *m);

static int msc_ntfs_reader(void *ctx, uint32_t lba, uint32_t count, void *buf)
{
    return msc_read((struct usb_msc *)ctx, lba, (uint16_t)count, buf);
}

static int msc_ntfs_writer(void *ctx, uint32_t lba, uint32_t count, const void *buf)
{
    return msc_write((struct usb_msc *)ctx, lba, (uint16_t)count, buf);
}

static int hex_val(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void url_decode_copy(char *dst, size_t dst_sz, const char *src)
{
    size_t o = 0;
    while (*src && o + 1 < dst_sz) {
        if (src[0] == '%' && hex_val(src[1]) >= 0 && hex_val(src[2]) >= 0) {
            dst[o++] = (char)((hex_val(src[1]) << 4) | hex_val(src[2]));
            src += 3;
        } else {
            dst[o++] = *src++;
        }
    }
    dst[o] = 0;
    while (o > 0 && dst[o - 1] == '/') {
        dst[--o] = 0;
    }
}

/*
 * Returns:
 *   0 = normal LittleFS path
 *   1 = mounted USB path (/usb0..3), *out_slot and out_sub populated
 *  -1 = unmounted /usb0..3 path (caller should return LFS_ERR_NOENT)
 */
static int parse_usb_path(const char *path, int *out_slot, char *out_sub, size_t sub_sz)
{
    if (!path)
        return 0;
    const char *p = path;
    while (*p == '/')
        p++;
    if (p[0] == 'u' && p[1] == 's' && p[2] == 'b' &&
        p[3] >= '0' && p[3] < ('0' + VFS_MAX_DRIVES) &&
        (p[4] == '\0' || p[4] == '/')) {
        int slot = p[3] - '0';
        if (g_drives[slot].fs_type == VFS_FS_NONE)
            return -1;
        *out_slot = slot;
        const char *rest = p + 4;
        while (*rest == '/')
            rest++;
        url_decode_copy(out_sub, sub_sz, rest);
        return 1;
    }
    return 0;
}

void obk_vfs_statfs(const char *path, uint64_t *out_total_units, uint64_t *out_free_units,
                    uint32_t *out_serial, char *out_label, int label_sz, char *out_fs, int fs_sz)
{
    int slot = -1;
    char sub[16];
    if (parse_usb_path(path, &slot, sub, sizeof(sub)) == 1 && slot >= 0 && slot < VFS_MAX_DRIVES) {
        struct vfs_drive *drv = &g_drives[slot];
        uint64_t bytes = (uint64_t)drv->msc.blocks * (uint64_t)(drv->msc.block_size ? drv->msc.block_size : 512u);
        if (bytes == 0)
            bytes = (uint64_t)drv->size_mb << 20;
        uint64_t units = bytes >> 12; /* 4 KB allocation units */
        if (units == 0)
            units = 262144ULL;
        *out_total_units = units;
        *out_free_units  = (units * 3ULL) >> 2;
        *out_serial      = 0x8196C100u + (uint32_t)slot;
        snprintf(out_label, (size_t)label_sz, "%s", drv->label[0] ? drv->label : "USB Drive");
        snprintf(out_fs, (size_t)fs_sz, "%s", drv->fs_name[0] ? drv->fs_name : "NTFS");
        return;
    }

    uint64_t sum_bytes = 512ULL * 1024ULL; /* 512 KB SPI Flash LittleFS */
    for (int s = 0; s < VFS_MAX_DRIVES; s++) {
        if (g_drives[s].fs_type != VFS_FS_NONE) {
            uint64_t b = (uint64_t)g_drives[s].msc.blocks * (uint64_t)(g_drives[s].msc.block_size ? g_drives[s].msc.block_size : 512u);
            if (b == 0)
                b = (uint64_t)g_drives[s].size_mb << 20;
            sum_bytes += b;
        }
    }
    uint64_t units = sum_bytes >> 12;
    if (units < 128ULL)
        units = 262144ULL;
    *out_total_units = units;
    *out_free_units  = (units * 3ULL) >> 2;
    *out_serial      = 0x8196C001u;
    snprintf(out_label, (size_t)label_sz, "OpenBeken VFS");
    snprintf(out_fs, (size_t)fs_sz, "NTFS");
}

static int drv_ensure_writable(int slot)
{
    struct vfs_drive *drv = &g_drives[slot];
    if (drv->write_state > 0)
        return 0;
    if (drv->write_state < 0)
        return LFS_ERR_IO;
    if (drv->msc.blocks < 128) {
        drv->write_state = -1;
        return LFS_ERR_IO;
    }
    static uint8_t orig[512] __attribute__((aligned(32)));
    static uint8_t pat[512]  __attribute__((aligned(32)));
    static uint8_t back[512] __attribute__((aligned(32)));
    for (int t = 0; t < 5; t++) {
        if (msc_test_unit_ready(&drv->msc) == 0)
            break;
        delay_ms(20);
    }
    uint32_t lba = drv->msc.blocks - 64u;
    if (msc_read(&drv->msc, lba, 1, orig) < 0)
        return LFS_ERR_IO;
    for (int k = 0; k < 512; k++)
        pat[k] = (uint8_t)(orig[k] ^ 0x5Au);
    memset(back, 0, sizeof(back));
    cctl_write(CCTL_DWBINVAL);
    int w1 = msc_write(&drv->msc, lba, 1, pat);
    int r2 = msc_read(&drv->msc, lba, 1, back);
    int w2 = msc_write(&drv->msc, lba, 1, orig);
    if (w1 < 0 || r2 < 0 || w2 < 0 || memcmp(pat, back, 512) != 0) {
        drv->write_state = -1;
        ADDLOG_ERROR(LOG_FEATURE_MAIN,
                     "USB /usb%d (%s) hardware write-verify failed (readback=%02x%02x%02x%02x); marking read-only to protect volume",
                     slot, drv->label, back[0], back[1], back[2], back[3]);
        return LFS_ERR_IO;
    }
    drv->write_state = 1;
    return 0;
}

static void fat_mkdir_parents(int slot, const char *sub)
{
    char tmp[264];
    const char *p = sub;
    while ((p = strchr(p, '/')) != 0) {
        size_t len = (size_t)(p - sub);
        if (len > 0 && len < 240) {
            snprintf(tmp, sizeof(tmp), "%d:/%.*s", slot, (int)len, sub);
            f_mkdir(tmp);
        }
        while (*p == '/')
            p++;
    }
}

/* ---------------- NTFS path resolution & indexed directory reader -------- */

struct ntfs_find_ctx {
    const char *name;
    size_t      len;
    uint32_t    mft;
    uint32_t    attr;
    uint64_t    size;
    int         found;
};

static int ascii_strncasecmp(const char *a, const char *b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca != cb || ca == 0)
            return (int)ca - (int)cb;
    }
    return 0;
}

static int ntfs_find_cb(const struct ntfs_dirent *e, void *arg)
{
    struct ntfs_find_ctx *c = (struct ntfs_find_ctx *)arg;
    if (strlen(e->name) == c->len && ascii_strncasecmp(e->name, c->name, c->len) == 0) {
        c->mft   = e->mft;
        c->attr  = e->attr;
        c->size  = e->size;
        c->found = 1;
        return 1;
    }
    return 0;
}

static int ntfs_resolve_path(struct ntfs_fs *fs, const char *subpath,
                             uint32_t *out_mft, uint32_t *out_attr, uint64_t *out_size)
{
    uint32_t cur_mft  = NTFS_ROOT;
    uint32_t cur_attr = NTFS_ATTR_DIRECTORY;
    uint64_t cur_size = 0;

    const char *p = subpath;
    while (*p) {
        while (*p == '/')
            p++;
        if (!*p)
            break;
        const char *slash = strchr(p, '/');
        size_t clen = slash ? (size_t)(slash - p) : strlen(p);
        if (!(cur_attr & NTFS_ATTR_DIRECTORY))
            return LFS_ERR_NOTDIR;

        struct ntfs_find_ctx ctx;
        ctx.name  = p;
        ctx.len   = clen;
        ctx.found = 0;
        if (ntfs_list(fs, cur_mft, ntfs_find_cb, &ctx) < 0 || !ctx.found)
            return LFS_ERR_NOENT;

        cur_mft  = ctx.mft;
        cur_attr = ctx.attr;
        cur_size = ctx.size;
        p += clen;
    }

    *out_mft  = cur_mft;
    *out_attr = cur_attr;
    *out_size = cur_size;
    return 0;
}

/* Resolve parent directory MFT and leaf filename; optionally create missing parent dirs */
static int ntfs_resolve_parent(struct ntfs_fs *fs, const char *subpath, int create_Switch,
                               uint32_t *out_dir_mft, char *out_leaf, size_t leaf_sz)
{
    uint32_t cur_mft = NTFS_ROOT;
    const char *p = subpath;
    while (*p == '/')
        p++;
    if (!*p)
        return LFS_ERR_INVAL;

    for (;;) {
        const char *slash = strchr(p, '/');
        if (!slash) {
            strncpy(out_leaf, p, leaf_sz - 1);
            out_leaf[leaf_sz - 1] = 0;
            *out_dir_mft = cur_mft;
            return 0;
        }
        size_t clen = (size_t)(slash - p);
        if (clen == 0 || clen >= 128)
            return LFS_ERR_INVAL;

        struct ntfs_find_ctx ctx;
        ctx.name  = p;
        ctx.len   = clen;
        ctx.found = 0;
        if (ntfs_list(fs, cur_mft, ntfs_find_cb, &ctx) == 0 && ctx.found) {
            if (!(ctx.attr & NTFS_ATTR_DIRECTORY))
                return LFS_ERR_NOTDIR;
            cur_mft = ctx.mft;
        } else if (create_Switch) {
            char comp[128];
            memcpy(comp, p, clen);
            comp[clen] = 0;
            uint32_t new_dir = 0;
            if (ntfs_create(fs, cur_mft, comp, 1, &new_dir) < 0)
                return LFS_ERR_IO;
            cur_mft = new_dir;
        } else {
            return LFS_ERR_NOENT;
        }

        p = slash;
        while (*p == '/')
            p++;
        if (!*p)
            return LFS_ERR_INVAL;
    }
}

struct ntfs_nth_ctx {
    uint32_t         target;
    uint32_t         cur;
    struct lfs_info *info;
    int              found;
};

static int ntfs_nth_cb(const struct ntfs_dirent *e, void *arg)
{
    if (e->name[0] == '$')
        return 0;
    struct ntfs_nth_ctx *c = (struct ntfs_nth_ctx *)arg;
    if (c->cur++ == c->target) {
        c->info->type = (e->attr & NTFS_ATTR_DIRECTORY) ? LFS_TYPE_DIR : LFS_TYPE_REG;
        c->info->size = (e->attr & NTFS_ATTR_DIRECTORY) ? 0 : (lfs_size_t)e->size;
        strncpy(c->info->name, e->name, LFS_NAME_MAX);
        c->info->name[LFS_NAME_MAX] = 0;
        c->found = 1;
        return 1;
    }
    return 0;
}

/* ---------------------------- VFS File Operations ------------------------ */

int obk_vfs_file_open(lfs_t *lfs, lfs_file_t *file, const char *path, int flags)
{
    int slot = -1;
    char sub[256];
    int pr = parse_usb_path(path, &slot, sub, sizeof(sub));
    if (pr == 0)
        return lfs_file_open(lfs, file, path, flags);
    if (pr < 0)
        return LFS_ERR_NOENT;

    /* Opening "/usb0" or "usb0/" itself is a directory open check from rest_interface.c */
    if (sub[0] == '\0')
        return LFS_ERR_ISDIR;

    if (((flags & 3) != LFS_O_RDONLY || (flags & (LFS_O_CREAT | LFS_O_TRUNC | LFS_O_APPEND))) &&
        drv_ensure_writable(slot) < 0)
        return LFS_ERR_IO;

    struct vfs_drive *drv = &g_drives[slot];
    int h_idx = -1;
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (!g_files[i].in_use) {
            h_idx = i;
            break;
        }
    }
    if (h_idx < 0)
        return LFS_ERR_NOMEM;

    struct vfs_file_handle *h = &g_files[h_idx];
    memset(h, 0, sizeof(*h));

    if (drv->fs_type == VFS_FS_FAT) {
        char fat_path[264];
        snprintf(fat_path, sizeof(fat_path), "%d:/%s", slot, sub);

        if ((flags & 3) == LFS_O_RDONLY) {
            FILINFO fno;
            if (f_stat(fat_path, &fno) == FR_OK && (fno.fattrib & AM_DIR))
                return LFS_ERR_ISDIR;
        } else if (flags & LFS_O_CREAT) {
            fat_mkdir_parents(slot, sub);
        }

        BYTE mode = 0;
        int acc = flags & 3;
        if (acc == LFS_O_RDONLY) mode = FA_READ;
        else if (acc == LFS_O_WRONLY) mode = FA_WRITE;
        else mode = FA_READ | FA_WRITE;

        if (flags & LFS_O_TRUNC)
            mode |= FA_CREATE_ALWAYS;
        else if (flags & LFS_O_APPEND)
            mode |= FA_OPEN_APPEND;
        else if (flags & LFS_O_CREAT)
            mode |= FA_OPEN_ALWAYS;

        FRESULT fr = f_open(&h->fil, fat_path, mode);
        if (fr == FR_NO_FILE || fr == FR_NO_PATH) {
            FILINFO fno;
            if (f_stat(fat_path, &fno) == FR_OK && (fno.fattrib & AM_DIR))
                return LFS_ERR_ISDIR;
            return LFS_ERR_NOENT;
        }
        if (fr != FR_OK)
            return LFS_ERR_IO;

        h->in_use  = 1;
        h->slot    = slot;
        h->fs_type = VFS_FS_FAT;
        file->flags = VFS_HANDLE_MAGIC | (uint32_t)h_idx;
        return 0;
    }

    if (drv->fs_type == VFS_FS_NTFS) {
        uint32_t dir_mft = NTFS_ROOT;
        char leaf[128];
        if (ntfs_resolve_parent(&drv->ntfs_vol, sub, (flags & LFS_O_CREAT) != 0,
                                &dir_mft, leaf, sizeof(leaf)) < 0)
            return LFS_ERR_NOENT;

        uint32_t mft = 0, attr = 0;
        uint64_t fsize = 0;
        int rc = ntfs_resolve_path(&drv->ntfs_vol, sub, &mft, &attr, &fsize);
        if (rc == LFS_ERR_NOENT && (flags & LFS_O_CREAT)) {
            if (ntfs_create(&drv->ntfs_vol, dir_mft, leaf, 0, &mft) < 0)
                return LFS_ERR_IO;
            attr = NTFS_ATTR_ARCHIVE;
            fsize = 0;
            rc = 0;
            h->dirty = 1;
        }
        if (rc < 0)
            return rc;
        if (attr & NTFS_ATTR_DIRECTORY)
            return LFS_ERR_ISDIR;
        if (ntfs_open(&drv->ntfs_vol, mft, &h->nfile) < 0)
            return LFS_ERR_IO;

        if ((flags & LFS_O_TRUNC) && h->nfile.size > 0) {
            if (ntfs_truncate(&drv->ntfs_vol, &h->nfile, dir_mft, leaf, 0) < 0)
                return LFS_ERR_IO;
            h->dirty = 1;
        }

        h->in_use       = 1;
        h->slot         = slot;
        h->fs_type      = VFS_FS_NTFS;
        h->ntfs_dir_mft = dir_mft;
        strncpy(h->ntfs_name, leaf, sizeof(h->ntfs_name) - 1);
        h->ntfs_pos     = (flags & LFS_O_APPEND) ? h->nfile.size : 0;
        file->flags     = VFS_HANDLE_MAGIC | (uint32_t)h_idx;
        return 0;
    }

    return LFS_ERR_IO;
}

static struct vfs_file_handle *get_usb_file(lfs_file_t *file)
{
    if (!file || (file->flags & VFS_HANDLE_MASK) != VFS_HANDLE_MAGIC)
        return 0;
    uint32_t idx = file->flags & ~VFS_HANDLE_MASK;
    if (idx >= VFS_MAX_FILES || !g_files[idx].in_use)
        return 0;
    return &g_files[idx];
}

#define VFS_STREAM_BUF_SZ 65536u
static uint8_t s_rd_buf[VFS_STREAM_BUF_SZ] __attribute__((aligned(32)));
static struct vfs_file_handle *s_rd_h = 0;
static uint64_t s_rd_off = 0;
static uint32_t s_rd_len = 0;
static uint32_t s_rd_pos = 0;

static uint8_t s_wr_buf[VFS_STREAM_BUF_SZ] __attribute__((aligned(32)));
static struct vfs_file_handle *s_wr_h = 0;
static uint32_t s_wr_len = 0;

static void vfs_invalidate_read(struct vfs_file_handle *h, int sync_fat_pos)
{
    if (s_rd_h && (!h || s_rd_h == h)) {
        if (sync_fat_pos && s_rd_h->fs_type == VFS_FS_FAT && s_rd_pos < s_rd_len)
            f_lseek(&s_rd_h->fil, (FSIZE_t)(s_rd_off + s_rd_pos));
        s_rd_h = 0;
        s_rd_len = 0;
        s_rd_pos = 0;
    }
}

static int vfs_flush_write(struct vfs_file_handle *h)
{
    if (!s_wr_h || (h && s_wr_h != h))
        return 0;
    struct vfs_file_handle *wh = s_wr_h;
    uint32_t len = s_wr_len;
    s_wr_h = 0;
    s_wr_len = 0;
    if (len == 0)
        return 0;
    cctl_write(CCTL_DWB); /* flush cached writes in s_wr_buf to physical SDRAM before USB DMA */
    vfs_invalidate_read(wh, 0);
    if (wh->fs_type == VFS_FS_FAT) {
        UINT bw = 0;
        FRESULT fr = f_write(&wh->fil, s_wr_buf, (UINT)len, &bw);
        if (fr != FR_OK || bw != (UINT)len) {
            ADDLOG_ERROR(LOG_FEATURE_MAIN, "FAT f_write slot %d err=%d sense=%x/%02x/%02x",
                         wh->slot, (int)fr, g_drives[wh->slot].msc.sense_key,
                         g_drives[wh->slot].msc.asc, g_drives[wh->slot].msc.ascq);
            return LFS_ERR_IO;
        }
        return 0;
    }
    if (wh->fs_type == VFS_FS_NTFS) {
        int n = ntfs_write(&g_drives[wh->slot].ntfs_vol, &wh->nfile,
                           0, 0, wh->ntfs_pos, s_wr_buf, len);
        if (n != (int)len)
            return LFS_ERR_IO;
        wh->ntfs_pos += (uint64_t)n;
        wh->dirty = 1;
        return 0;
    }
    return LFS_ERR_IO;
}

int obk_vfs_file_close(lfs_t *lfs, lfs_file_t *file)
{
    struct vfs_file_handle *h = get_usb_file(file);
    if (!h)
        return lfs_file_close(lfs, file);
    int rc = 0;
    if (s_wr_h == h && vfs_flush_write(h) < 0)
        rc = LFS_ERR_IO;
    vfs_invalidate_read(h, 0);
    if (h->fs_type == VFS_FS_FAT) {
        FRESULT fr = f_close(&h->fil);
        if (fr != FR_OK) {
            ADDLOG_ERROR(LOG_FEATURE_MAIN, "FAT f_close slot %d err=%d sense=%x/%02x/%02x",
                         h->slot, (int)fr, g_drives[h->slot].msc.sense_key,
                         g_drives[h->slot].msc.asc, g_drives[h->slot].msc.ascq);
            rc = LFS_ERR_IO;
        }
    } else if (h->fs_type == VFS_FS_NTFS && h->dirty) {
        if (ntfs_sync(&g_drives[h->slot].ntfs_vol, &h->nfile, h->ntfs_dir_mft, h->ntfs_name) < 0)
            rc = LFS_ERR_IO;
        msc_sync(&g_drives[h->slot].msc);
    }
    h->in_use = 0;
    file->flags = 0;
    return rc;
}

int obk_vfs_file_sync(lfs_t *lfs, lfs_file_t *file)
{
    struct vfs_file_handle *h = get_usb_file(file);
    if (!h)
        return lfs_file_sync(lfs, file);
    if (s_wr_h == h && vfs_flush_write(h) < 0)
        return LFS_ERR_IO;
    if (h->fs_type == VFS_FS_FAT)
        return (f_sync(&h->fil) == FR_OK) ? 0 : LFS_ERR_IO;
    if (h->fs_type == VFS_FS_NTFS && h->dirty) {
        if (ntfs_sync(&g_drives[h->slot].ntfs_vol, &h->nfile, h->ntfs_dir_mft, h->ntfs_name) < 0)
            return LFS_ERR_IO;
        msc_sync(&g_drives[h->slot].msc);
    }
    return 0;
}

typedef void *(*kseg0_memcpy_fn)(void *, const void *, size_t);

static void fast_kseg0_copy(void *dst, const void *src, size_t n)
{
    void *cdst = (void *)((uintptr_t)dst & ~0x20000000u);
    const void *csrc = (const void *)((uintptr_t)src & ~0x20000000u);
    kseg0_memcpy_fn fn = (kseg0_memcpy_fn)((uintptr_t)memcpy & ~0x20000000u);
    fn(cdst, csrc, n);
}

lfs_ssize_t obk_vfs_file_read(lfs_t *lfs, lfs_file_t *file, void *buffer, lfs_size_t size)
{
    struct vfs_file_handle *h = get_usb_file(file);
    if (!h)
        return lfs_file_read(lfs, file, buffer, size);
    if (s_wr_h == h && vfs_flush_write(h) < 0)
        return LFS_ERR_IO;

    uint8_t *dst = (uint8_t *)buffer;
    lfs_size_t remaining = size;
    lfs_size_t total = 0;
    while (remaining > 0) {
        if (s_rd_h == h && s_rd_pos < s_rd_len) {
            uint32_t avail = s_rd_len - s_rd_pos;
            uint32_t take = (remaining < avail) ? (uint32_t)remaining : avail;
            fast_kseg0_copy(dst, s_rd_buf + s_rd_pos, take);
            cctl_write(CCTL_DWB);
            s_rd_pos += take;
            if (h->fs_type == VFS_FS_NTFS)
                h->ntfs_pos += take;
            dst += take;
            remaining -= take;
            total += take;
            continue;
        }
        vfs_invalidate_read(0, 1);
        /* Direct zero-copy DMA into caller buffer for large reads (>= 64 KB) */
        if (remaining >= VFS_STREAM_BUF_SZ) {
            if (h->fs_type == VFS_FS_FAT) {
                UINT br = 0;
                if (f_read(&h->fil, dst, (UINT)remaining, &br) != FR_OK)
                    return total > 0 ? (lfs_ssize_t)total : LFS_ERR_IO;
                total += br;
                break;
            } else if (h->fs_type == VFS_FS_NTFS) {
                int n = ntfs_read(&g_drives[h->slot].ntfs_vol, &h->nfile, h->ntfs_pos, dst, (uint32_t)remaining);
                if (n < 0)
                    return total > 0 ? (lfs_ssize_t)total : LFS_ERR_IO;
                h->ntfs_pos += (uint64_t)n;
                total += (uint32_t)n;
                break;
            }
        }
        if (h->fs_type == VFS_FS_FAT) {
            UINT br = 0;
            uint64_t off0 = (uint64_t)f_tell(&h->fil);
            if (f_read(&h->fil, s_rd_buf, VFS_STREAM_BUF_SZ, &br) != FR_OK)
                return total > 0 ? (lfs_ssize_t)total : LFS_ERR_IO;
            if (br == 0)
                break;
            cctl_write(CCTL_DWBINVAL);
            s_rd_h = h;
            s_rd_off = off0;
            s_rd_len = (uint32_t)br;
            s_rd_pos = 0;
        } else if (h->fs_type == VFS_FS_NTFS) {
            int n = ntfs_read(&g_drives[h->slot].ntfs_vol, &h->nfile, h->ntfs_pos, s_rd_buf, VFS_STREAM_BUF_SZ);
            if (n < 0)
                return total > 0 ? (lfs_ssize_t)total : LFS_ERR_IO;
            if (n == 0)
                break;
            cctl_write(CCTL_DWBINVAL);
            s_rd_h = h;
            s_rd_off = h->ntfs_pos;
            s_rd_len = (uint32_t)n;
            s_rd_pos = 0;
        } else {
            return LFS_ERR_IO;
        }
    }
    return (lfs_ssize_t)total;
}

lfs_ssize_t obk_vfs_file_write(lfs_t *lfs, lfs_file_t *file, const void *buffer, lfs_size_t size)
{
    struct vfs_file_handle *h = get_usb_file(file);
    if (!h)
        return lfs_file_write(lfs, file, buffer, size);
    if (size == 0)
        return 0;
    vfs_invalidate_read(h, 1);
    if (s_wr_h && s_wr_h != h && vfs_flush_write(s_wr_h) < 0)
        return LFS_ERR_IO;

    const uint8_t *src = (const uint8_t *)buffer;
    lfs_size_t remaining = size;
    s_wr_h = h;
    cctl_write(CCTL_DWBINVAL);
    while (remaining > 0) {
        uint32_t space = VFS_STREAM_BUF_SZ - s_wr_len;
        uint32_t take = (remaining < space) ? (uint32_t)remaining : space;
        fast_kseg0_copy(s_wr_buf + s_wr_len, src, take);
        s_wr_len += take;
        src += take;
        remaining -= take;
        if (s_wr_len == VFS_STREAM_BUF_SZ) {
            if (vfs_flush_write(h) < 0)
                return LFS_ERR_IO;
            s_wr_h = h;
        }
    }
    return (lfs_ssize_t)size;
}

lfs_soff_t obk_vfs_file_seek(lfs_t *lfs, lfs_file_t *file, lfs_soff_t off, int whence)
{
    struct vfs_file_handle *h = get_usb_file(file);
    if (!h)
        return lfs_file_seek(lfs, file, off, whence);
    if (s_wr_h == h && vfs_flush_write(h) < 0)
        return LFS_ERR_IO;
    vfs_invalidate_read(h, 1);
    if (h->fs_type == VFS_FS_FAT) {
        FSIZE_t target = 0;
        if (whence == LFS_SEEK_SET) target = (FSIZE_t)off;
        else if (whence == LFS_SEEK_CUR) target = (FSIZE_t)((lfs_soff_t)f_tell(&h->fil) + off);
        else if (whence == LFS_SEEK_END) target = (FSIZE_t)((lfs_soff_t)f_size(&h->fil) + off);
        if (f_lseek(&h->fil, target) != FR_OK)
            return LFS_ERR_IO;
        return (lfs_soff_t)f_tell(&h->fil);
    }
    if (h->fs_type == VFS_FS_NTFS) {
        int64_t target = 0;
        if (whence == LFS_SEEK_SET) target = off;
        else if (whence == LFS_SEEK_CUR) target = (int64_t)h->ntfs_pos + off;
        else if (whence == LFS_SEEK_END) target = (int64_t)h->nfile.size + off;
        if (target < 0) target = 0;
        h->ntfs_pos = (uint64_t)target;
        return (lfs_soff_t)h->ntfs_pos;
    }
    return LFS_ERR_IO;
}

int obk_vfs_file_truncate(lfs_t *lfs, lfs_file_t *file, lfs_off_t size)
{
    struct vfs_file_handle *h = get_usb_file(file);
    if (!h)
        return lfs_file_truncate(lfs, file, size);
    if (s_wr_h == h && vfs_flush_write(h) < 0)
        return LFS_ERR_IO;
    vfs_invalidate_read(h, 1);
    if (h->fs_type == VFS_FS_FAT) {
        if (f_size(&h->fil) == (FSIZE_t)size)
            return 0;
        if (f_lseek(&h->fil, (FSIZE_t)size) != FR_OK)
            return LFS_ERR_IO;
        return (f_truncate(&h->fil) == FR_OK) ? 0 : LFS_ERR_IO;
    }
    if (h->fs_type == VFS_FS_NTFS) {
        int rc = ntfs_truncate(&g_drives[h->slot].ntfs_vol, &h->nfile,
                               h->ntfs_dir_mft, h->ntfs_name, (uint64_t)size);
        if (rc == 0)
            h->dirty = 1;
        return (rc == 0) ? 0 : LFS_ERR_IO;
    }
    return LFS_ERR_INVAL;
}

lfs_soff_t obk_vfs_file_tell(lfs_t *lfs, lfs_file_t *file)
{
    struct vfs_file_handle *h = get_usb_file(file);
    if (!h)
        return lfs_file_tell(lfs, file);
    if (s_wr_h == h && vfs_flush_write(h) < 0)
        return LFS_ERR_IO;
    if (h->fs_type == VFS_FS_FAT) {
        if (s_rd_h == h)
            return (lfs_soff_t)(s_rd_off + s_rd_pos);
        return (lfs_soff_t)f_tell(&h->fil);
    }
    if (h->fs_type == VFS_FS_NTFS)
        return (lfs_soff_t)h->ntfs_pos;
    return LFS_ERR_IO;
}

int obk_vfs_file_rewind(lfs_t *lfs, lfs_file_t *file)
{
    struct vfs_file_handle *h = get_usb_file(file);
    if (!h)
        return lfs_file_rewind(lfs, file);
    return (obk_vfs_file_seek(lfs, file, 0, LFS_SEEK_SET) >= 0) ? 0 : LFS_ERR_IO;
}

lfs_soff_t obk_vfs_file_size(lfs_t *lfs, lfs_file_t *file)
{
    struct vfs_file_handle *h = get_usb_file(file);
    if (!h)
        return lfs_file_size(lfs, file);
    if (s_wr_h == h && vfs_flush_write(h) < 0)
        return LFS_ERR_IO;
    if (h->fs_type == VFS_FS_FAT)
        return (lfs_soff_t)f_size(&h->fil);
    if (h->fs_type == VFS_FS_NTFS)
        return (lfs_soff_t)h->nfile.size;
    return LFS_ERR_IO;
}

/* ----------------------- VFS Directory & Path Operations ----------------- */

int obk_vfs_remove(lfs_t *lfs, const char *path)
{
    int slot = -1;
    char sub[256];
    int pr = parse_usb_path(path, &slot, sub, sizeof(sub));
    if (pr == 0)
        return lfs_remove(lfs, path);
    if (pr < 0 || sub[0] == '\0')
        return LFS_ERR_INVAL;
    if (drv_ensure_writable(slot) < 0)
        return LFS_ERR_IO;
    if (g_drives[slot].fs_type == VFS_FS_FAT) {
        char fat_path[264];
        snprintf(fat_path, sizeof(fat_path), "%d:/%s", slot, sub);
        return (f_unlink(fat_path) == FR_OK) ? 0 : LFS_ERR_IO;
    }
    if (g_drives[slot].fs_type == VFS_FS_NTFS) {
        uint32_t dir_mft = NTFS_ROOT;
        char leaf[128];
        if (ntfs_resolve_parent(&g_drives[slot].ntfs_vol, sub, 0, &dir_mft, leaf, sizeof(leaf)) < 0)
            return LFS_ERR_NOENT;
        if (ntfs_remove(&g_drives[slot].ntfs_vol, dir_mft, leaf) < 0)
            return LFS_ERR_IO;
        msc_sync(&g_drives[slot].msc);
        return 0;
    }
    return LFS_ERR_INVAL;
}

int obk_vfs_rename(lfs_t *lfs, const char *oldpath, const char *newpath)
{
    int slot1 = -1, slot2 = -1;
    char sub1[256], sub2[256];
    int pr1 = parse_usb_path(oldpath, &slot1, sub1, sizeof(sub1));
    int pr2 = parse_usb_path(newpath, &slot2, sub2, sizeof(sub2));
    if (pr1 == 0 && pr2 == 0)
        return lfs_rename(lfs, oldpath, newpath);
    if (pr1 < 0 || pr2 < 0 || (pr1 > 0 && sub1[0] == '\0') || (pr2 > 0 && sub2[0] == '\0'))
        return LFS_ERR_INVAL;
    if (strcmp(oldpath, newpath) == 0)
        return 0;

    if (pr1 > 0 && pr2 > 0 && slot1 == slot2 && g_drives[slot1].fs_type == VFS_FS_FAT) {
        if (drv_ensure_writable(slot1) < 0)
            return LFS_ERR_IO;
        fat_mkdir_parents(slot2, sub2);
        char fat_old[264], fat_new[264];
        snprintf(fat_old, sizeof(fat_old), "%d:/%s", slot1, sub1);
        snprintf(fat_new, sizeof(fat_new), "%d:/%s", slot2, sub2);
        f_unlink(fat_new);
        return (f_rename(fat_old, fat_new) == FR_OK) ? 0 : LFS_ERR_IO;
    }

    /* Streamed move for NTFS or cross-volume renames */
    lfs_file_t src, dst;
    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));
    int rc = obk_vfs_file_open(lfs, &src, oldpath, LFS_O_RDONLY);
    if (rc < 0)
        return rc;
    rc = obk_vfs_file_open(lfs, &dst, newpath, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (rc < 0) {
        obk_vfs_file_close(lfs, &src);
        return rc;
    }
    uint8_t *tmp = (uint8_t *)malloc(4096);
    if (!tmp) {
        obk_vfs_file_close(lfs, &dst);
        obk_vfs_file_close(lfs, &src);
        return LFS_ERR_NOMEM;
    }
    int copy_err = 0;
    for (;;) {
        lfs_ssize_t n = obk_vfs_file_read(lfs, &src, tmp, 4096);
        if (n < 0) { copy_err = (int)n; break; }
        if (n == 0) break;
        if (obk_vfs_file_write(lfs, &dst, tmp, (lfs_size_t)n) != n) {
            copy_err = LFS_ERR_IO;
            break;
        }
    }
    free(tmp);
    if (obk_vfs_file_close(lfs, &dst) < 0 && !copy_err)
        copy_err = LFS_ERR_IO;
    obk_vfs_file_close(lfs, &src);
    if (copy_err < 0) {
        obk_vfs_remove(lfs, newpath);
        return copy_err;
    }
    return obk_vfs_remove(lfs, oldpath);
}

int obk_vfs_mkdir(lfs_t *lfs, const char *path)
{
    int slot = -1;
    char sub[256];
    int pr = parse_usb_path(path, &slot, sub, sizeof(sub));
    if (pr == 0)
        return lfs_mkdir(lfs, path);
    if (pr < 0)
        return LFS_ERR_NOENT;
    if (sub[0] == '\0')
        return 0; /* /usb0 root already exists */
    if (drv_ensure_writable(slot) < 0)
        return LFS_ERR_IO;
    if (g_drives[slot].fs_type == VFS_FS_FAT) {
        fat_mkdir_parents(slot, sub);
        char fat_path[264];
        snprintf(fat_path, sizeof(fat_path), "%d:/%s", slot, sub);
        FRESULT fr = f_mkdir(fat_path);
        return (fr == FR_OK || fr == FR_EXIST) ? 0 : LFS_ERR_IO;
    }
    if (g_drives[slot].fs_type == VFS_FS_NTFS) {
        uint32_t dir_mft = NTFS_ROOT;
        char leaf[128];
        if (ntfs_resolve_parent(&g_drives[slot].ntfs_vol, sub, 1, &dir_mft, leaf, sizeof(leaf)) < 0)
            return LFS_ERR_IO;
        uint32_t mft = 0;
        int rc = ntfs_create(&g_drives[slot].ntfs_vol, dir_mft, leaf, 1, &mft);
        if (rc == 0 || rc == NTFS_E_EXIST) {
            msc_sync(&g_drives[slot].msc);
            return 0;
        }
        return LFS_ERR_IO;
    }
    return LFS_ERR_INVAL;
}

int obk_vfs_stat(lfs_t *lfs, const char *path, struct lfs_info *info)
{
    int slot = -1;
    char sub[256];
    int pr = parse_usb_path(path, &slot, sub, sizeof(sub));
    if (pr == 0)
        return lfs_stat(lfs, path, info);
    if (pr < 0)
        return LFS_ERR_NOENT;

    memset(info, 0, sizeof(*info));
    if (sub[0] == '\0') {
        info->type = LFS_TYPE_DIR;
        snprintf(info->name, sizeof(info->name), "usb%d", slot);
        return 0;
    }
    if (g_drives[slot].fs_type == VFS_FS_FAT) {
        char fat_path[264];
        FILINFO fno;
        snprintf(fat_path, sizeof(fat_path), "%d:/%s", slot, sub);
        if (f_stat(fat_path, &fno) != FR_OK)
            return LFS_ERR_NOENT;
        info->type = (fno.fattrib & AM_DIR) ? LFS_TYPE_DIR : LFS_TYPE_REG;
        info->size = (lfs_size_t)fno.fsize;
        strncpy(info->name, fno.fname, LFS_NAME_MAX);
        return 0;
    }
    if (g_drives[slot].fs_type == VFS_FS_NTFS) {
        uint32_t mft = 0, attr = 0;
        uint64_t fsize = 0;
        int rc = ntfs_resolve_path(&g_drives[slot].ntfs_vol, sub, &mft, &attr, &fsize);
        if (rc < 0)
            return rc;
        info->type = (attr & NTFS_ATTR_DIRECTORY) ? LFS_TYPE_DIR : LFS_TYPE_REG;
        info->size = (lfs_size_t)fsize;
        return 0;
    }
    return LFS_ERR_IO;
}

int obk_vfs_dir_open(lfs_t *lfs, lfs_dir_t *dir, const char *path)
{
    int slot = -1;
    char sub[256];
    int pr = parse_usb_path(path, &slot, sub, sizeof(sub));
    if (pr == 0) {
        int rc = lfs_dir_open(lfs, dir, path);
        if (rc >= 0) {
            const char *p = path ? path : "";
            while (*p == '/')
                p++;
            if (*p == '\0') {
                g_root_dir_ptr = dir;
                g_root_usb_idx = 0;
            }
        }
        return rc;
    }
    if (pr < 0)
        return LFS_ERR_NOENT;

    int d_idx = -1;
    for (int i = 0; i < VFS_MAX_DIRS; i++) {
        if (!g_dirs[i].in_use) {
            d_idx = i;
            break;
        }
    }
    if (d_idx < 0)
        return LFS_ERR_NOMEM;

    struct vfs_dir_handle *d = &g_dirs[d_idx];
    memset(d, 0, sizeof(*d));
    struct vfs_drive *drv = &g_drives[slot];

    if (drv->fs_type == VFS_FS_FAT) {
        char fat_path[264];
        if (sub[0])
            snprintf(fat_path, sizeof(fat_path), "%d:/%s", slot, sub);
        else
            snprintf(fat_path, sizeof(fat_path), "%d:", slot);

        if (f_opendir(&d->fdir, fat_path) != FR_OK)
            return LFS_ERR_NOENT;
        d->in_use  = 1;
        d->slot    = slot;
        d->fs_type = VFS_FS_FAT;
        d->step    = 0;
        dir->pos   = VFS_HANDLE_MAGIC | (uint32_t)d_idx;
        return 0;
    }

    if (drv->fs_type == VFS_FS_NTFS) {
        uint32_t mft = NTFS_ROOT, attr = NTFS_ATTR_DIRECTORY;
        uint64_t fsize = 0;
        if (sub[0]) {
            int rc = ntfs_resolve_path(&drv->ntfs_vol, sub, &mft, &attr, &fsize);
            if (rc < 0)
                return rc;
            if (!(attr & NTFS_ATTR_DIRECTORY))
                return LFS_ERR_NOTDIR;
        }
        d->in_use   = 1;
        d->slot     = slot;
        d->fs_type  = VFS_FS_NTFS;
        d->ntfs_mft = mft;
        d->step     = 0;
        dir->pos    = VFS_HANDLE_MAGIC | (uint32_t)d_idx;
        return 0;
    }

    return LFS_ERR_IO;
}

static struct vfs_dir_handle *get_usb_dir(lfs_dir_t *dir)
{
    if (!dir || (dir->pos & VFS_HANDLE_MASK) != VFS_HANDLE_MAGIC)
        return 0;
    uint32_t idx = dir->pos & ~VFS_HANDLE_MASK;
    if (idx >= VFS_MAX_DIRS || !g_dirs[idx].in_use)
        return 0;
    return &g_dirs[idx];
}

int obk_vfs_dir_read(lfs_t *lfs, lfs_dir_t *dir, struct lfs_info *info)
{
    struct vfs_dir_handle *d = get_usb_dir(dir);
    if (!d) {
        if (dir == g_root_dir_ptr && g_root_usb_idx > 0)
            goto emit_root_usb;
        int rc = lfs_dir_read(lfs, dir, info);
        if (rc > 0 || dir != g_root_dir_ptr)
            return rc;
emit_root_usb:
        while (g_root_usb_idx < VFS_MAX_DRIVES) {
            int slot = g_root_usb_idx++;
            if (g_drives[slot].fs_type != VFS_FS_NONE) {
                memset(info, 0, sizeof(*info));
                info->type = LFS_TYPE_DIR;
                info->size = 0;
                snprintf(info->name, sizeof(info->name), "usb%d", slot);
                return 1;
            }
        }
        return 0;
    }

    memset(info, 0, sizeof(*info));
    if (d->step == 0) {
        d->step = 1;
        info->type = LFS_TYPE_DIR;
        strcpy(info->name, ".");
        return 1;
    }
    if (d->step == 1) {
        d->step = 2;
        info->type = LFS_TYPE_DIR;
        strcpy(info->name, "..");
        return 1;
    }

    if (d->fs_type == VFS_FS_FAT) {
        FILINFO fno;
        if (f_readdir(&d->fdir, &fno) != FR_OK || fno.fname[0] == 0)
            return 0;
        info->type = (fno.fattrib & AM_DIR) ? LFS_TYPE_DIR : LFS_TYPE_REG;
        info->size = (fno.fattrib & AM_DIR) ? 0 : (lfs_size_t)fno.fsize;
        strncpy(info->name, fno.fname, LFS_NAME_MAX);
        info->name[LFS_NAME_MAX] = 0;
        d->step++;
        return 1;
    }

    if (d->fs_type == VFS_FS_NTFS) {
        struct ntfs_nth_ctx ctx;
        ctx.target = d->step - 2;
        ctx.cur    = 0;
        ctx.info   = info;
        ctx.found  = 0;
        if (ntfs_list(&g_drives[d->slot].ntfs_vol, d->ntfs_mft, ntfs_nth_cb, &ctx) < 0)
            return LFS_ERR_IO;
        if (!ctx.found)
            return 0;
        d->step++;
        return 1;
    }

    return 0;
}

int obk_vfs_dir_close(lfs_t *lfs, lfs_dir_t *dir)
{
    struct vfs_dir_handle *d = get_usb_dir(dir);
    if (!d) {
        if (dir == g_root_dir_ptr) {
            g_root_dir_ptr = 0;
            g_root_usb_idx = 0;
        }
        return lfs_dir_close(lfs, dir);
    }
    if (d->fs_type == VFS_FS_FAT)
        f_closedir(&d->fdir);
    d->in_use = 0;
    dir->pos  = 0;
    return 0;
}

/* --------------------- USB Hub & Multi-Drive Hotplug --------------------- */

static uint8_t g_mount_tries[16];

static int has_unmounted_msc_candidate(void)
{
    int ndevs = usb_dev_count();
    for (int i = 0; i < ndevs; i++) {
        const struct usb_dev *d = usb_get_dev(i);
        if (!d || !d->present || d->is_hub || d->addr >= 16)
            continue;
        int mounted = 0;
        for (int s = 0; s < VFS_MAX_DRIVES; s++) {
            if (g_drives[s].fs_type != VFS_FS_NONE && g_drives[s].usb_addr == d->addr) {
                mounted = 1;
                break;
            }
        }
        if (!mounted && g_mount_tries[d->addr] < 3)
            return 1;
    }
    return 0;
}

static void sync_usb_drives(void)
{
    int ndevs = usb_dev_count();

    /* First unmount any drive whose USB device was unplugged */
    for (int slot = 0; slot < VFS_MAX_DRIVES; slot++) {
        if (g_drives[slot].fs_type == VFS_FS_NONE)
            continue;
        const struct usb_dev *d = usb_dev_by_addr(g_drives[slot].usb_addr);
        if (!d || !d->present) {
            ADDLOG_INFO(LOG_FEATURE_MAIN, "USB drive /usb%d (addr %d) disconnected",
                        slot, g_drives[slot].usb_addr);
            if (g_drives[slot].fs_type == VFS_FS_FAT) {
                char drv_path[4] = { (char)('0' + slot), ':', 0, 0 };
                f_mount(0, drv_path, 0);
            }
            if (g_drives[slot].usb_addr < 16)
                g_mount_tries[g_drives[slot].usb_addr] = 0;
            diskio_usb_attach_slot(slot, 0);
            memset(&g_drives[slot], 0, sizeof(g_drives[slot]));
        }
    }

    /* Mount any newly enumerated USB mass storage devices */
    for (int i = 0; i < ndevs; i++) {
        const struct usb_dev *d = usb_get_dev(i);
        if (!d || !d->present || d->is_hub)
            continue;

        int already = 0;
        for (int s = 0; s < VFS_MAX_DRIVES; s++) {
            if (g_drives[s].fs_type != VFS_FS_NONE && g_drives[s].usb_addr == d->addr) {
                already = 1;
                break;
            }
        }
        if (already)
            continue;
        if (d->addr < 16 && g_mount_tries[d->addr] >= 3)
            continue;

        int slot = -1;
        for (int s = 0; s < VFS_MAX_DRIVES; s++) {
            if (g_drives[s].fs_type == VFS_FS_NONE) {
                slot = s;
                break;
            }
        }
        if (slot < 0)
            break;

        if (d->addr < 16)
            g_mount_tries[d->addr]++;

        struct vfs_drive *drv = &g_drives[slot];
        memset(drv, 0, sizeof(*drv));
        int mrc = msc_init_dev(&drv->msc, d);
        if (mrc < 0) {
            ADDLOG_INFO(LOG_FEATURE_MAIN,
                        "USB addr %d (%04x:%04x) msc_init_dev rc=%d (try %u/3)",
                        d->addr, d->vid, d->pid, mrc,
                        d->addr < 16 ? g_mount_tries[d->addr] : 1);
            continue;
        }

        drv->usb_addr = d->addr;
        drv->hub_addr = d->hub_addr;
        drv->hub_port = d->hub_port;
        drv->size_mb  = (uint32_t)(((uint64_t)drv->msc.blocks * drv->msc.block_size) >> 20);

        diskio_usb_attach_slot(slot, &drv->msc);
        char drv_path[4] = { (char)('0' + slot), ':', 0, 0 };
        FRESULT fr = f_mount(&drv->fat_vol, drv_path, 1);
        for (int mt = 0; mt < 4 && fr == FR_DISK_ERR; mt++) {
            delay_ms(200);
            fr = f_mount(&drv->fat_vol, drv_path, 1);
        }
        if (fr == FR_OK) {
            static const char *const fst[] = { "FAT", "FAT12", "FAT16", "FAT32", "exFAT" };
            DWORD vsn = 0;
            f_getlabel(drv_path, drv->label, &vsn);
            strcpy(drv->fs_name, fst[drv->fat_vol.fs_type <= 4 ? drv->fat_vol.fs_type : 0]);
            drv->fs_type = VFS_FS_FAT;
            ADDLOG_INFO(LOG_FEATURE_MAIN,
                        "Mounted /usb%d: %s \"%s\" (%u MB, %s %s, USB addr %d hub %u port %u)",
                        slot, drv->fs_name, drv->label, drv->size_mb,
                        drv->msc.vendor, drv->msc.product,
                        d->addr, d->hub_addr, d->hub_port);
        } else {
            f_mount(0, drv_path, 0);
            if (ntfs_mount(&drv->ntfs_vol, msc_ntfs_reader, &drv->msc) == 0) {
                ntfs_set_writer(&drv->ntfs_vol, msc_ntfs_writer);
                strncpy(drv->label, drv->ntfs_vol.label, sizeof(drv->label) - 1);
                strcpy(drv->fs_name, "NTFS");
                drv->fs_type = VFS_FS_NTFS;
                ADDLOG_INFO(LOG_FEATURE_MAIN,
                            "Mounted /usb%d: NTFS \"%s\" (%u MB, %s %s, USB addr %d hub %u port %u)",
                            slot, drv->label, drv->size_mb,
                            drv->msc.vendor, drv->msc.product,
                            d->addr, d->hub_addr, d->hub_port);
            } else {
                ADDLOG_INFO(LOG_FEATURE_MAIN,
                            "USB addr %d (%04x:%04x) filesystem mount failed (f_mount=%d)",
                            d->addr, d->vid, d->pid, (int)fr);
                diskio_usb_attach_slot(slot, 0);
                memset(drv, 0, sizeof(*drv));
            }
        }
    }
}

static uint8_t s_bench_buf[4096] __attribute__((aligned(32)));

static void run_usb_benchmark(void)
{
    for (int i = 0; i < (int)sizeof(s_bench_buf); i++)
        s_bench_buf[i] = (uint8_t)(i * 31u + 7u);

    for (int s = 0; s < VFS_MAX_DRIVES; s++) {
        struct vfs_drive *drv = &g_drives[s];
        if (drv->fs_type == VFS_FS_NONE)
            continue;

        /* Ensure writable state is already probed before timing */
        drv_ensure_writable(s);
        vfs_invalidate_read(0, 0);

        /* 1. Raw MSC 512 KB read (8 x 64 KB = 1024 sectors) */
        uint32_t t0 = timer_ms();
        int raw_ok = 1;
        for (int k = 0; k < 8; k++) {
            if (msc_read(&drv->msc, 2048u + (uint32_t)k * 128u, 128u, s_rd_buf) != 0) {
                raw_ok = 0;
                break;
            }
        }
        uint32_t dt_raw = timer_ms() - t0;
        uint32_t raw_kbs = (raw_ok && dt_raw > 0) ? (512u * 1000u / dt_raw) : 0;

        /* 2. VFS 256 KB file write (64 x 4 KB chunks) if writable */
        uint32_t wr_kbs = 0, dt_wr = 0;
        char bpath[64];
        snprintf(bpath, sizeof(bpath), "/usb%d/bench.bin", s);
        if (drv->write_state > 0) {
            lfs_file_t wf;
            memset(&wf, 0, sizeof(wf));
            uint32_t tw0 = timer_ms();
            if (obk_vfs_file_open(&lfs, &wf, bpath, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) == 0) {
                int wok = 1;
                for (int k = 0; k < 64; k++) {
                    if (obk_vfs_file_write(&lfs, &wf, s_bench_buf, 4096) != 4096) {
                        wok = 0;
                        break;
                    }
                }
                obk_vfs_file_close(&lfs, &wf);
                dt_wr = timer_ms() - tw0;
                if (wok && dt_wr > 0)
                    wr_kbs = (256u * 1000u) / dt_wr;
            }
        }

        /* 3. VFS file read (up to 256 KB in 4 KB chunks) */
        uint32_t rd_kbs = 0, dt_rd = 0, rd_bytes = 0;
        char rpath[160];
        snprintf(rpath, sizeof(rpath), "/usb%d/bench.bin", s);
        if (drv->write_state < 0) {
            /* Pick largest root file on RO drive */
            lfs_dir_t d;
            struct lfs_info info;
            char rootp[16];
            snprintf(rootp, sizeof(rootp), "/usb%d", s);
            uint32_t best_sz = 0;
            if (obk_vfs_dir_open(&lfs, &d, rootp) == 0) {
                while (obk_vfs_dir_read(&lfs, &d, &info) > 0) {
                    if (info.type == LFS_TYPE_REG && info.size > best_sz) {
                        best_sz = info.size;
                        snprintf(rpath, sizeof(rpath), "/usb%d/%s", s, info.name);
                    }
                }
                obk_vfs_dir_close(&lfs, &d);
            }
        }
        {
            lfs_file_t rf;
            memset(&rf, 0, sizeof(rf));
            if (obk_vfs_file_open(&lfs, &rf, rpath, LFS_O_RDONLY) == 0) {
                uint32_t tr0 = timer_ms();
                while (rd_bytes < 262144u) {
                    lfs_ssize_t n = obk_vfs_file_read(&lfs, &rf, s_bench_buf, 4096);
                    if (n <= 0)
                        break;
                    rd_bytes += (uint32_t)n;
                }
                dt_rd = timer_ms() - tr0;
                obk_vfs_file_close(&lfs, &rf);
                if (rd_bytes > 0 && dt_rd > 0)
                    rd_kbs = (rd_bytes * 1000u / 1024u) / dt_rd;
            }
        }
        if (drv->write_state > 0)
            obk_vfs_remove(&lfs, bpath);

        ADDLOG_INFO(LOG_FEATURE_MAIN,
                    "BENCH /usb%d (%s \"%s\"): raw_rd=%u KB/s (%u ms), vfs_rd=%u KB/s (%u KB/%u ms), vfs_wr=%u KB/s (256 KB/%u ms)",
                    s, drv->fs_name, drv->label,
                    raw_kbs, dt_raw, rd_kbs, rd_bytes >> 10, dt_rd, wr_kbs, dt_wr);
    }
}

extern void net_poll(void);
extern void ota_poll(void);
extern size_t xPortGetFreeHeapSize(void);

static int  s_prof_pending = 0;
static int  s_prof_mb      = 10;
static int  s_prof_iters   = 3;
static int  s_prof_slot    = -1;
static int  s_prof_keep    = 0;
static char s_prof_report[3584];
static int  s_prof_report_len = 0;

static void prof_append(const char *fmt, ...)
{
    if (s_prof_report_len >= (int)sizeof(s_prof_report) - 128)
        return;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(s_prof_report + s_prof_report_len,
                      sizeof(s_prof_report) - (size_t)s_prof_report_len, fmt, ap);
    va_end(ap);
    if (n > 0)
        s_prof_report_len += n;
}

typedef uint32_t (*kseg0_fill_crc_fn)(uint8_t *cbuf, uint32_t chunk_sz,
                                      uint32_t c, uint32_t num_chunks,
                                      uint32_t it, uint32_t crc);
typedef uint32_t (*kseg0_crc_fn)(const uint8_t *cbuf, uint32_t chunk_sz, uint32_t crc);

static uint32_t kseg0_fill_and_crc_impl(uint8_t *cbuf, uint32_t chunk_sz,
                                        uint32_t c, uint32_t num_chunks,
                                        uint32_t it, uint32_t crc)
{
    uint32_t *w = (uint32_t *)cbuf;
    uint32_t x = 0x9E3779B9u ^ (c * 0x85EBCA6Bu) ^ (it * 0xC2B2AE35u);
    uint32_t words = chunk_sz >> 2;
    for (uint32_t k = 0; k < words; k++) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        w[k] = x;
    }
    if (c == 0) {
        w[0] = 0xFFD8FFE0u;
        w[1] = 0x00104A46u;
        w[2] = 0x49460001u;
        w[3] = 0x01010060u;
        w[4] = 0x00600000u;
    }
    if (c == num_chunks - 1u) {
        cbuf[chunk_sz - 2u] = 0xFF;
        cbuf[chunk_sz - 1u] = 0xD9;
    }
    for (uint32_t k = 0; k < words; k++)
        crc = ((crc << 5) + crc) ^ w[k];
    return crc;
}

static uint32_t kseg0_crc_impl(const uint8_t *cbuf, uint32_t chunk_sz, uint32_t crc)
{
    const uint32_t *w = (const uint32_t *)cbuf;
    uint32_t words = chunk_sz >> 2;
    for (uint32_t k = 0; k < words; k++)
        crc = ((crc << 5) + crc) ^ w[k];
    return crc;
}

static void run_vfs_profile(int mb, int iters, int target_slot, int keep_file)
{
    if (mb < 1) mb = 1;
    if (mb > 64) mb = 64;
    if (iters < 1) iters = 1;
    if (iters > 20) iters = 20;

    uint32_t total_bytes = (uint32_t)mb * 1024u * 1024u;
    uint32_t chunk_sz    = 16384u;
    uint32_t num_chunks  = total_bytes / chunk_sz;

    s_prof_report_len = 0;
    s_prof_report[0]  = 0;

    uint32_t heap_before = (uint32_t)xPortGetFreeHeapSize();
    uint8_t *buf = (uint8_t *)malloc(chunk_sz);
    if (!buf) {
        ADDLOG_ERROR(LOG_FEATURE_MAIN, "PROFILE: failed to allocate 16 KB stream buffer");
        return;
    }
    uint8_t *cbuf = (uint8_t *)((uintptr_t)buf & ~0x20000000u);
    kseg0_fill_crc_fn fill_fn = (kseg0_fill_crc_fn)((uintptr_t)kseg0_fill_and_crc_impl & ~0x20000000u);
    kseg0_crc_fn      crc_fn  = (kseg0_crc_fn)((uintptr_t)kseg0_crc_impl & ~0x20000000u);

    uint32_t heap_during = (uint32_t)xPortGetFreeHeapSize();

    prof_append("=== VFS %d MB JPEG STREAMING PROFILE (%d iter(s), 16 KB chunk + 64 KB VFS window) ===\n",
                mb, iters);
    prof_append("Heap free: before=%u B, during_stream=%u B (dynamic RAM used=%u B for %u KB file)\n",
                heap_before, heap_during, heap_before - heap_during, total_bytes >> 10);

    for (int s = 0; s < VFS_MAX_DRIVES; s++) {
        if (target_slot >= 0 && s != target_slot)
            continue;
        struct vfs_drive *drv = &g_drives[s];
        if (drv->fs_type == VFS_FS_NONE)
            continue;
        drv_ensure_writable(s);

        if (drv->write_state < 0) {
            /* Hardware read-only drive (e.g. /usb2 SLITAZ CORE): profile 10 MB VFS streaming READ */
            char rpath[160];
            rpath[0] = 0;
            lfs_dir_t d;
            struct lfs_info info;
            char rootp[16];
            snprintf(rootp, sizeof(rootp), "/usb%d", s);
            uint32_t best_sz = 0;
            if (obk_vfs_dir_open(&lfs, &d, rootp) == 0) {
                while (obk_vfs_dir_read(&lfs, &d, &info) > 0) {
                    if (info.type == LFS_TYPE_REG && info.size > best_sz) {
                        best_sz = info.size;
                        snprintf(rpath, sizeof(rpath), "/usb%d/%s", s, info.name);
                    }
                }
                obk_vfs_dir_close(&lfs, &d);
            }
            if (rpath[0]) {
                uint32_t sum_rd_kbs = 0;
                for (int it = 1; it <= iters; it++) {
                    struct usb_bulk_stats u1 = usb_bulk_stats;
                    uint32_t t1 = timer_ms();
                    uint32_t rd_bytes = 0, rd_crc = 5381u;
                    while (rd_bytes < total_bytes) {
                        lfs_file_t f;
                        memset(&f, 0, sizeof(f));
                        if (obk_vfs_file_open(&lfs, &f, rpath, LFS_O_RDONLY) < 0)
                            break;
                        while (rd_bytes < total_bytes) {
                            uint32_t ask = (total_bytes - rd_bytes) < chunk_sz ? (total_bytes - rd_bytes) : chunk_sz;
                            int rres = (int)obk_vfs_file_read(&lfs, &f, buf, ask);
                            if (rres <= 0)
                                break;
                            rd_crc = crc_fn(cbuf, (uint32_t)rres & ~3u, rd_crc);
                            rd_bytes += (uint32_t)rres;
                        }
                        obk_vfs_file_close(&lfs, &f);
                        net_poll();
                        ota_poll();
                    }
                    uint32_t rd_ms = timer_ms() - t1;
                    struct usb_bulk_stats u2 = usb_bulk_stats;
                    uint32_t rd_usb_ms  = (u2.us_xfer - u1.us_xfer) / 1000u;
                    uint32_t rd_copy_ms = (u2.us_copy - u1.us_copy) / 1000u;
                    uint32_t rd_fs_ms   = (rd_ms > rd_usb_ms + rd_copy_ms) ? (rd_ms - rd_usb_ms - rd_copy_ms) : 0;
                    uint32_t rd_xfers   = u2.transfers - u1.transfers;
                    uint32_t rd_kbs     = rd_ms ? ((rd_bytes >> 10) * 1000u) / rd_ms : 0;
                    sum_rd_kbs += rd_kbs;
                    prof_append("/usb%d (%s \"%s\" RO-HW) iter %d/%d [%u MB]: RD=%u KB/s (%u ms: bus=%u ms copy=%u ms fs=%u ms xfers=%u) | OK crc=%08X\n",
                                s, drv->fs_name, drv->label, it, iters, mb,
                                rd_kbs, rd_ms, rd_usb_ms, rd_copy_ms, rd_fs_ms, rd_xfers, rd_crc);
                }
                prof_append("  -> SUMMARY /usb%d (%s RO-HW): %d/%d PASS | RD avg=%u KB/s\n",
                            s, drv->fs_name, iters, iters, sum_rd_kbs / (uint32_t)iters);
            }
            continue;
        }

        char path[48];
        snprintf(path, sizeof(path), "usb%d/prof%dm.jpg", s, mb);

        uint32_t sum_wr_kbs = 0, sum_rd_kbs = 0;
        uint32_t min_wr_kbs = 0xFFFFFFFFu, max_wr_kbs = 0;
        uint32_t min_rd_kbs = 0xFFFFFFFFu, max_rd_kbs = 0;
        int      pass_cnt   = 0;

        for (int it = 1; it <= iters; it++) {
            lfs_file_t f;
            memset(&f, 0, sizeof(f));

            /* --- 1. STREAM WRITE (mb MB JPEG) --- */
            struct usb_bulk_stats u0 = usb_bulk_stats;
            uint32_t t0 = timer_ms();
            uint32_t wr_crc = 5381u;
            uint32_t wr_bytes = 0;

            if (obk_vfs_file_open(&lfs, &f, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) >= 0) {
                for (uint32_t c = 0; c < num_chunks; c++) {
                    wr_crc = fill_fn(cbuf, chunk_sz, c, num_chunks, (uint32_t)it, wr_crc);
                    cctl_write(CCTL_DWB);

                    int wres = (int)obk_vfs_file_write(&lfs, &f, buf, chunk_sz);
                    if (wres != (int)chunk_sz)
                        break;
                    wr_bytes += (uint32_t)wres;

                    if ((c & 15u) == 15u) {
                        net_poll();
                        ota_poll();
                    }
                }
                obk_vfs_file_close(&lfs, &f);
            }
            uint32_t wr_ms = timer_ms() - t0;
            struct usb_bulk_stats u1 = usb_bulk_stats;
            uint32_t wr_usb_ms  = (u1.us_xfer - u0.us_xfer) / 1000u;
            uint32_t wr_copy_ms = (u1.us_copy - u0.us_copy) / 1000u;
            uint32_t wr_fs_ms   = (wr_ms > wr_usb_ms + wr_copy_ms) ? (wr_ms - wr_usb_ms - wr_copy_ms) : 0;
            uint32_t wr_xfers   = u1.transfers - u0.transfers;
            uint32_t wr_kbs     = wr_ms ? ((wr_bytes >> 10) * 1000u) / wr_ms : 0;

            /* --- 2. STREAM READ-BACK & VERIFY (mb MB JPEG) --- */
            memset(&f, 0, sizeof(f));
            uint32_t t1 = timer_ms();
            uint32_t rd_crc = 5381u;
            uint32_t rd_bytes = 0;
            int      soi_ok = 0, eoi_ok = 0;

            if (obk_vfs_file_open(&lfs, &f, path, LFS_O_RDONLY) >= 0) {
                for (uint32_t c = 0; c < num_chunks; c++) {
                    int rres = (int)obk_vfs_file_read(&lfs, &f, buf, chunk_sz);
                    if (rres != (int)chunk_sz)
                        break;
                    if (c == 0 && buf[0] == 0xFF && buf[1] == 0xD8 &&
                        buf[6] == 'J' && buf[7] == 'F' && buf[8] == 'I' && buf[9] == 'F')
                        soi_ok = 1;
                    if (c == num_chunks - 1u &&
                        buf[chunk_sz - 2u] == 0xFF && buf[chunk_sz - 1u] == 0xD9)
                        eoi_ok = 1;
                    rd_crc = crc_fn(cbuf, chunk_sz, rd_crc);
                    rd_bytes += (uint32_t)rres;
                    if ((c & 15u) == 15u) {
                        net_poll();
                        ota_poll();
                    }
                }
                obk_vfs_file_close(&lfs, &f);
            }
            uint32_t rd_ms = timer_ms() - t1;
            struct usb_bulk_stats u2 = usb_bulk_stats;
            uint32_t rd_usb_ms  = (u2.us_xfer - u1.us_xfer) / 1000u;
            uint32_t rd_copy_ms = (u2.us_copy - u1.us_copy) / 1000u;
            uint32_t rd_fs_ms   = (rd_ms > rd_usb_ms + rd_copy_ms) ? (rd_ms - rd_usb_ms - rd_copy_ms) : 0;
            uint32_t rd_xfers   = u2.transfers - u1.transfers;
            uint32_t rd_kbs     = rd_ms ? ((rd_bytes >> 10) * 1000u) / rd_ms : 0;

            int ok = (wr_bytes == total_bytes && rd_bytes == total_bytes &&
                      wr_crc == rd_crc && soi_ok && eoi_ok);
            if (ok) {
                pass_cnt++;
                sum_wr_kbs += wr_kbs;
                sum_rd_kbs += rd_kbs;
                if (wr_kbs < min_wr_kbs) min_wr_kbs = wr_kbs;
                if (wr_kbs > max_wr_kbs) max_wr_kbs = wr_kbs;
                if (rd_kbs < min_rd_kbs) min_rd_kbs = rd_kbs;
                if (rd_kbs > max_rd_kbs) max_rd_kbs = rd_kbs;
            }

            if (!(keep_file && it == iters))
                obk_vfs_remove(&lfs, path);

            ADDLOG_INFO(LOG_FEATURE_MAIN,
                        "PROFILE /usb%d (%s) #%d/%d [%u MB JPG]: WR=%u KB/s (%u ms: usb=%u copy=%u fs=%u xfers=%u) | RD=%u KB/s (%u ms: usb=%u copy=%u fs=%u xfers=%u) | %s (%08X)",
                        s, drv->fs_name, it, iters, mb,
                        wr_kbs, wr_ms, wr_usb_ms, wr_copy_ms, wr_fs_ms, wr_xfers,
                        rd_kbs, rd_ms, rd_usb_ms, rd_copy_ms, rd_fs_ms, rd_xfers,
                        ok ? "VERIFY_OK" : "FAIL", rd_crc);

            prof_append("/usb%d (%s \"%s\") iter %d/%d [%u MB]: WR=%u KB/s (%u ms: bus=%u ms copy=%u ms fs=%u ms xfers=%u) | RD=%u KB/s (%u ms: bus=%u ms copy=%u ms fs=%u ms xfers=%u) | %s crc=%08X\n",
                        s, drv->fs_name, drv->label, it, iters, mb,
                        wr_kbs, wr_ms, wr_usb_ms, wr_copy_ms, wr_fs_ms, wr_xfers,
                        rd_kbs, rd_ms, rd_usb_ms, rd_copy_ms, rd_fs_ms, rd_xfers,
                        ok ? "OK" : "FAIL", rd_crc);
        }

        if (pass_cnt > 0) {
            prof_append("  -> SUMMARY /usb%d (%s): %d/%d PASS | WR avg=%u KB/s (min=%u max=%u) | RD avg=%u KB/s (min=%u max=%u)\n",
                        s, drv->fs_name, pass_cnt, iters,
                        sum_wr_kbs / (uint32_t)pass_cnt, min_wr_kbs, max_wr_kbs,
                        sum_rd_kbs / (uint32_t)pass_cnt, min_rd_kbs, max_rd_kbs);
        }
    }

    free(buf);
    uint32_t heap_after = (uint32_t)xPortGetFreeHeapSize();
    prof_append("Heap free after cleanup: %u B (net leak=%d B) | msc_err=%u stalls=%u resets=%u\n",
                heap_after, (int)heap_before - (int)heap_after,
                msc_stats.bulk_errors, msc_stats.stalls, msc_stats.reset_recoveries);
    ADDLOG_INFO(LOG_FEATURE_MAIN,
                "PROFILE DONE: heap before=%u during=%u after=%u (leak=%d B)",
                heap_before, heap_during, heap_after, (int)heap_before - (int)heap_after);

    /* Save full report to LittleFS /prof_report.txt so it can be fetched via /api/lfs/prof_report.txt */
    lfs_file_t rf;
    memset(&rf, 0, sizeof(rf));
    if (obk_vfs_file_open(&lfs, &rf, "prof_report.txt", LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) >= 0) {
        obk_vfs_file_write(&lfs, &rf, s_prof_report, (lfs_size_t)s_prof_report_len);
        obk_vfs_file_close(&lfs, &rf);
    }
}

static commandResult_t CMD_VFS_Profile(const void *context, const char *cmd, const char *args, int cmdFlags)
{
    int mb = 10, iters = 3, slot = -1, keep = 0;
    if (args && *args) {
        while (*args == ' ') args++;
        if (*args) {
            mb = atoi(args);
            while (*args && *args != ' ') args++;
            while (*args == ' ') args++;
            if (*args) {
                iters = atoi(args);
                while (*args && *args != ' ') args++;
                while (*args == ' ') args++;
                if (*args) {
                    slot = atoi(args);
                    while (*args && *args != ' ') args++;
                    while (*args == ' ') args++;
                    if (*args)
                        keep = atoi(args);
                }
            }
        }
    }
    s_prof_mb      = mb;
    s_prof_iters   = iters;
    s_prof_slot    = slot;
    s_prof_keep    = keep;
    s_prof_pending = 1;
    ADDLOG_INFO(LOG_FEATURE_MAIN,
                "Queued VFS profile: %d MB x %d iter(s) on slot=%d (keep=%d)",
                mb, iters, slot, keep);
    return CMD_RES_OK;
}

static commandResult_t CMD_USB_Status(const void *context, const char *cmd, const char *args, int cmdFlags)
{
    int ndevs = usb_dev_count();
    ADDLOG_INFO(LOG_FEATURE_MAIN, "USB Topology: %d device(s) enumerated", ndevs);
    for (int i = 0; i < ndevs; i++) {
        const struct usb_dev *d = usb_get_dev(i);
        if (!d || !d->present)
            continue;
        ADDLOG_INFO(LOG_FEATURE_MAIN,
                    "  [addr %d, hub %u port %u] %04x:%04x %s (%s)",
                    d->addr, d->hub_addr, d->hub_port, d->vid, d->pid,
                    d->product[0] ? d->product : usb_class_name(d->dev_desc[4]),
                    usb_speed_name(d->speed));
    }
    for (int s = 0; s < VFS_MAX_DRIVES; s++) {
        if (g_drives[s].fs_type != VFS_FS_NONE) {
            const char *rw = (g_drives[s].write_state < 0) ? "RO-HW" :
                             (g_drives[s].write_state > 0) ? "RW-OK" : "RW";
            ADDLOG_INFO(LOG_FEATURE_MAIN,
                        "  /usb%d -> %s label=\"%s\" %u MB (addr %d, %s)",
                        s, g_drives[s].fs_name, g_drives[s].label,
                        g_drives[s].size_mb, g_drives[s].usb_addr, rw);
        }
    }
    ADDLOG_INFO(LOG_FEATURE_MAIN,
                "  msc_stats: cmds=%u retries=%u fail_st=%u resets=%u stalls=%u bulk_err=%u",
                msc_stats.commands, msc_stats.retries, msc_stats.failed_status,
                msc_stats.reset_recoveries, msc_stats.stalls, msc_stats.bulk_errors);
    if (args && strncmp(args, "bench", 5) == 0)
        run_usb_benchmark();
    return CMD_RES_OK;
}

static int      g_usb_ready = 0;
static uint32_t g_last_usb_poll_ms = 0;

void rtl8196c_usb_vfs_init(void)
{
    if (!lfs_present())
        init_lfs(1);

    CMD_RegisterCommand("usb_status", CMD_USB_Status, NULL);
    CMD_RegisterCommand("vfs_profile", CMD_VFS_Profile, NULL);

    if (usb_host_init() == 0) {
        delay_ms(500); /* allow VBUS bulk caps on hub/drives to fully discharge */
        usb_vbus(1);
        delay_ms(400); /* allow root hub/drive to power up and assert D+ */
        g_usb_ready = 1;

        int ev = usb_poll();
        if (ev == USB_EV_ENUM_FAILED) {
            usb_vbus(0);
            delay_ms(800);
            usb_vbus(1);
            delay_ms(400);
            ev = usb_poll();
        }
        if (ev == USB_EV_CONNECTED) {
            if (usb_dev()->is_hub) {
                delay_ms(600); /* allow 2nd-tier cascaded hub ports to finish spin-up */
                usb_hub_scan_all();
                delay_ms(300); /* allow newly enumerated 2nd-tier drives to settle */
            }
            sync_usb_drives();
        }
        g_last_usb_poll_ms = timer_ms();
    }
}

void rtl8196c_usb_vfs_poll(void)
{
    if (!g_usb_ready)
        return;

    if (s_prof_pending) {
        s_prof_pending = 0;
        run_vfs_profile(s_prof_mb, s_prof_iters, s_prof_slot, s_prof_keep);
        g_last_usb_poll_ms = timer_ms();
        return;
    }

    /* Never poll USB hub topology while any USB file or directory handle is open */
    for (int i = 0; i < VFS_MAX_FILES; i++)
        if (g_files[i].in_use)
            return;
    for (int i = 0; i < VFS_MAX_DIRS; i++)
        if (g_dirs[i].in_use)
            return;

    uint32_t now = timer_ms();
    if ((uint32_t)(now - g_last_usb_poll_ms) < 2000u)
        return;
    g_last_usb_poll_ms = now;

    int ev = usb_poll();
    if (ev == USB_EV_CONNECTED || ev == USB_EV_DISCONNECTED) {
        memset(g_mount_tries, 0, sizeof(g_mount_tries));
        sync_usb_drives();
    } else if (ev == USB_EV_ENUM_FAILED) {
        usb_vbus(0);
        delay_ms(600);
        usb_vbus(1);
    } else if (usb_dev()->present && usb_dev()->is_hub) {
        if (usb_hub_scan_all() > 0 || has_unmounted_msc_candidate())
            sync_usb_drives();
    } else if (usb_dev()->present && has_unmounted_msc_candidate()) {
        sync_usb_drives();
    }
}

#endif /* PLATFORM_RTL8196C */
