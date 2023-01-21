#include <evcurl/evcurl.h>




int main(int argc, char **argv)
{
    struct ev_loop *loop = ev_loop_new(EVBACKEND_POLL | EVFLAG_NOENV);

    evcurl_processor_t* __evcurl_p = evcurl_create(loop);

    new_conn("http://192.168.88.1/", __evcurl_p);
    new_conn("http://192.168.88.2/", __evcurl_p);
    new_conn("https://mail.ru/", __evcurl_p);
    new_conn("https://fvgwretrmail.ru/", __evcurl_p);

    ev_loop(__evcurl_p->loop, 0);


    new_conn("http://192.168.88.2/", __evcurl_p);
    new_conn("http://192.168.88.4/", __evcurl_p);
    new_conn("http://192.168.88.5/", __evcurl_p);

    ev_loop(__evcurl_p->loop, 0);

    evcurl_destroy(__evcurl_p);

    ev_loop_destroy(loop);
}

