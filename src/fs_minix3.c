#include "common.h"
#include "vfs.h"
#include "driver.h"
#include "utils.h"
#include "kmalloc.h"
#include "array.h"
#include "blk.h"
#include "kprint.h"

static s64 identify(u32 adid);
static s64 mount(u32 adid, File *mount_point);
static s64 create(File *dir, const char *name, u32 kind);
static s64 read(File *file, u8 *dst, u64 offset, u64 n_bytes);
static s64 write(File *file, u8 *src, u64 offset, u64 n_bytes);
static s64 size(File *file);

static FS_Impl impl = {
    .name     = "minix3",
    .identify = identify,
    .mount    = mount,
    .create   = create,
    .read     = read,
    .write    = write,
    .size     = size,
};

typedef struct {
    u32 num_inodes;
    u16 _pad0;
    u16 imap_blocks;
    u16 zmap_blocks;
    u16 first_data_zone;
    u16 log_zone_size;
    u16 _pad1;
    u32 max_size;
    u32 num_zones;
    u16 magic;
    u16 _pad2;
    u16 block_size;
    u8  disk_version;
} Super_Block;

typedef struct {
    u16 mode;
    u16 nlinks;
    u16 uid;
    u16 gid;
    u32 size;
    u32 atime;
    u32 mtime;
    u32 ctime;
    u32 zones[10];
} Inode;

typedef struct {
   u32  inode;
   char name[60];
} Dir_Entry;

#define OFFSET(sb, _inum)                  \
    ( 1024ULL                              \
    + (sb).block_size                      \
    + ((_inum) - 1ULL) * 64ULL             \
    + ((sb).imap_blocks * (sb).block_size) \
    + ((sb).zmap_blocks * (sb).block_size))


typedef struct {
    u32         adid;
    Super_Block sb;
} Instance;


static array_t instances;

void fs_minix3(void) {
    instances = array_make(Instance*);
    fs_impl(FS_MINIX3, impl);
}


static Instance *get_instance(u32 adid) {
    Instance **it;

    array_traverse(instances, it) {
        if ((*it)->adid == adid) { return *it; }
    }

    return NULL;
}

static s64 identify(u32 adid) {
    Super_Block  sb;
    Instance    *inst;

    if (blk_read(adid, 1024, (void*)&sb, sizeof(sb)) == -1) {
        return 0;
    }

    if (sb.magic == 0x4D5A) {
        inst = kmalloc(sizeof(*inst));
        inst->adid = adid;
        memcpy(&inst->sb, &sb, sizeof(inst->sb));
        array_push(instances, inst);
        return 1;
    }

    return 0;
}


static void mount_inode(Instance *inst, Inode *inode, Dir_Entry *entry, File *mount_point);

static File *mount_file(Instance *inst, Inode *inode, Dir_Entry *entry, File *mount_point) {
    File *f;

    f = vfs_new_file(entry->name,
                     inode->mode & S_IFDIR
                         ? FILE_DIRECTORY
                         : FILE_REGULAR,
                     inst->adid);

    f->inode = entry->inode;
    f->fs    = FS_MINIX3;

    vfs_add_dir_entry(mount_point, f);

    return f;
}

static void mount_dir_entries(Instance *inst, Inode *inode, File *mount_point) {
    u32        num_entries;
    void      *block;
    u32        i;
    Dir_Entry *entry;
    u32        j;
    Inode      entry_inode;
    u32       *iblock;
    u32        k;
    u32       *diblock;
    u32        l;
    u32       *tiblock;

    num_entries = inode->size / sizeof(Dir_Entry);

    if (num_entries == 0) { return; }

    block = kmalloc(inst->sb.block_size);

    for (i = 0; i < 7 && num_entries; i += 1) {
        if (inode->zones[i] == 0) { continue; }

        blk_read(inst->adid,
                 inode->zones[i] * inst->sb.block_size,
                 block,
                 inst->sb.block_size);

        for (j = 0; j < inst->sb.block_size / sizeof(Dir_Entry) && num_entries; j += 1) {
            entry = block + (j * sizeof(Dir_Entry));

            blk_read(inst->adid, OFFSET(inst->sb, entry->inode), (void*)&entry_inode, sizeof(entry_inode));
            mount_inode(inst, &entry_inode, entry, mount_point);


            num_entries -= 1;
        }
    }

    if (num_entries > 0) {
        iblock = kmalloc(inst->sb.block_size);

        blk_read(inst->adid, inode->zones[7] * inst->sb.block_size, (void*)iblock, inst->sb.block_size);

        for (i = 0; i < inst->sb.block_size / 4 && num_entries; i += 1) {
            if (iblock[i] == 0) { continue; }

            blk_read(inst->adid,
                     iblock[i] * inst->sb.block_size,
                     block,
                     inst->sb.block_size);

            for (j = 0; j < inst->sb.block_size / sizeof(Dir_Entry) && num_entries; j += 1) {
                entry = block + (j * sizeof(Dir_Entry));

                blk_read(inst->adid, OFFSET(inst->sb, entry->inode), (void*)&entry_inode, sizeof(entry_inode));
                mount_inode(inst, &entry_inode, entry, mount_point);

                num_entries -= 1;
            }
        }

        kfree(iblock);
    }

    if (num_entries > 0) {
        iblock  = kmalloc(inst->sb.block_size);
        diblock = kmalloc(inst->sb.block_size);

        blk_read(inst->adid, inode->zones[8] * inst->sb.block_size, (void*)iblock, inst->sb.block_size);

        for (i = 0; i < inst->sb.block_size / 4 && num_entries; i += 1) {
            if (iblock[i] == 0) { continue; }

            blk_read(inst->adid, iblock[i] * inst->sb.block_size, (void*)diblock, inst->sb.block_size);

            for (j = 0; j < inst->sb.block_size / 4 && num_entries; j += 1) {
                if (diblock[j] == 0) { continue; }

                blk_read(inst->adid,
                         diblock[j] * inst->sb.block_size,
                         block,
                         inst->sb.block_size);

                for (k = 0; k < inst->sb.block_size / sizeof(Dir_Entry) && num_entries; k += 1) {
                    entry = block + (k * sizeof(Dir_Entry));

                    blk_read(inst->adid, OFFSET(inst->sb, entry->inode), (void*)&entry_inode, sizeof(entry_inode));
                    mount_inode(inst, &entry_inode, entry, mount_point);

                    num_entries -= 1;
                }
            }
        }

        kfree(diblock);
        kfree(iblock);
    }

    if (num_entries > 0) {
        iblock  = kmalloc(inst->sb.block_size);
        diblock = kmalloc(inst->sb.block_size);
        tiblock = kmalloc(inst->sb.block_size);

        blk_read(inst->adid, inode->zones[9] * inst->sb.block_size, (void*)iblock, inst->sb.block_size);

        for (i = 0; i < inst->sb.block_size / 4 && num_entries; i += 1) {
            if (iblock[i] == 0) { continue; }

            blk_read(inst->adid, iblock[i] * inst->sb.block_size, (void*)diblock, inst->sb.block_size);

            for (j = 0; j < inst->sb.block_size / 4 && num_entries; j += 1) {
                if (diblock[j] == 0) { continue; }

                blk_read(inst->adid, diblock[j] * inst->sb.block_size, (void*)tiblock, inst->sb.block_size);

                for (k = 0; k < inst->sb.block_size / 4 && num_entries; k += 1) {
                    if (tiblock[k] == 0) { continue; }

                    for (l = 0; l < inst->sb.block_size / sizeof(Dir_Entry) && num_entries; l += 1) {
                        entry = block + (l * sizeof(Dir_Entry));

                        blk_read(inst->adid, OFFSET(inst->sb, entry->inode), (void*)&entry_inode, sizeof(entry_inode));
                        mount_inode(inst, &entry_inode, entry, mount_point);

                        num_entries -= 1;
                    }
                }
            }
        }

        kfree(tiblock);
        kfree(diblock);
        kfree(iblock);
    }

    kfree(block);
}

static void mount_inode(Instance *inst, Inode *inode, Dir_Entry *entry, File *mount_point) {
    File *f;

    f = mount_file(inst, inode, entry, mount_point);

    if (inode->mode & S_IFDIR
    &&  strcmp(entry->name, ".")  != 0
    &&  strcmp(entry->name, "..") != 0) {
        mount_dir_entries(inst, inode, f);
    }
}

static s64 mount(u32 adid, File *mount_point) {
    Instance *inst;
    u32       inum;
    Inode     inode;

    if ((inst = get_instance(adid)) == NULL) { return -1; }

    inum = 1;
    blk_read(inst->adid, OFFSET(inst->sb, inum), (void*)&inode, sizeof(inode));

    if (!(inode.mode & S_IFDIR)) {
        kprint("root inode is not a directory\n");
        return -1;
    }

    mount_dir_entries(inst, &inode, mount_point);

    return 0;
}

static s64 claim_inode(Instance *inst) {
    u8  *block;
    u32  inum;
    u32  b;
    u32  i;
    u32  j;

    block = kmalloc(inst->sb.block_size);

    inum = 1;
    for (b = 0; b < inst->sb.imap_blocks; b += 1) {
        blk_read(inst->adid,
                 1024ULL + inst->sb.block_size + (b * inst->sb.block_size),
                 block,
                 inst->sb.block_size);

        for (i = 0; i < inst->sb.block_size; i += 1) {
            for (j = 0; j < 8; j += 1) {
                if (!(block[i] & (1 << j))) {
                    block[i] |= (1 << j);
                    blk_write(inst->adid,
                              1024ULL + inst->sb.block_size + (b * inst->sb.block_size),
                              block,
                              inst->sb.block_size);
                    kfree(block);
                    return inum;
                }
                inum += 1;
            }
        }
    }

    kfree(block);
    return -1;
}

static u32 claim_zone(Instance *inst) {
    u8  *block;
    u32  znum;
    u32  b;
    u32  i;
    u32  j;

    block = kmalloc(inst->sb.block_size);

    znum = 1;
    for (b = 0; b < inst->sb.zmap_blocks; b += 1) {
        blk_read(inst->adid,
                 1024ULL + inst->sb.block_size + (inst->sb.imap_blocks * inst->sb.block_size) + (b * inst->sb.block_size),
                 block,
                 inst->sb.block_size);

        for (i = 0; i < inst->sb.block_size; i += 1) {
            for (j = 0; j < 8; j += 1) {
                if (!(block[i] & (1 << j))) {
                    block[i] |= (1 << j);
                    blk_write(inst->adid,
                              1024ULL + inst->sb.block_size + (b * inst->sb.block_size),
                              block,
                              inst->sb.block_size);
                    kfree(block);
                    return znum;
                }
                znum += 1;
            }
        }
    }

    kfree(block);
    return -1;
}

static void grow_to_fit(Instance *inst, u32 inum, Inode *inode, u32 new_size) {
    u32  new_blocks;
    u32  changed;
    u32  i;
    u32 *iblock;
    u32  ichanged;

    if (inode->size >= new_size) { return; }

    new_blocks = (new_size - inode->size) / inst->sb.block_size;

    changed = 0;

    for (i = 0; i < 7 && new_blocks; i += 1) {
        if (inode->zones[i] == 0) {
            inode->zones[i] = claim_zone(inst);
            new_blocks -= 1;
            changed = 1;
        }
    }

    if (changed) {
        blk_write(inst->adid, OFFSET(inst->sb, inum), (void*)inode, sizeof(*inode));
    }

    if (new_blocks > 0) {
        iblock = kmalloc(inst->sb.block_size);
        blk_read(inst->adid, inode->zones[7] * inst->sb.block_size, (void*)iblock, inst->sb.block_size);

        ichanged = 0;

        for (i = 0; i < inst->sb.block_size / 4 && new_blocks; i += 1) {
            if (iblock[i] == 0) {
                iblock[i] = claim_inode(inst);
                new_blocks -= 1;
                ichanged    = 1;
            }
        }

        if (ichanged) {
            blk_write(inst->adid, inode->zones[8] * inst->sb.block_size, (void*)iblock, inst->sb.block_size);
        }
        kfree(iblock);
    }
    /* @todo doubly and triply indirect */

}

static s64 create(File *dir, const char *name, u32 kind) {
    Instance  *inst;
    Inode      inode;
    u32        inum;
    void      *block;
    u32        num_entries;
    u32        i;
    Dir_Entry *entry;
    File      *new;

    if ((inst = get_instance(dir->adid)) == NULL) { return -1; }

    blk_read(inst->adid,
             OFFSET(inst->sb, dir->inode),
             (void*)&inode,
             sizeof(inode));

    grow_to_fit(inst, dir->inode, &inode, inode.size + sizeof(Dir_Entry));

    inum = claim_inode(inst);

    blk_read(inst->adid,
             OFFSET(inst->sb, dir->inode),
             (void*)&inode,
             sizeof(inode));

    num_entries = inode.size / sizeof(Dir_Entry);

    block = kmalloc(inst->sb.block_size);

    for (i = 0; i < 7; i += 1) {
        if (inode.zones[i] == 0) { continue; }

        blk_read(inst->adid,
                 inode.zones[i] * inst->sb.block_size,
                 block,
                 inst->sb.block_size);

        if (num_entries > inst->sb.block_size / sizeof(Dir_Entry)) {
            num_entries -= inst->sb.block_size / sizeof(Dir_Entry);
        } else {
            entry = block + num_entries * sizeof(Dir_Entry);

            entry->inode = inum;
            strcpy(entry->name, name);

            blk_write(inst->adid,
                    inode.zones[i] * inst->sb.block_size,
                    block,
                    inst->sb.block_size);
            break;
        }
    }

    inode.size += sizeof(Dir_Entry);

    blk_write(inst->adid,
              OFFSET(inst->sb, dir->inode),
              (void*)&inode,
              sizeof(inode));


    /* @todo indirection */

    kfree(block);



    memset(&inode, 0, sizeof(inode));

    if (kind == FILE_REGULAR) {
        inode.mode |= S_IFREG;
    } else if (kind == FILE_DIRECTORY) {
        inode.mode |= S_IFDIR;
    }

    inode.nlinks = 1;

    blk_write(inst->adid, OFFSET(inst->sb, inum), (void*)&inode, sizeof(inode));

    new = vfs_new_file(name, kind, inst->adid);
    vfs_add_dir_entry(dir, new);

    return 0;
}

static s64 read(File *file, u8 *dst, u64 offset, u64 n_bytes) {
    Instance *inst;
    Inode     inode;
    void     *block;
    u64       boff;
    u32       i;
    u64       start;
    u64       n;
    u32      *iblock;
    u32      *diblock;
    u32       j;
    u32      *tiblock;
    u32       k;

    inst = get_instance(file->adid);

    blk_read(inst->adid,
             OFFSET(inst->sb, file->inode),
             (void*)&inode,
             sizeof(inode));

    block = kmalloc(inst->sb.block_size);
    boff  = 0;

    for (i = 0; i < 7 && n_bytes; i += 1) {
        if (inode.zones[i] == 0) { continue; }

        if (boff + inst->sb.block_size >= offset) {
            start = offset & (inst->sb.block_size - 1);
            n     = start == 0
                        ? MIN(inst->sb.block_size, n_bytes)
                        : inst->sb.block_size - start;

            blk_read(inst->adid, inode.zones[i] * inst->sb.block_size + start, dst, n);

            offset  += n;
            dst     += n;
            n_bytes -= n;
        }

        boff += inst->sb.block_size;
    }

    if (n_bytes > 0) {
        iblock = kmalloc(inst->sb.block_size);

        blk_read(inst->adid, inode.zones[7] * inst->sb.block_size, (void*)iblock, inst->sb.block_size);

        for (i = 0; i < inst->sb.block_size / 4 && n_bytes; i += 1) {
            if (iblock[i] == 0) { continue; }

            if (boff + inst->sb.block_size >= offset) {
                start = offset & (inst->sb.block_size - 1);
                n     = start == 0
                            ? MIN(inst->sb.block_size, n_bytes)
                            : inst->sb.block_size - start;

                blk_read(inst->adid, iblock[i] * inst->sb.block_size + start, dst, n);

                offset  += n;
                dst     += n;
                n_bytes -= n;
            }

            boff += inst->sb.block_size;
        }

        kfree(iblock);
    }

    if (n_bytes > 0) {
        iblock  = kmalloc(inst->sb.block_size);
        diblock = kmalloc(inst->sb.block_size);

        blk_read(inst->adid, inode.zones[8] * inst->sb.block_size, (void*)iblock, inst->sb.block_size);

        for (i = 0; i < inst->sb.block_size / 4 && n_bytes; i += 1) {
            if (iblock[i] == 0) { continue; }

            blk_read(inst->adid, iblock[i] * inst->sb.block_size, (void*)diblock, inst->sb.block_size);

            for (j = 0; j < inst->sb.block_size / 4 && n_bytes; j += 1) {
                if (diblock[j] == 0) { continue; }

                if (boff + inst->sb.block_size >= offset) {
                    start = offset & (inst->sb.block_size - 1);
                    n     = start == 0
                                ? MIN(inst->sb.block_size, n_bytes)
                                : inst->sb.block_size - start;

                    blk_read(inst->adid, diblock[j] * inst->sb.block_size + start, dst, n);

                    offset  += n;
                    dst     += n;
                    n_bytes -= n;
                }

                boff += inst->sb.block_size;
            }
        }

        kfree(diblock);
        kfree(iblock);
    }

    if (n_bytes > 0) {
        iblock  = kmalloc(inst->sb.block_size);
        diblock = kmalloc(inst->sb.block_size);
        tiblock = kmalloc(inst->sb.block_size);

        blk_read(inst->adid, inode.zones[9] * inst->sb.block_size, (void*)iblock, inst->sb.block_size);

        for (i = 0; i < inst->sb.block_size / 4 && n_bytes; i += 1) {
            if (iblock[i] == 0) { continue; }

            blk_read(inst->adid, iblock[i] * inst->sb.block_size, (void*)diblock, inst->sb.block_size);

            for (j = 0; j < inst->sb.block_size / 4 && n_bytes; j += 1) {
                if (diblock[j] == 0) { continue; }

                blk_read(inst->adid, diblock[j] * inst->sb.block_size, (void*)tiblock, inst->sb.block_size);

                for (k = 0; k < inst->sb.block_size / 4 && n_bytes; k += 1) {
                    if (tiblock[k] == 0) { continue; }

                    if (boff + inst->sb.block_size >= offset) {
                        start = offset & (inst->sb.block_size - 1);
                        n     = start == 0
                                    ? MIN(inst->sb.block_size, n_bytes)
                                    : inst->sb.block_size - start;

                        blk_read(inst->adid, tiblock[k] * inst->sb.block_size + start, dst, n);

                        offset  += n;
                        dst     += n;
                        n_bytes -= n;
                    }

                    boff += inst->sb.block_size;
                }
            }
        }

        kfree(tiblock);
        kfree(diblock);
        kfree(iblock);
    }

    kfree(block);

    return n_bytes;
}

static s64 write(File *file, u8 *src, u64 offset, u64 n_bytes) {
    return -1;
}

static s64 size(File *file) {
    Instance *inst;
    Inode     inode;

    inst = get_instance(file->adid);

    blk_read(inst->adid,
             OFFSET(inst->sb, file->inode),
             (void*)&inode,
             sizeof(inode));

    return inode.size;
}
