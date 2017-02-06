#include <iostream>

extern "C" {
#include <bmon/bmon.h>
#include <bmon/conf.h>
#include <bmon/module.h>

int netlink_do_init(void);
void netlink_read(void);

} // C


int main() {

    conf_init_pre();
    module_init();
    conf_init_post();

    netlink_do_init();


    for(;;) {
        netlink_read();
        usleep(50000);
    }

    return 0;
}
