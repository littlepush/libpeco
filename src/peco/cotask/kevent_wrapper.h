// This is a wrapper for kevent under Mac OS X
#define core_event_listen           EVFILT_READ
#define core_event_read             EVFILT_READ
#define core_event_write            EVFILT_WRITE
#define core_flag_add               EV_ADD
#define core_flag_del               EV_DELETE
#define core_flag_oneshot           EV_ONESHOT
#define core_flag_eof               EV_EOF
#define core_flag_error             EV_ERROR

#define core_fd_create(fd)          (fd) = kqueue()           
#define core_get_event(p)           (p)->flags
#define core_get_so(p)              (p)->ident
#define core_get_filter(p)          (p)->filter

#define core_is_error_event(p)      (core_get_event(p) & core_flag_eof) || (core_get_event(p) & core_flag_error)
#define core_is_incoming(p)         core_get_filter(p) == core_event_read
#define core_is_outgoing(p)         core_get_filter(p) == core_event_write

inline int __core_event_ctl(
    pe::co::core_event_t* p_core_event, int ident, int eid, int core_fd, int flag
) {
    EV_SET(p_core_event, ident, eid, flag, 0, 0, NULL);
    return kevent(core_fd, p_core_event, 1, NULL, 0, NULL);
}

#define core_event_init(e)
#define core_event_add(fd, e, so, f)    __core_event_ctl(&e, so, f, fd, core_flag_add)
#define core_event_del(fd, e, so, f)    __core_event_ctl(&e, so, f, fd, core_flag_del)

inline int core_event_wait(
    int fd, pe::co::core_event_t* p_core_event, int max, uint32_t timedout
) {
    struct timespec __ts = { timedout / 1000, timedout % 1000 * 1000 * 1000 };
    return kevent(fd, NULL, 0, p_core_event, max, &__ts);
}
#define core_has_event(p, e)                        core_get_filter(p) == (e)
#define core_make_flag(e, fg)                       unsigned short _flags = (fg)
#define core_mask_flag(e, fg)                       _flags |= (fg)
#define core_event_prepare(e, so)
#define core_event_ctl_with_flag(fd, so, e, ceid)   __core_event_ctl(&e, so, ceid, fd, _flags)
