#ifndef __VFS_H__
#define __VFS_H__

#include "common.h"
#include "array.h"

enum {
    FILE_REGULAR,
    FILE_DIRECTORY,
};

enum {
    FILE_RD,
    FILE_WR,
    FILE_EX,
};

enum {
    FS_MINIX3,
    FS_EXT4,
    NUM_FS,
};

#define S_FMT(x)       (S_IFMT & (x))

#define S_IFMT        0170000 /* These bits determine file type.  */

/* File types. */
#define S_IFDIR       0040000 /* Directory.         */
#define S_IFCHR       0020000 /* Character device.  */
#define S_IFBLK       0060000 /* Block device.      */
#define S_IFREG       0100000 /* Regular file.      */
#define S_IFIFO       0010000 /* FIFO.              */
#define S_IFLNK       0120000 /* Symbolic link.     */
#define S_IFSOCK      0140000 /* Socket.            */

/* Protection bits. */
#define S_ISUID       04000   /* Set user ID on execution.              */
#define S_ISGID       02000   /* Set group ID on execution.             */
#define S_ISVTX       01000   /* Save swapped text after use (sticky).  */
#define S_IREAD       0400    /* Read by owner.                         */
#define S_IWRITE      0200    /* Write by owner.                        */
#define S_IEXEC       0100    /* Execute by owner.                      */


typedef struct File {
    struct File *parent;
    u32          inode;
    u32          adid;
    char         name[256];
    u32          kind;
    u32          fs;
    array_t      dir_entries;
} File;

typedef struct {
    char  name[32];
    s64 (*identify)(u32);
    s64 (*mount)(u32, File*);
    s64 (*create)(File*, const char*, u32);
    s64 (*read)(File*, u8*, u64, u64);
    s64 (*write)(File*, u8*, u64, u64);
    s64 (*size)(File*);
} FS_Impl;

void init_vfs(void);

void  fs_impl(u32 which_fs, FS_Impl impl);
File *vfs_new_file(const char *name, u32 kind, u32 adid);
void  vfs_add_dir_entry(File *dir, File *entry);
void  vfs_file_delete(File *f);
File *get_file(const char *path);
s64   get_file_path(File *file, char *buff);
s64   path_dirname(const char *path, char *buff);
s64   file_create(File *dir, const char *name, u32 kind);
s64   file_read(File *file, u8 *dst, u64 offset, u64 n_bytes);
s64   file_write(File *file, u8 *src, u64 offset, u64 n_bytes);
s64   file_size(File *file);

#endif
