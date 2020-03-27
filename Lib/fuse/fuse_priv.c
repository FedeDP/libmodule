#include "fuse_priv.h"

#define FUSE_USE_VERSION 35

#include "module.h"
#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h> // to get fuse fd to process events internally
#include <sys/poll.h> // poll operation support
#include <sys/ioctl.h> // ioctl support
#include <limits.h>

#define FUSE_PRIV()         fuse_priv_t *f = (fuse_priv_t *)c->fuse;
#define FUSE_CTX()          ctx_t *c = fuse_get_context()->private_data;
#define FUSE_ASSERT()       if (strlen(path) <= 1 || !map_has_key(c->modules, path + 1)) { return -ENOENT; }
#define FUSE_MOD()          mod_t *mod = map_get(c->modules, path + 1);

/** TODO: these will go in a separate public header, eg: <module/fs.h>! **/
#define MODULE_MAGIC        'L'

typedef struct {
    union {
        const char recipient[NAME_MAX];
        const char topic[NAME_MAX];
    };
    const uint8_t msg[512];
    const size_t size;
} fs_ps_t;

enum {
    MOD_GET_STATE   = _IOR(MODULE_MAGIC, 0, mod_states),
    MOD_START       = _IO(MODULE_MAGIC, 1),
    MOD_STOP        = _IO(MODULE_MAGIC, 2),
    MOD_RESUME      = _IO(MODULE_MAGIC, 3),
    MOD_PAUSE       = _IO(MODULE_MAGIC, 4),
    MOD_STATS       = _IOR(MODULE_MAGIC, 5, stats_t),
    MOD_PEEK_MSG    = _IOR(MODULE_MAGIC, 6, msg_t), // TODO
    MOD_SUBSCRIBE   = _IOW(MODULE_MAGIC, 7, char *),
    MOD_TELL        = _IOW(MODULE_MAGIC, 8, fs_ps_t),
    MOD_PUBLISH     = _IOW(MODULE_MAGIC, 9, fs_ps_t),
    MOD_BROADCAST   = _IOW(MODULE_MAGIC, 10, fs_ps_t),
};
/**                                                 **/

typedef struct {
    struct fuse *handler;
    struct fuse_buf buf;
    char *read_buf;
    size_t read_len;
    size_t write_len;
    time_t start;
    ev_src_t *src;
} fuse_priv_t;

typedef struct {
    int in_evt;
    mod_list_t *poll_handlers;
} fuse_poll_t;

static int fs_getattr(const char *path, struct stat *stbuf,
                      struct fuse_file_info *fi);
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi,
                      enum fuse_readdir_flags flags);
static void fs_logger(const self_t *self, const char *fmt, va_list args);
static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int fs_unlink(const char *path);
static int fs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi);
static bool init(void);
static void receive(const msg_t *msg, const void *userdata);
static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi);
static void poll_dtor(void *data);
static int fs_poll(const char *path, struct fuse_file_info *fi,
                   struct fuse_pollhandle *ph, unsigned *reventsp);
static int fs_ioctl(const char *path, unsigned int cmd, void *arg,
                    struct fuse_file_info *fi, unsigned int flags, void *data);

static const struct fuse_operations operations = {
    .getattr    = fs_getattr,
    .readdir    = fs_readdir,
    .read       = fs_read,
    .unlink     = fs_unlink,
    .create     = fs_create,
    .utimens    = fs_utimens,
    .poll       = fs_poll,
    .ioctl      = fs_ioctl
};

static int fs_getattr(const char *path, struct stat *stbuf,
                         struct fuse_file_info *fi) {
    FUSE_CTX();
    FUSE_PRIV();
    
    memset(stbuf, 0, sizeof(struct stat));
    
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();
    stbuf->st_atime = f->start;
    stbuf->st_mtime = f->start;
    if (strcmp(path, "/") == 0) { // root dir of fuse fs
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = map_length(c->modules);
        return 0;
    } 
    if (strlen(path) > 1 && map_has_key(c->modules, path + 1)) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 1024; // non-zero size
        return 0;
    }
    // not existent
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
    
    FUSE_CTX();
    for (mod_map_itr_t *itr = map_itr_new(c->modules); itr; itr = map_itr_next(itr)) {
        mod_t *mod = map_itr_get_data(itr);
        filler(buf, mod->name, NULL, 0, 0);
    }
    return 0;
}

static void fs_logger(const self_t *self, const char *fmt, va_list args) {
    FUSE_CTX();
    FUSE_PRIV();
    f->write_len += vsnprintf(f->read_buf + f->write_len, f->read_len, fmt, args);
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    FUSE_CTX();
    FUSE_ASSERT();
    FUSE_PRIV();
    FUSE_MOD();
    
    /* Store old context logger and replace it with a fuse one */
    log_cb old_log = c->logger;
    c->logger = fs_logger;
    f->read_buf = buf;
    f->read_len = size;
    f->write_len = 0;
    
    module_dump(mod->self);

    /* Restore real context logger */
    f->read_buf = NULL;
    f->read_len = 0;
    c->logger = old_log; 
    return f->write_len;
}

static int fs_unlink(const char *path) {
    FUSE_CTX();
    FUSE_ASSERT();
    FUSE_MOD();
    
    if (module_deregister(&mod->self) == MOD_OK) {
        return 0;
    }
    return -EPERM;
}

static int fs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
    return 0;
}

static bool init(void) {
    return true;
}

static void receive(const msg_t *msg, const void *userdata) {
    
}

static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    static userhook_t fuse_hook = { init, NULL, receive, NULL };
    FUSE_CTX();
    
    if (strlen(path) > 1) {
        self_t *self = NULL;
        if (module_register(mem_strdup(path + 1), c->name, &self, &fuse_hook, MOD_NAME_DUP) == MOD_OK) {
            return 0;
        }
        return -EPERM;
    }
    return -ENOENT;
}

static void poll_dtor(void *data) {
    fuse_pollhandle_destroy(data);
}

static int fs_poll(const char *path, struct fuse_file_info *fi,
                     struct fuse_pollhandle *ph, unsigned *reventsp) {
    FUSE_CTX();
    FUSE_ASSERT();
    FUSE_MOD();
    
    if (ph) {
        if (!mod->fuse_ph) {
            mod->fuse_ph = memhook._calloc(1, sizeof(fuse_poll_t));
        }
        fuse_poll_t *fp = (fuse_poll_t *)mod->fuse_ph;
        if (!fp->poll_handlers) {
            fp->poll_handlers = list_new(poll_dtor);
        }
        /* Insert new handler to support multiple handlers */
        if (fp->in_evt == 0) {
            list_insert(fp->poll_handlers, ph, NULL);
        } else {
            *reventsp |= POLLIN;
            fp->in_evt--;
        }
    }
    return 0;
}

static int fs_ioctl(const char *path, unsigned int cmd, void *arg,
                      struct fuse_file_info *fi, unsigned int flags, void *data) {
    FUSE_CTX();
    FUSE_ASSERT();
    FUSE_MOD();
    
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
            return module_start(mod->self);
        case MOD_STOP:
            return module_stop(mod->self);
        case MOD_RESUME:
            return module_resume(mod->self);
        case MOD_PAUSE:
            return module_pause(mod->self);
        case MOD_STATS:
            return module_stats(&mod->ref, data);
        case MOD_PEEK_MSG:
            data = NULL; // TODO: implement!
            break;
        case MOD_SUBSCRIBE:
            return module_register_sub(mod->self, data, SRC_DUP, NULL);
        case MOD_TELL: {
            fs_ps_t *p = (fs_ps_t *)data;
            const self_t *other = NULL;
            module_ref(mod->self, p->recipient, &other);
            return module_tell(mod->self, other, p->msg, p->size, PS_DUP_DATA);
        }
        case MOD_PUBLISH: {
            fs_ps_t *p = (fs_ps_t *)data;
            return module_publish(mod->self, p->topic, p->msg, p->size, PS_DUP_DATA);
        }
        case MOD_BROADCAST: {
            fs_ps_t *p = (fs_ps_t *)data;
            return module_broadcast(mod->self, p->msg, p->size, PS_DUP_DATA);
        }
        default:
            break;
    }
    return -EINVAL;
}

/** Private API **/

mod_ret fs_init(ctx_t *c) {
    c->fuse = memhook._calloc(1, sizeof(fuse_priv_t));
    MOD_ALLOC_ASSERT(c->fuse);
    
    FUSE_PRIV();
    char *name = "libmodule";
    struct fuse_args a = FUSE_ARGS_INIT(1, &name);
    
    f->handler = fuse_new(&a, &operations, sizeof(operations), c);
    MOD_ALLOC_ASSERT(f->handler);
    
    f->start = time(NULL);
    
    mkdir(c->name, 0777);
    if (fuse_mount(f->handler, c->name) == 0) {
        f->src = memhook._calloc(1, sizeof(ev_src_t));
        MOD_ALLOC_ASSERT(f->src);
        
        /* Actually register fuse fd in poll plugin */
        f->src->type = TYPE_FD;  
        f->src->fd_src.fd = fuse_session_fd(fuse_get_session(f->handler));
        if (poll_set_new_evt(&c->ppriv, f->src, ADD) == 0) {
            return MOD_OK;
        }
   }
   return MOD_ERR;
}

mod_ret fs_process(ctx_t *c) {
    FUSE_PRIV();

    struct fuse_session *sess = fuse_get_session(f->handler);
    if (fuse_session_receive_buf(sess, &f->buf) > 0) {
        fuse_session_process_buf(sess, &f->buf);
        return MOD_OK;
    }
    MODULE_DEBUG("Fuse: failed to retrieve buffer.\n");
    return MOD_ERR;
}

mod_ret fs_notify(const msg_t *msg) {
    mod_t *mod = msg->self->mod;
    if (mod->fuse_ph) {
        fuse_poll_t *fp = (fuse_poll_t *)mod->fuse_ph;
        if (msg->type == TYPE_PS && msg->ps_msg->type == LOOP_STOPPED) {
            /* When loop gets stopped, destroy the list */
            list_free(fp->poll_handlers);
            fp->poll_handlers = NULL;
        } else {
            fp->in_evt = list_length(fp->poll_handlers);
            for (mod_list_itr_t *itr = list_itr_new(fp->poll_handlers); itr; itr = list_itr_next(itr)) {
                fuse_notify_poll(list_itr_get_data(itr));
            }
            list_clear(fp->poll_handlers);
        }
    }
    return MOD_OK;
}

mod_ret fs_end(ctx_t *c) {
    FUSE_PRIV();
    
    /* Deregister fuse fd */
    poll_set_new_evt(&c->ppriv, f->src, RM);
    memhook._free(f->src);
    
    /* Destroy fuse handler */
    fuse_unmount(f->handler);
    fuse_destroy(f->handler);

    /* Delete folder */
    remove(c->name);
    
    memhook._free(c->fuse);
    c->fuse = NULL;
    return MOD_OK;
}
