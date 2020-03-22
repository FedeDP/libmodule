#include "fuse_priv.h"

#define FUSE_USE_VERSION 35

#include "module.h"
#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h> // to get fuse fd to process events internally
#include <sys/poll.h> // poll operation support
#include <sys/ioctl.h> // ioctl support

#define FUSE_PRIV()         fuse_priv_t *f = (fuse_priv_t *)c->fuse;
#define FUSE_CTX()          ctx_t *c = fuse_get_context()->private_data;
#define FUSE_ASSERT()       if (strlen(path) <= 1 || !map_has_key(c->modules, path + 1)) { return -ENOENT; }
#define FUSE_MOD()          mod_t *mod = map_get(c->modules, path + 1);

/* TODO: these will go in a separate public header! */
typedef enum { M_START, M_STOP, M_RESUME, M_PAUSE } fs_mod_state_ops;

typedef struct {
    const char *recipient; // topic for publish, module's name for tell
    const void *msg;
    const size_t size;
} fs_mod_ps_params;

enum {
    MOD_GET_STATE   = _IOR('L', 0, mod_states),
    MOD_SET_STATE   = _IOW('L', 1, fs_mod_state_ops),
    MOD_RECV_MSG    = _IOR('L', 2, msg_t),
    MOD_SUBSCRIBE   = _IOW('L', 3, const char *),
    MOD_TELL        = _IOW('L', 4, fs_mod_ps_params),
    MOD_PUBLISH     = _IOW('L', 5, fs_mod_ps_params),
};

/* */

typedef enum { D_TELL, D_START, D_STOP, D_RESUME, D_PAUSE, D_SUB, D_PUB, D_SIZE } dict_ops;

static const char *dict[] = {
    "-> '%m[^']' '%m[^']'",
    "START",
    "STOP",
    "RESUME",
    "PAUSE",
    "SUB '%m[^']'",
    "PUB '%m[^']' '%m[^']'"
};

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

static int do_getattr(const char *path, struct stat *stbuf,
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
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = 1024; // non-zero size
        return 0;
    }
    // not existent
    return -ENOENT;
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
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

static void fuse_read(const self_t *self, const char *fmt, va_list args) {
    FUSE_CTX();
    FUSE_PRIV();
    f->write_len += vsnprintf(f->read_buf + f->write_len, f->read_len, fmt, args);
}

static int do_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    FUSE_CTX();
    FUSE_ASSERT();
    FUSE_PRIV();
    FUSE_MOD();
    
    /* Store old context logger and replace it with a fuse one */
    log_cb old_log = c->logger;
    c->logger = fuse_read;
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

static int grammar_parse(mod_t *mod, const char *buf, size_t size) {
    int ret = -EINVAL;
    for (int i = D_TELL; i < D_SIZE; i++) {
        switch (i) {
        case D_TELL: {
            const char *recipient = NULL, *msg = NULL;
            if (sscanf(buf, dict[i], &recipient, &msg) == 2) {
                const self_t *other = NULL;
                module_ref(mod->self, recipient, &other);
                ret = module_tell(mod->self, other, msg, strlen(msg), PS_AUTOFREE);
            }
            memhook._free((void *)recipient);
            break;
        }
        case D_START: {
            if (!strncmp(buf, dict[i], strlen(dict[i]))) {
                ret = module_start(mod->self);
            }
            break;
        }
        case D_STOP: {
            if (!strncmp(buf, dict[i], strlen(dict[i]))) {
                ret = module_stop(mod->self);
            }
            break;
        }
        case D_RESUME: {
            if (!strncmp(buf, dict[i], strlen(dict[i]))) {
                ret = module_resume(mod->self);
            }
            break;
        }
        case D_PAUSE: {
            if (!strncmp(buf, dict[i], strlen(dict[i]))) {
                ret = module_pause(mod->self);
            }
            break;
        }
        case D_SUB: {
            const char *topic = NULL;
            if (sscanf(buf, dict[i], &topic) == 1) {
                ret = module_register_sub(mod->self, topic, SRC_DUP, NULL);
                memhook._free((void *)topic);
            }
            break;
        }
        case D_PUB: {
            const char *topic = NULL, *msg = NULL;
            if (sscanf(buf, dict[i], &topic, &msg) == 2) {
                ret = module_publish(mod->self, topic, msg, strlen(msg), PS_AUTOFREE);
            }
            memhook._free((void *)topic);
            break;
        }
        default:
            break;
        }
    }
    if (ret != MOD_OK) {
        ret = -EINVAL;
    } else {
        ret = size;
    }
    return ret;
}

static int do_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    FUSE_CTX();
    FUSE_ASSERT();
    FUSE_MOD();
    
    return grammar_parse(mod, buf, size);
}

static int do_unlink(const char *path) {
    FUSE_CTX();
    FUSE_ASSERT();
    FUSE_MOD();
    
    if (mod->from_fuse && module_deregister(&mod->self) == MOD_OK) {
        return 0;
    }
    return -EPERM;
}

static int do_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
    return 0;
}

static bool fs_init(void) {
    return true;
}

static void fs_recv(const msg_t *msg, const void *userdata) {
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
}

static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    static userhook_t fuse_hook = { fs_init, NULL, fs_recv, NULL };
    FUSE_CTX();
    
    if (strlen(path) > 1) {
        self_t *self = NULL;
        if (module_register(mem_strdup(path + 1), c->name, &self, &fuse_hook) == MOD_OK) {
            self->mod->from_fuse = true;
            return 0;
        }
        return -EPERM;
    }
    return -ENOENT;
}

static void poll_dtor(void *data) {
    fuse_pollhandle_destroy(data);
}

static int do_poll(const char *path, struct fuse_file_info *fi,
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

static int do_ioctl(const char *path, unsigned int cmd, void *arg,
                      struct fuse_file_info *fi, unsigned int flags, void *data) {
    FUSE_CTX();
    FUSE_ASSERT();
    FUSE_MOD();
    
    if (flags & FUSE_IOCTL_COMPAT)
        return -ENOSYS;
    
    switch (cmd) {
        case MOD_GET_STATE:
            *(mod_states *)data = mod->state;
            return 0;
        case MOD_SET_STATE:
            switch (*((fs_mod_state_ops *)data)) {
            case M_START:
                return module_start(mod->self);
            case M_STOP:
                return module_stop(mod->self);
            case M_RESUME:
                return module_resume(mod->self);
            case M_PAUSE:
                return module_pause(mod->self);
            default:
                break;
            }
            break;
        case MOD_RECV_MSG:
            data = NULL; // FIXME
            break;
        case MOD_SUBSCRIBE:
            return module_register_sub(mod->self, data, SRC_DUP, NULL);
        case MOD_TELL: {
            fs_mod_ps_params *p = (fs_mod_ps_params *)data;
            const self_t *other = NULL;
            module_ref(mod->self, p->recipient, &other);
            return module_tell(mod->self, other, p->msg, p->size, PS_DUP_DATA);
        }
        case MOD_PUBLISH: {
            fs_mod_ps_params *p = (fs_mod_ps_params *)data;
            return module_publish(mod->self, p->recipient, p->msg, p->size, PS_DUP_DATA);
        }
        default:
            break;
    }
    return -EINVAL;
}

static const struct fuse_operations operations = {
    .getattr    = do_getattr,
    .readdir    = do_readdir,
    .read       = do_read,
    .write      = do_write,
    .unlink     = do_unlink,
    .create     = do_create,
    .utimens    = do_utimens,
    .poll       = do_poll,
    .ioctl      = do_ioctl
};

int fuse_init(ctx_t *c) {
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
        // FIXME...?
        f->buf.mem = memhook._malloc(1024);
        f->buf.size = 1024;
        MOD_ALLOC_ASSERT(f->buf.mem);
        
        f->src = memhook._calloc(1, sizeof(ev_src_t));
        MOD_ALLOC_ASSERT(f->src);
        
        /* Actually register fuse fd in poll plugin */
        f->src->type = TYPE_FD;  
        f->src->fd_src.fd = fuse_session_fd(fuse_get_session(f->handler));
        poll_set_new_evt(&c->ppriv, f->src, ADD);
   }
   return -1;
}

int fuse_process(ctx_t *c) {
    FUSE_PRIV();

    struct fuse_session *sess = fuse_get_session(f->handler);
    if (fuse_session_receive_buf(sess, &f->buf) > 0) {
        fuse_session_process_buf(sess, &f->buf);
        return 0;
    }
    MODULE_DEBUG("Fuse: failed to retrieve buffer.\n");
    return -1;
}

int fuse_end(ctx_t *c) {
    FUSE_PRIV();
    
    /* Deregister fuse fd */
    poll_set_new_evt(&c->ppriv, f->src, RM);
    memhook._free(f->src);
    
    /* Destroy fuse handler */
    fuse_unmount(f->handler);
    fuse_destroy(f->handler);
    
    /* Free other resources */
    memhook._free(f->buf.mem);

    /* Delete folder */
    remove(c->name);
    
    memhook._free(c->fuse);
    c->fuse = NULL;
    return 0;
}
