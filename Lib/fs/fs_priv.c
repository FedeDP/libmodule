#include "fs_priv.h"

#define FUSE_USE_VERSION 35

#include "module.h"
#include "mem.h"
#include <fuse.h>
#include <fuse_lowlevel.h>  // to get fuse fd to process events internally
#include <sys/poll.h>       // poll operation support
#include <sys/ioctl.h>      // ioctl support
#include <limits.h>

#define FS_PRIV()         fs_ctx_t *f = (fs_ctx_t *)c->fs;
#define FS_CTX()          ctx_t *c = fuse_get_context()->private_data;
#define FS_CLIENT()       fs_client_t *cl = (fs_client_t *)fi->fh; if (!cl) { return -ENOENT; }
#define FS_MOD()          FS_CLIENT(); mod_t *mod = cl->mod;

/** TODO: these will go in a separate public header, eg: module/fs.h **/
#define MODULE_MAGIC        'm' // libmodule API global prefix

#define MOD_DATA_SIZE_LIMIT 512 

typedef struct {
    union {
        const char recipient[NAME_MAX];
        const char topic[NAME_MAX];
    };
    const uint8_t msg[MOD_DATA_SIZE_LIMIT];
    const size_t size;
} fs_ps_t;

typedef struct {
    mod_src_types type;
    size_t size;
    uint8_t data[]; // Use flexible array member
} fs_msg_t;

typedef struct {
    ps_msg_type type;
    char sender[NAME_MAX];
    char topic[NAME_MAX];
    uint8_t data[MOD_DATA_SIZE_LIMIT];
    size_t size;
} fs_ps_msg_t;

typedef struct {
    char path[PATH_MAX];
    unsigned int events;
} fs_pt_msg_t;

enum {
    MOD_GET_STATE           = _IOR(MODULE_MAGIC, 0, mod_states),
    MOD_START               = _IO(MODULE_MAGIC, 1),
    MOD_STOP                = _IO(MODULE_MAGIC, 2),
    MOD_RESUME              = _IO(MODULE_MAGIC, 3),
    MOD_PAUSE               = _IO(MODULE_MAGIC, 4),
    MOD_STATS               = _IOR(MODULE_MAGIC, 5, stats_t),
    MOD_SUBSCRIBE           = _IOW(MODULE_MAGIC, 6, char *),
    MOD_TELL                = _IOW(MODULE_MAGIC, 7, fs_ps_t),
    MOD_PUBLISH             = _IOW(MODULE_MAGIC, 8, fs_ps_t),
    MOD_BROADCAST           = _IOW(MODULE_MAGIC, 9, fs_ps_t),
    MOD_GET_MSGDATA_SIZE    = _IOR(MODULE_MAGIC, 10, size_t)
};
/**                                                 **/

typedef struct {
    struct fuse *handler;
    struct fuse_buf buf;
    struct fuse_args args;
    time_t start;
    ev_src_t *src;
} fs_ctx_t;

typedef struct {
    mod_t *mod;
    struct fuse_pollhandle *ph;
    bool in_evt;
    char *read_buf;
    size_t read_len;
    size_t write_len;
    bool read_all;
} fs_client_t;

typedef struct {
    m_list_t *clients;
    fs_msg_t *msg;
} fs_priv_t;

/* FS-related functions */
static int fs_getattr(const char *path, struct stat *stbuf,
                      struct fuse_file_info *fi);
static int fs_open(const char *path, struct fuse_file_info *fi);
static int fs_release(const char *path, struct fuse_file_info *fi);
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi,
                      enum fuse_readdir_flags flags);

static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int fs_unlink(const char *path);
static int fs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi);
static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi);
static int fs_poll(const char *path, struct fuse_file_info *fi,
                   struct fuse_pollhandle *ph, unsigned *reventsp);
static int fs_ioctl(const char *path, unsigned int cmd, void *arg,
                    struct fuse_file_info *fi, unsigned int flags, void *data);

/* Internal functions */
static void client_dtor(void *data);
static void fs_logger(const self_t *self, const char *fmt, va_list args);
static void msg_dump(fs_client_t *cl, const fs_priv_t *fp);
static bool init(void);
static void receive(const msg_t *msg, const void *userdata);
static void fs_store_msg(fs_priv_t *fp, const msg_t *msg);
static void fs_wakeup_clients(fs_priv_t *fp);

static const struct fuse_operations operations = {
    .getattr    = fs_getattr,
    .open       = fs_open,
    .release    = fs_release,
    .readdir    = fs_readdir,
    .read       = fs_read,
    .unlink     = fs_unlink,
    .create     = fs_create,
    .utimens    = fs_utimens,
    .poll       = fs_poll,
    .ioctl      = fs_ioctl
};

/** Fuse OPS Start **/
static int fs_getattr(const char *path, struct stat *stbuf,
                         struct fuse_file_info *fi) {
    FS_CTX();
    FS_PRIV();
    
    memset(stbuf, 0, sizeof(struct stat));
    
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();
    stbuf->st_atime = f->start;
    stbuf->st_mtime = f->start;
    if (strcmp(path, "/") == 0) { // root dir of fuse fs
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = m_map_length(c->modules);
        return 0;
    } 
    if (strlen(path) > 1 && m_map_has_key(c->modules, path + 1)) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 1024; // non-zero size
        return 0;
    }
    return -ENOENT;
}

static int fs_open(const char *path, struct fuse_file_info *fi) {
    FS_CTX();
    
    if (strlen(path) > 1) {
        mod_t *mod = m_map_get(c->modules, path + 1);
        if (mod) {
            fs_client_t *cl = memhook._calloc(1, sizeof(fs_client_t));
            if (!cl) {
                return -ENOMEM;
            }
            if (!mod->fs) {
                mod->fs = memhook._calloc(1, sizeof(fs_priv_t));
                if (!mod->fs) {
                    memhook._free(cl);
                    return -ENOMEM;
                }
            }
            fs_priv_t *fp = (fs_priv_t *)mod->fs;
            if (!fp->clients) { 
                fp->clients = m_list_new(client_dtor);
                if (!fp->clients) {
                    memhook._free(cl);
                    memhook._free(fp);
                    mod->fs = NULL;
                    return -ENOMEM;
                }
            }
            cl->mod = m_mem_ref(mod); // keep module alive while any client uses it
            m_list_insert(fp->clients, cl, NULL);
            fi->fh = (uint64_t) cl;
            fi->direct_io = 1;
            fi->nonseekable = 1;
            return 0;
        }
        return -ENOENT;
    }
    return -EISDIR;
}

static int fs_release(const char *path, struct fuse_file_info *fi) {
    FS_MOD();
    fi->fh = 0;
    fs_priv_t *fp = (fs_priv_t *)mod->fs;
    if (fp) {
        m_list_remove(fp->clients, cl, NULL);
        if (m_list_length(fp->clients) == 0) {
            m_list_free(&fp->clients);
            memhook._free(fp);
        }
        return 0;
    }
    return -ENOENT;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags) {
    if (strcmp(path, "/")) {
        return -ENOENT;
    }
        
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    
    FS_CTX();
    for (m_map_itr_t *itr = m_map_itr_new(c->modules); itr; itr = m_map_itr_next(itr)) {
        mod_t *mod = m_map_itr_get_data(itr);
        filler(buf, mod->name, NULL, 0, 0);
    }
    return 0;
}

/* NOTE: partial reads are unsupported!! */
static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {    
    FS_CTX();
    FS_MOD();
        
    cl->read_buf = buf;
    cl->read_len = size;
    cl->write_len = 0;
    errno = 0;
    
    if (!cl->in_evt) {
        if (!cl->read_all) {
            /* Store old context logger and replace it with a fuse one */
            log_cb old_log = c->logger;
            const void *old_userdata = mod->userdata;
            c->logger = fs_logger;
            mod->userdata = (void *)fi->fh;
    
            m_mod_dump(mod->self);
            if (cl->write_len > cl->read_len) {
                cl->write_len = cl->read_len;
            }
            
            /* Restore real context logger */
            c->logger = old_log;
            mod->userdata = old_userdata;
            cl->read_all = true;
        } else {
            cl->read_all = false;
        }
    } else {
        fs_priv_t *fp = (fs_priv_t *)mod->fs;
        if (size < sizeof(fs_msg_t) + fp->msg->size) {
            errno = EIO;
            cl->write_len = -1;
        } else {
            msg_dump(cl, fp);
            cl->in_evt = false;
        }
    }

    cl->read_buf = NULL;
    cl->read_len = 0;
    return cl->write_len;
}

static int fs_unlink(const char *path) {
    FS_CTX();
    if (strlen(path) > 1) {
        mod_t *mod = m_map_get(c->modules, path + 1);
        if (mod) {
            if (m_mod_deregister(&mod->self) == 0) {
                return 0;
            }
            return -EPERM;
        }
    }
    return -ENOENT;
}

static int fs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
    return 0;
}

static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    const userhook_t fuse_hook = { init, NULL, receive, NULL };
    FS_CTX();
    
    if (strlen(path) > 1) {
        self_t *self = NULL;
        if (m_mod_register(path + 1, c->name, &self, &fuse_hook, MOD_NAME_DUP) == 0) {
            return 0;
        }
        return -EPERM;
    }
    return -ENOENT;
}

static int fs_poll(const char *path, struct fuse_file_info *fi,
                     struct fuse_pollhandle *ph, unsigned *reventsp) {
    FS_MOD();

    if (ph) {
        if (cl->in_evt) {
            fs_priv_t *fp = (fs_priv_t *)mod->fs;
            if (fp->msg) {
                *reventsp |= POLLIN;
            } else {
                *reventsp |= POLLERR;
            }
            fuse_pollhandle_destroy(ph);
        } else {
            cl->ph = ph; // store pollhandle for future notifications
        }
    }
    return 0;
}

static int fs_ioctl(const char *path, unsigned int cmd, void *arg,
                      struct fuse_file_info *fi, unsigned int flags, void *data) {
    FS_MOD();
    
    if (flags & FUSE_IOCTL_COMPAT) {
        return -ENOSYS;
    }
    
    if (flags & FUSE_IOCTL_DIR) {
        return -EINVAL;
    }
    
    switch (cmd) {
        case MOD_GET_STATE:
            *(mod_states *)data = mod->state;
            return 0;
        case MOD_START:
            return m_mod_start(mod->self);
        case MOD_STOP:
            return m_mod_stop(mod->self);
        case MOD_RESUME:
            return m_mod_resume(mod->self);
        case MOD_PAUSE:
            return m_mod_pause(mod->self);
        case MOD_STATS:
            return m_mod_stats(&mod->ref, data);
        case MOD_SUBSCRIBE:
            return m_mod_register_sub(mod->self, data, SRC_DUP, NULL);
        case MOD_TELL: {
            fs_ps_t *p = (fs_ps_t *)data;
            const self_t *other = NULL;
            m_mod_ref(mod->self, p->recipient, &other);
            return m_mod_tell(mod->self, other, p->msg, p->size, PS_DUP_DATA);
        }
        case MOD_PUBLISH: {
            fs_ps_t *p = (fs_ps_t *)data;
            return m_mod_publish(mod->self, p->topic, p->msg, p->size, PS_DUP_DATA);
        }
        case MOD_BROADCAST: {
            fs_ps_t *p = (fs_ps_t *)data;
            return m_mod_broadcast(mod->self, p->msg, p->size, PS_DUP_DATA);
        }
        case MOD_GET_MSGDATA_SIZE: {
            if (cl->in_evt) {
                fs_priv_t *fp = (fs_priv_t *)mod->fs;
                if (fp->msg) {
                    *(size_t *)data = fp->msg->size;
                } else {
                    *(size_t *)data = -1;
                }
                return 0;
            }
        }
        default:
            break;
    }
    return -EINVAL;
}
/** Fuse OPS End **/

static void client_dtor(void *data) {
    fs_client_t *cl = (fs_client_t *)data;
    if (cl->ph) {
        fuse_pollhandle_destroy(cl->ph);
        cl->ph = NULL;
    }
    m_mem_unref(cl->mod);
    memhook._free(cl);
}

static void fs_logger(const self_t *self, const char *fmt, va_list args) {
    fs_client_t *cl = (fs_client_t *)self->mod->userdata;
    if (cl->write_len < cl->read_len) {
        /* 
         * vsnprintf: If the output was truncated due to this limit then the return value 
         * is the number of characters (excluding the terminating null byte) which would have been written 
         * to the final string if enough space had been available.
         * 
         * After module_dump() call, f->write_len is enforced to be at maximum f->read_len.
         */
        cl->write_len += vsnprintf(cl->read_buf + cl->write_len, cl->read_len - cl->write_len, fmt, args);
    }
}

static void msg_dump(fs_client_t *cl, const fs_priv_t *fp) {
    const size_t msg_size = sizeof(fs_msg_t) + fp->msg->size;
    memcpy(cl->read_buf, fp->msg, msg_size);
    cl->write_len += msg_size;
}

static bool init(void) {
    return true;
}

static void receive(const msg_t *msg, const void *userdata) {
    
}

static void fs_store_msg(fs_priv_t *fp, const msg_t *msg) {
    fs_ps_msg_t ps_msg = {0};
    fs_pt_msg_t pt_msg = {0};
    const void *data = NULL;
    size_t size = -1;
    switch (msg->type) {
        case TYPE_FD:
            size = sizeof(fd_msg_t);
            data = (void *)msg->fd_msg;
            break;
        case TYPE_PS:
            ps_msg.type = msg->ps_msg->type;
            if (msg->ps_msg->topic) {
                strncpy(ps_msg.topic, msg->ps_msg->topic, sizeof(ps_msg.topic));
            }
            if (msg->ps_msg->sender) {
                strncpy(ps_msg.sender, msg->ps_msg->sender->mod->name, sizeof(ps_msg.sender));
            }
            ps_msg.size = msg->ps_msg->size;
            if (msg->ps_msg->data) {
                memcpy(ps_msg.data, msg->ps_msg->data, sizeof(ps_msg.data));
            }
            data = (void *)&ps_msg;
            size = sizeof(fs_ps_msg_t);
            break;
        case TYPE_TMR:
            size = sizeof(tmr_msg_t);
            data = (void *)msg->tmr_msg;
            break;
        case TYPE_SGN:
            size = sizeof(sgn_msg_t);
            data = (void *)msg->sgn_msg;
            break;
        case TYPE_PID:
            size = sizeof(pid_msg_t);
            data = (void *)msg->pid_msg;
            break;
        case TYPE_PATH:
            strncpy(pt_msg.path, msg->pt_msg->path, sizeof(pt_msg.path));
            pt_msg.events = msg->pt_msg->events;
            size = sizeof(fs_pt_msg_t);
            data = (void *)&pt_msg;
            break;
        default:
            break;
    }
    if (data && size != -1) {
        void *tmp = memhook._realloc(fp->msg, sizeof(fs_msg_t) + size);
        if (tmp) {
            fp->msg = tmp;
            fp->msg->type = msg->type;
            fp->msg->size = size;
            memcpy(fp->msg->data, data, size);
        }
    }
}

static void fs_wakeup_clients(fs_priv_t *fp) {
    for (m_list_itr_t *itr = m_list_itr_new(fp->clients); itr; itr = m_list_itr_next(itr)) {
        fs_client_t *cl = m_list_itr_get_data(itr);
        if (cl->ph) {
            cl->in_evt = true;
            fuse_notify_poll(cl->ph);
            fuse_pollhandle_destroy(cl->ph);
            cl->ph = NULL;
        }
    }
}

/** Private API **/

int fs_init(ctx_t *c) {
    c->fs = memhook._calloc(1, sizeof(fs_ctx_t));
    MOD_ALLOC_ASSERT(c->fs);
    
    FS_PRIV();

    /* Mandatory fuse arg: app name */
    fuse_opt_add_arg(&f->args, "libmodule");
    
    f->handler = fuse_new(&f->args, &operations, sizeof(operations), c);
    MOD_ALLOC_ASSERT(f->handler);
    
    f->start = time(NULL);
    
    mkdir(c->name, 0777);
    int ret = fuse_mount(f->handler, c->name);
    if (ret == 0) {
        f->src = memhook._calloc(1, sizeof(ev_src_t));
        MOD_ALLOC_ASSERT(f->src);
        
        /* Actually register fuse fd in poll plugin */
        f->src->type = TYPE_FD;
        f->src->fd_src.fd = fuse_session_fd(fuse_get_session(f->handler));
        ret = poll_set_new_evt(&c->ppriv, f->src, ADD);
   }
   return ret;
}

int fs_process(ctx_t *c) {
    FS_PRIV();

    struct fuse_session *sess = fuse_get_session(f->handler);
    int ret = fuse_session_receive_buf(sess, &f->buf);
    if (ret > 0) {
        fuse_session_process_buf(sess, &f->buf);
        return 0;
    }
    MODULE_DEBUG("Fuse: failed to retrieve buffer.\n");
    return ret;
}

int fs_notify(const msg_t *msg) {
    mod_t *mod = msg->self->mod;
    if (mod && mod->fs) {
        fs_priv_t *fp = (fs_priv_t *)mod->fs;
        if (msg->type == TYPE_PS && msg->ps_msg->type == LOOP_STOPPED) {
            /* When loop gets stopped, destroy clients list */
            m_list_free(&fp->clients);
            memhook._free(fp->msg);
            fp->msg = NULL;
            memhook._free(fp);
            mod->fs = NULL;
        } else {
            fs_wakeup_clients(fp);
            fs_store_msg(fp, msg);
        }
    }
    return 0;
}

int fs_cleanup(mod_t *mod) {
    if (mod->fs) {
        fs_priv_t *fp = (fs_priv_t *)mod->fs;
        /* Destroy stored message, if any */
        memhook._free(fp->msg);
        fp->msg = NULL;
        
        fs_wakeup_clients(fp);
    }
    return 0;
}

int fs_end(ctx_t *c) {
    FS_PRIV();
    
    /* Deregister fuse fd */
    poll_set_new_evt(&c->ppriv, f->src, RM);
    memhook._free(f->src);

    /* Free fuse recv buf */
    memhook._free(f->buf.mem);
    
    /* Destroy fuse handler */
    fuse_unmount(f->handler);
    fuse_destroy(f->handler);
    
    /* Free fuse args */
    fuse_opt_free_args(&f->args);

    /* Delete folder */
    remove(c->name);
    
    memhook._free(c->fs);
    c->fs = NULL;
    return 0;
}
