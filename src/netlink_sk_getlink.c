#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/netlink.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>





// struct ifinfomsg {
//    unsigned char  ifi_family; /* AF_UNSPEC */
//    unsigned short ifi_type;   /* Device type */
//    int            ifi_index;  /* Interface index */
//    unsigned int   ifi_flags;  /* Device flags  */
//    unsigned int   ifi_change; /* change mask */
// };


int main() {
      struct {
              struct nlmsghdr n;
              struct ifinfomsg r;
      } req;

      int status;
      char buf[16384];
      struct nlmsghdr *nlmp;
      struct ifinfomsg *rtmp;
      struct rtattr *rtatp;
      int rtattrlen;

      // SOCKET //
      int fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);

      memset(&req, 0, sizeof(req));
      req.n.nlmsg_len   = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
      req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
      req.n.nlmsg_type  = RTM_GETLINK;

      // SEND //
      status = send(fd, &req, req.n.nlmsg_len, 0);
      if (status < 0) {
              perror("send");
              return 1;
      }

      // RECV //
      status = recv(fd, buf, sizeof(buf), 0);
      if (status < 0) {
              perror("recv");
              return 1;
      }
      if(status == 0){
              printf("EOF\n");
              return 1;
      }

      // Loop MESSAGES //
      for(nlmp = (struct nlmsghdr *)buf; status > sizeof(*nlmp);){
              int len = nlmp->nlmsg_len;
              int req_len = len - sizeof(*nlmp);

              if (req_len<0 || len>status) {
                      printf("error\n");
                      return -1;
              }

              if (!NLMSG_OK(nlmp, status)) {
                      printf("NLMSG not OK\n");
                      return 1;
              }

              rtmp = (struct ifinfomsg *)NLMSG_DATA(nlmp);
              rtatp = (struct rtattr *)IFLA_RTA(rtmp);
              rtattrlen = IFLA_PAYLOAD(nlmp);

              // printf("Index Of Iface= %d, %d\n",rtmp->ifi_index,rtattrlen);

              // LOOP ATTRIBUTES //
              for (; RTA_OK(rtatp, rtattrlen); rtatp = RTA_NEXT(rtatp, rtattrlen)) {

                  //        rta_type         value type         description
                  //        ──────────────────────────────────────────────────────────
                  //        IFLA_UNSPEC      -                  unspecified.
                  //        IFLA_ADDRESS     hardware address   interface L2 address
                  //        IFLA_BROADCAST   hardware address   L2 broadcast address.
                  //        IFLA_IFNAME      asciiz string      Device name.
                  //        IFLA_MTU         unsigned int       MTU of the device.
                  //        IFLA_LINK        int                Link type.
                  //        IFLA_QDISC       asciiz string      Queueing discipline.
                  //        IFLA_STATS       see below          Interface Statistics.

                  if(rtatp->rta_type == IFLA_IFNAME){
                      printf("ifname -> %s \t",(char*)RTA_DATA(rtatp));
                  }
                  if(rtatp->rta_type == IFLA_STATS){
                      struct rtnl_link_stats *stat = (struct rtnl_link_stats *)RTA_DATA(rtatp);
                      printf(" tx: %lu - rx: %lu \n",stat->rx_bytes, stat->tx_bytes);
                  }
              }
              status -= NLMSG_ALIGN(len);
              nlmp = (struct nlmsghdr*)((char*)nlmp + NLMSG_ALIGN(len));
      }
      return 0;
}
