#ifndef __evgpio_h__
#define __evgpio_h__

#include <ev.h>

#if (EV_VERSION_MAJOR <= 4 && EV_VERSION_MINOR < 23)
#error "libev version mistmatch, please upgrade version with EV_PRI (github.com/pianist)"
#endif

typedef void (*evgpio_watcher_cb)(int gpio_num, char state);

int evgpio_watcher_init(struct ev_loop* loop, int gpio_num, evgpio_watcher_cb _cb);


#endif // __evgpio_h__

