#include <sys/inotify.h>
int inotify_init(void);
int inotify_add_watch(int fd, const char* pathname, uint32_t mask);
int inotify_rm_watch(int fd, int wd);
//示例
// int fd = inotify_init();
// int wd = inotify_add_watch(fd, "/tmp", IN_ALL_EVENTS);
// char buf[1024];
// ssize_t len = read(fd, buf, sizeof(buf));
// for(char* p = buf; p < buf + len;) {
    // struct inotify_event* event = (struct inotify_event*)p;
// printf("事件：%x，文件名：%s\n", event->mask, event->name);
// p += sizeof(struct inotify_event) + event->len;
// }
// inotify_rm_watch(fd, wd);
// close(fd);