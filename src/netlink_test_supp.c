
#include <stdio.h>
#include <string.h>

#include <bmon/bmon.h>
#include <bmon/conf.h>
#include <bmon/attr.h>
#include <bmon/utils.h>
#include <bmon/input.h>
#include <bmon/output.h>
#include <bmon/module.h>
#include <bmon/group.h>
#include <bmon/element.h>
#include <bmon/input.h>
#include <bmon/module.h>
#include <bmon/utils.h>

#include <linux/if.h>
#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/utils.h>
#include <netlink/route/link.h>
#include <netlink/route/tc.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/class.h>
#include <netlink/route/classifier.h>
#include <netlink/route/qdisc/htb.h>

#define max(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })

static struct nl_sock *sock;
static struct nl_cache *link_cache, *qdisc_cache;


static struct attr_map link_attrs[] = {
{
    .name		= "bytes",
    .type		= ATTR_TYPE_COUNTER,
    .unit		= UNIT_BYTE,
    .description	= "Bytes",
    .rxid		= RTNL_LINK_RX_BYTES,
    .txid		= RTNL_LINK_TX_BYTES,
},
{
    .name		= "packets",
    .type		= ATTR_TYPE_COUNTER,
    .unit		= UNIT_NUMBER,
    .description	= "Packets",
    .rxid		= RTNL_LINK_RX_PACKETS,
    .txid		= RTNL_LINK_TX_PACKETS,
}
};

static struct attr_map tc_attrs[] = {
{
    .name		= "tc_bytes",
    .type		= ATTR_TYPE_COUNTER,
    .unit		= UNIT_BYTE,
    .description	= "Bytes",
    .rxid		= -1,
    .txid		= RTNL_TC_BYTES,
},
{
    .name		= "tc_packets",
    .type		= ATTR_TYPE_COUNTER,
    .unit		= UNIT_NUMBER,
    .description	= "Packets",
    .rxid		= -1,
    .txid		= RTNL_TC_PACKETS,
}
};

int netlink_do_init(void)
{
    int err;

    if (!(sock = nl_socket_alloc())) {
        fprintf(stderr, "Unable to allocate netlink socket\n");
        goto disable;
    }

    if ((err = nl_connect(sock, NETLINK_ROUTE)) < 0) {
        fprintf(stderr, "Unable to connect netlink socket: %s\n", nl_geterror(err));
        goto disable;
    }

    if ((err = rtnl_link_alloc_cache(sock, AF_UNSPEC, &link_cache)) < 0) {
        fprintf(stderr, "Unable to allocate link cache: %s\n", nl_geterror(err));
        goto disable;
    }

    if ((err = rtnl_qdisc_alloc_cache(sock, &qdisc_cache)) < 0) {
        fprintf(stderr, "Warning: Unable to allocate qdisc cache: %s\n", nl_geterror(err));
        fprintf(stderr, "Disabling QoS statistics.\n");
        qdisc_cache = NULL;
    }

    if (attr_map_load(link_attrs, ARRAY_SIZE(link_attrs)) ||
        attr_map_load(tc_attrs, ARRAY_SIZE(tc_attrs)))
        BUG();

    return 0;

disable:
    return -EOPNOTSUPP;
}


void do_link(struct nl_object *obj, void *arg)
{
    static uint64_t c_rx_s = 0 , c_tx_s = 0;
    uint64_t c_rx = 0, c_tx = 0;
    struct rtnl_link *link = (struct rtnl_link *) obj;
    int i;

    if(!(rtnl_link_get_flags(link) & IFF_UP)) BUG();

    for (i = 0; i < ARRAY_SIZE(link_attrs); i++) {
        struct attr_map *m = &link_attrs[i];
        int flags = 0;

        if (m->rxid >= 0) {
            c_rx = rtnl_link_get_stat(link, m->rxid);
            flags |= UPDATE_FLAG_RX;
        }

        if (m->txid >= 0) {
            c_tx = rtnl_link_get_stat(link, m->txid);
            flags |= UPDATE_FLAG_TX;
        }
    }

    char *name = rtnl_link_get_name(link);
    if ( strcmp(name,"eno1")==0 ) {
        printf(" %s ",name);
        // printf( " -> %lu - %lu \n",(c_rx - c_rx_s),(c_tx - c_tx_s));
        uint64_t rx = c_rx-c_rx_s;
        uint64_t tx = c_tx-c_tx_s;
        if(c_rx_s && c_tx_s) {
            for(int i=0;i<min(rx,tx);++i)
                printf("X");
            for(int i=0;i<rx-min(rx,tx);++i)
                printf("/");
            for(int i=0;i<tx-min(rx,tx);++i)
                printf("\\");
            printf( "\n");
        }
        c_rx_s = c_rx;
        c_tx_s = c_tx;
    }
}

void netlink_read(void)
{
    int err;

    if ((err = nl_cache_resync(sock, link_cache, NULL, NULL)) < 0) {
        fprintf(stderr, "Unable to resync link cache: %s\n", nl_geterror(err));
        goto disable;
    }

    if (qdisc_cache &&
        (err = nl_cache_resync(sock, qdisc_cache, NULL, NULL)) < 0) {
        fprintf(stderr, "Unable to resync qdisc cache: %s\n", nl_geterror(err));
        goto disable;
    }

    nl_cache_foreach(link_cache, do_link, NULL);

    return;

disable:
    BUG();
//    netlink_ops.m_flags &= ~BMON_MODULE_ENABLED;
}







