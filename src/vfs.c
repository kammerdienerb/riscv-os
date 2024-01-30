#include "vfs.h"
#include "utils.h"
#include "kmalloc.h"
#include "kprint.h"
#include "driver.h"
#include "blk.h"
#include "utils.h"

void fs_minix3(void);
void fs_ext4(void);

static File *root;

static FS_Impl *fs_impls[NUM_FS];

File *vfs_new_file(const char *name, u32 kind, u32 adid) {
    File *new;

    new = kmalloc(sizeof(*new));

    new->parent = NULL;
    new->adid   = adid;

    new->name[0] = 0;
    strcpy(new->name, name);

    new->kind = kind;
    if (new->kind == FILE_DIRECTORY) {
        new->dir_entries = array_make(File*);
    }

    return new;
}

void vfs_add_dir_entry(File *dir, File *entry) {
    array_push(dir->dir_entries, entry);
    entry->parent = dir;
}

void vfs_file_delete(File *f) {
    u32    i;
    File **it;

    if (f->kind == FILE_DIRECTORY) {
        array_traverse(f->dir_entries, it) {
            vfs_file_delete(*it);
        }
        array_free(f->dir_entries);
    }

    if (f->parent != NULL) {
        i = 0;
        array_traverse(f->parent->dir_entries, it) {
            if ((*it) == f) {
                array_delete(f->parent->dir_entries, i);
                break;
            }
            i += 1;
        }
    }

    kfree(f);
}

static File * make_mount_point(u32 disk_number) {
    char  name[32];
    char  num_buff[16];
    File *d;

    strcpy(name, "disk");
    strcat(name, itos(num_buff, disk_number));

    d = vfs_new_file(name, FILE_DIRECTORY, 0);

    vfs_add_dir_entry(root, d);

    return d;
}

void init_vfs(void) {
    u32            disk_count;
    Driver_State **sit;
    Driver_State  *state;
    u32            i;
    File          *mount_point;
    char           buff[256];

    disk_count = 0;

    root = vfs_new_file("", FILE_DIRECTORY, 0);

    fs_minix3();
    fs_ext4();

    FOR_DRIVER_STATE(sit) {
        state = *sit;
        if (state->driver->type != DRV_BLOCK) { continue; }

        kprint("VFS: Found a drive on ADID 0x%x\n", state->active_device_id);

        for (i = 0; i < NUM_FS; i += 1) {
            if (fs_impls[i] == NULL) { continue; }

            if (fs_impls[i]->identify(state->active_device_id) == 1) {
                kprint("  %gidentified as a %s filesystem%_\n", fs_impls[i]->name);
                goto identified;
            }
        }

        kprint("  %yno identified filesystem%_\n");
        continue;

identified:;

        mount_point = disk_count
                        ? make_mount_point(disk_count)
                        : root;


        get_file_path(mount_point, buff);

        if (fs_impls[i]->mount(state->active_device_id, mount_point) != 0) {
            kprint("  %rfailed to mount filesystem to %s%_\n", buff);
            vfs_file_delete(mount_point);
        } else {
            kprint("  %gmounted to %s%_\n", buff);
        }

        disk_count += 1;
    }
}

void fs_impl(u32 which_fs, FS_Impl fns) {
    if (which_fs >= NUM_FS) { return; }

    fs_impls[which_fs]  = kmalloc(sizeof(*fs_impls[which_fs]));
    *fs_impls[which_fs] = fns;
}

File *get_file(const char *path) {
    u64    len;
    char   cpy[256];
    char  *p;
    File  *f;
    char  *k;
    char   oldk;
    File **it;

    len = strlen(path);

    if (len == 0 || path[0] != '/') {
        return NULL;
    }

    cpy[0] = 0;
    strcpy(cpy, path);

    p = cpy;
    f = root;

    while (f != NULL && *p == '/') {
        if (f->kind != FILE_DIRECTORY) { return NULL; }

        do { p += 1; } while (*p == '/');
        if (!*p) { break; }

        k = p;
        while (*k && *k != '/') { k += 1; }
        oldk = *k;
        *k   = 0;

        if (strcmp(p, ".") == 0) {
            goto cont;
        } else if (strcmp(p, "..") == 0) {
            if (f->parent != NULL) {
                f = f->parent;
            }
            goto cont;
        }

        array_traverse(f->dir_entries, it) {
            if (strcmp((*it)->name, p) == 0) {
                f = *it;
                goto cont;
            }
        }

        return NULL;

cont:;
        *k = oldk;
        p  = k;
    }

    return f;
}

s64 get_file_path(File *file, char *buff) {
    array_t   path;
    File    **it;

    if (file == NULL) {
        return -1;
    } else if (file == root) {
        buff[0] = 0;
        strcat(buff, "/");
        return 0;
    }

    path = array_make(File*);

    do {
        array_push(path, file);
        file = file->parent;
    } while (file->parent != NULL);

    buff[0] = 0;

    array_rtraverse(path, it) {
        strcat(buff, "/");
        strcat(buff, (*it)->name);
    }

    array_free(path);

    return 0;
}

s64 path_dirname(const char *path, char *buff) {
    u64         len;
    const char *p;

    buff[0] = 0;

    if (strcmp(path, "/") == 0) {
        strcpy(buff, "/");
        return 0;
    }

    len = strlen(path);
    if (len == 0) { return -1; }

    p = path + len - 1;

    while (p > path && *p != '/') { p -= 1; }

    if (p == path) { return -1; }

    memcpy(buff, path, p - path);
    buff[p - path] = 0;

    return 0;
}

s64 file_create(File *dir, const char *name, u32 kind) {
    if (dir->fs >= NUM_FS || fs_impls[dir->fs] == NULL) { return -1; }

    return fs_impls[dir->fs]->create(dir, name, kind);
}

s64 file_read(File *file, u8 *dst, u64 offset, u64 n_bytes) {
    if (file->fs >= NUM_FS || fs_impls[file->fs] == NULL) { return -1; }

    return fs_impls[file->fs]->read(file, dst, offset, n_bytes);
}

s64 file_write(File *file, u8 *src, u64 offset, u64 n_bytes) {
    if (file->fs >= NUM_FS || fs_impls[file->fs] == NULL) { return -1; }

    return fs_impls[file->fs]->write(file, src, offset, n_bytes);
}

s64 file_size(File *file) {
    if (file->fs >= NUM_FS || fs_impls[file->fs] == NULL) { return -1; }

    return fs_impls[file->fs]->size(file);
}
