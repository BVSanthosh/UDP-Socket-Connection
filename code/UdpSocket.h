#ifndef __UdpSocket_h__
#define __UdpSocket_h__

#include <stdint.h>
#include <netinet/in.h>

typedef struct UdpSocket_s {
  int                sd;
  struct sockaddr_in addr;
} UdpSocket_t;

typedef struct UdpBuffer_s {
  uint16_t n;         /* number of bytes to send */
  uint8_t *bytes;
} UdpBuffer_t;

UdpSocket_t *setupUdpSocket_t(const char *hostname, const uint16_t port);
/* unicast */
/* hostname == null, port == 0    local end-point, ephemeral port */
/* hostname == null, port != 0    local end-point, designated port */
/* hostname != null, port != 0    remote end-point */

int openUdp(UdpSocket_t *udp);
/* returns 0 and sets udp->sd if OK else returns -1 */

int sendUdp(const UdpSocket_t *local, const UdpSocket_t *remote,
  const UdpBuffer_t *buffer);
/* returns number of bytes sent or -1 on error */

int recvUdp(const UdpSocket_t *local, const UdpSocket_t *remote,
  UdpBuffer_t *buffer);
/* returns number of bytes sent or -1 on error */

void closeUdp(UdpSocket_t *udp);
/* if udp->sd != 0, the close(udp->sd) and free(udp) */

#endif /* __UdpSocket_h__ */