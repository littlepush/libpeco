// This is a wrapper for epoll under linux

#define core_event_listen           EPOLLIN | EPOLLET
#define core_event_read             EPOLLIN | EPOLLET
#define core_event_write            EPOLLOUT
#define core_flag_add               EPOLL_CTL_ADD
#define core_flag_del               EPOLL_CTL_DEL
#define core_flag_oneshot           EPOLLONESHOT
#define core_flag_eof               EPOLLHUP
#define core_flag_error             EPOLLERR

#define core_fd_create(fd)          (fd) = epoll_create1(0)
#define core_get_event(p)           (p)->events
#define core_get_so(p)              (p)->data.fd
#define core_get_filter(p)          (p)->events

#define core_is_error_event(p)      core_get_event(p) & core_flag_eof || core_get_event(p) & core_flag_error
#define core_is_incoming(p)         core_get_filter(p) & EPOLLIN
#define core_is_outgoing(p)         core_get_filter(p) & EPOLLOUT

inline int __core_event_ctl(
    pe::co::core_event_t* p_core_event, int so, int eid, int core_fd, int flag
) {
    core_get_so(p_core_event) = so;
    core_get_event(p_core_event) = flag;
    if ( -1 == epoll_ctl( core_fd, eid, so, p_core_event ) ) {
        if ( errno == EEXIST ) { 
            return epoll_ctl( core_fd, EPOLL_CTL_MOD, so, p_core_event );
        } else {
            return -1;
        }
    } else {
        return 0;
    }
}

#define core_event_init(e)                  memset(&(_e), 0, sizeof(_e));
#define core_event_add(fd, e, so, f)        __core_event_ctl(&e, so, core_flag_add, fd, f)
#define core_event_del(fd, e, so, f)        __core_event_ctl(&e, so, core_flag_del, fd, f)
#define core_event_wait(fd, es, mc, t)      epoll_wait((fd), (es), (mc), (t))
#define core_has_event(p, e)                core_get_filter(p) & (e)
#define core_make_flag(e, fg)               int _op = (fg)
#define core_mask_flag(e, fg)               core_get_filter(&e) |= (fg)
#define core_event_prepare(e, so)           core_get_filter(&e) |= EPOLLET
#define core_event_ctl_with_flag(fd, so, e, ceid)   __core_event_ctl(&e, so, _op, fd, core_get_filter(&e) | ceid)
