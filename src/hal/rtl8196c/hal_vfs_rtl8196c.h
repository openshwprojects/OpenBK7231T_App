#ifndef HAL_VFS_RTL8196C_H
#define HAL_VFS_RTL8196C_H

#ifdef PLATFORM_RTL8196C

void rtl8196c_usb_vfs_init(void);
void rtl8196c_usb_vfs_poll(void);

int obk_vfs_file_open(lfs_t *lfs, lfs_file_t *file, const char *path, int flags);
int obk_vfs_file_close(lfs_t *lfs, lfs_file_t *file);
int obk_vfs_file_sync(lfs_t *lfs, lfs_file_t *file);
lfs_ssize_t obk_vfs_file_read(lfs_t *lfs, lfs_file_t *file, void *buffer, lfs_size_t size);
lfs_ssize_t obk_vfs_file_write(lfs_t *lfs, lfs_file_t *file, const void *buffer, lfs_size_t size);
lfs_soff_t obk_vfs_file_seek(lfs_t *lfs, lfs_file_t *file, lfs_soff_t off, int whence);
int obk_vfs_file_truncate(lfs_t *lfs, lfs_file_t *file, lfs_off_t size);
lfs_soff_t obk_vfs_file_tell(lfs_t *lfs, lfs_file_t *file);
int obk_vfs_file_rewind(lfs_t *lfs, lfs_file_t *file);
lfs_soff_t obk_vfs_file_size(lfs_t *lfs, lfs_file_t *file);

int obk_vfs_remove(lfs_t *lfs, const char *path);
int obk_vfs_rename(lfs_t *lfs, const char *oldpath, const char *newpath);
int obk_vfs_mkdir(lfs_t *lfs, const char *path);
int obk_vfs_stat(lfs_t *lfs, const char *path, struct lfs_info *info);

int obk_vfs_dir_open(lfs_t *lfs, lfs_dir_t *dir, const char *path);
int obk_vfs_dir_close(lfs_t *lfs, lfs_dir_t *dir);
int obk_vfs_dir_read(lfs_t *lfs, lfs_dir_t *dir, struct lfs_info *info);

#ifndef OBK_VFS_IMPLEMENTATION
#define lfs_file_open     obk_vfs_file_open
#define lfs_file_close    obk_vfs_file_close
#define lfs_file_sync     obk_vfs_file_sync
#define lfs_file_read     obk_vfs_file_read
#define lfs_file_write    obk_vfs_file_write
#define lfs_file_seek     obk_vfs_file_seek
#define lfs_file_truncate obk_vfs_file_truncate
#define lfs_file_tell     obk_vfs_file_tell
#define lfs_file_rewind   obk_vfs_file_rewind
#define lfs_file_size     obk_vfs_file_size
#define lfs_remove        obk_vfs_remove
#define lfs_rename        obk_vfs_rename
#define lfs_mkdir         obk_vfs_mkdir
#define lfs_stat          obk_vfs_stat
#define lfs_dir_open      obk_vfs_dir_open
#define lfs_dir_close     obk_vfs_dir_close
#define lfs_dir_read      obk_vfs_dir_read
#endif

#endif /* PLATFORM_RTL8196C */
#endif /* HAL_VFS_RTL8196C_H */
