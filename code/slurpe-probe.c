#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h> 
#include <sys/time.h> 
#include <stdlib.h>
#include "UdpSocket.h"
#include "slurpe-probe.h"

#define G_MY_PORT    getuid()
#define G_SIZE ((uint32_t) 256) //Number of bytes to send accross the network
#define SIZEOF_MyProtocol_pkt_maximum_payload 32
#define G_ITIMER_S   ((uint32_t) 1) // seconds
#define G_ITIMER_US  ((uint32_t) 0) // microseconds
#define SIZEOF_MyProtocol_pkt SIZEOF_MyProtocol_pkt_maximum_payload

/* descriptors to watch */
int G_net;

/* signals to block - signal handlers add to this mask */
sigset_t G_sigmask;

/* signal action / handler hooks */
struct sigaction G_sigio, G_sigalrm;

/* other globals */
UdpSocket_t *G_local;
UdpSocket_t *G_remote;
MyProtocol_pkt_t *p;

struct itimerval G_timer;
int G_flag;
int seqcounter = 10000;
int pktcounter = 0;

int main(int argc, char *argv[]){

  if (argc != 2) {
    printf("Please check the command line arguments");
    exit(0);
  }

  if ((G_local = setupUdpSocket_t((char *) 0, G_MY_PORT)) == (UdpSocket_t *) 0) {
    printf("local problem");
    exit(0);
  }

    if ((G_remote = setupUdpSocket_t(argv[1], G_MY_PORT)) == (UdpSocket_t *) 0) {
    printf("remote hostname/port problem");
    exit(0);
  }

  if (openUdp(G_local) < 0) {
    printf("openUdp() problem");
    exit(0);
  }
  
  /* initialise global vars */
  G_net = G_local->sd;
  sigemptyset(&G_sigmask);
  
  /* setup signal handlers */
  setupSIGIO(); //Handles receiving UDP
  setupSIGALRM(); //Handles sending UDP
  
  //Set up loop to wait for signal
  G_flag = 0;
  while(!G_flag) {
    (void) pause(); 
  }

  /* tidy up */
  closeUdp(G_local);
  closeUdp(G_remote);
  free(p);

  return 0;
}

void handleSIGIO(int sig){

  if (sig == SIGIO) {

    /* protect the network and keyboard reads from signals */
    sigprocmask(SIG_BLOCK, &G_sigmask, (sigset_t *) 0);

    checkNetwork();

    /* allow the signals to be delivered */
    sigprocmask(SIG_UNBLOCK, &G_sigmask, (sigset_t *) 0);
  }
  else {
    printf("handleSIGIO(): got a bad signal number");
  }
}

void setupSIGIO(){

  sigaddset(&G_sigmask, SIGIO);

  G_sigio.sa_handler = handleSIGIO;
  G_sigio.sa_flags = 0;

  if (sigaction(SIGIO, &G_sigio, (struct sigaction *) 0) < 0) {
    printf("setupSIGIO(): sigaction() problem");
    exit(0);
  }

  if (setAsyncFd(G_net) < 0) {
    printf("setupSIGIO(): setAsyncFd(G_net) problem");
    exit(0);
  }
}

void setITIMER(uint32_t sec, uint32_t usec){

  G_timer.it_interval.tv_sec = sec;
  G_timer.it_interval.tv_usec = usec;
  G_timer.it_value.tv_sec = sec;
  G_timer.it_value.tv_usec = usec;

  if (setitimer(ITIMER_REAL, &G_timer, (struct itimerval *) 0) != 0) {
    printf("setITIMER(): setitimer() problem");
  }
}

void handleSIGALRM(int sig){

  UdpBuffer_t buffer;
  buffer.bytes = malloc(sizeof(p));

  if (sig == SIGALRM) {

    /* protect handler actions from signals */
    sigprocmask(SIG_BLOCK, &G_sigmask, (sigset_t *) 0);

    //create UDP packet
    createUDP();

    printf("Packet Number: %d \n", p->pktnum);
    printf("Timestamp: %d \n", p->timestamp);
    printf("Sequence Number: %d \n", p->seqnum);
    printf("Payload: ");
    for(int counter = 0; counter < SIZEOF_MyProtocol_pkt_maximum_payload; ++counter){
      printf("%i", (int) (p->payload)[counter]);
    }
    printf("\n");

    // Convert values to network (canonical) byte order
    p->pktnum = htons(p->pktnum); 
    p->seqnum = htons(p->seqnum);
    p->timestamp = htons(p->timestamp);

    memcpy(buffer.bytes, p, sizeof(MyProtocol_pkt_t));
    buffer.n = sizeof(MyProtocol_pkt_t);

    printf("<-");
    for(int counter = 0; counter < sizeof(MyProtocol_pkt_t); ++counter){
      printf("%d", buffer.bytes[counter]);
    }
    printf(" (data stream to the network)");
    printf("\n");

    //Send UDP packet
    if (sendUdp(G_local, G_remote, &buffer) != buffer.n) {
      printf("SendUdp() problem");
    }

    /* protect handler actions from signals */
    sigprocmask(SIG_UNBLOCK, &G_sigmask, (sigset_t *) 0);
  }
  else {
    printf("handleSIGALRM() got a bad signal");
  }
}
void setupSIGALRM(){

  sigaddset(&G_sigmask, SIGALRM);

  G_sigalrm.sa_handler = handleSIGALRM;
  G_sigalrm.sa_flags = 0;

  if (sigaction(SIGALRM, &G_sigalrm, (struct sigaction *) 0) < 0) {
    perror("setupSIGALRM(): sigaction() problem");
    exit(0);
  }
  else {
    setITIMER(G_ITIMER_S, G_ITIMER_US);
  }
} 

void checkNetwork(){

  MyProtocol_pkt_t pkt;
  UdpSocket_t receive;
  UdpBuffer_t buffer;
  unsigned char bytes[sizeof(MyProtocol_pkt_t)];
  int r;

  /* print any network input for now, will save to a file later*/
  buffer.bytes = bytes;
  buffer.n = sizeof(MyProtocol_pkt_t);

  if ((r = recvUdp(G_local, &receive, &buffer)) < 0) {
    if (errno != EWOULDBLOCK) {
      printf("checkNetwork(): recvUdp() problem");
    }
  }
  else {

    memcpy(&pkt, buffer.bytes, sizeof(MyProtocol_pkt_t));

    printf("Packet Number: %d \n", ntohs(pkt.pktnum));
    printf("Timestamp: %d\n", ntohs(pkt.timestamp));
    printf("Sequence Number: %d\n", ntohs(pkt.seqnum));
    printf("Payload:");
    for(int counter = 0; counter < sizeof(MyProtocol_pkt_t); ++counter){
        printf("%d", (int) (p->payload)[counter]);
    }
    printf("\n");

    printf("->");
    for(int counter = 0; counter < sizeof(MyProtocol_pkt_t); ++counter){
        printf("%d", buffer.bytes[counter]);
    }
    printf(" (data stream from the network)");
    printf("\n");
  }
}

int setAsyncFd(int fd){

  int r, flags = O_NONBLOCK | O_ASYNC; // man 2 fcntl

  if (fcntl(fd, F_SETOWN, getpid()) < 0) {
    printf("setAsyncFd(): fcntl(fd, F_SETOWN, getpid()");
    exit(0);
  }

  if ((r = fcntl(fd, F_SETFL, flags)) < 0) {
    printf("setAsyncFd(): fcntl() problem");
    exit(0);
  }

  return r;
}

MyProtocol_pkt_t createUDP(){

  p = malloc(sizeof(MyProtocol_pkt_t));

  //Set header value
  ++seqcounter;
  p->pktnum = 0;
  p->seqnum = 0;
  p->pktnum = ++pktcounter;
  p->seqnum  = p->seqnum + seqcounter;
  p->timestamp = getTimeStamp();
  
  //Set packet content
  for(int i = 0; i < SIZEOF_MyProtocol_pkt_maximum_payload; ++i){  // values 0 ... 31 in payload bytes
    p->payload[i] = i;
  } 
}

uint16_t getTimeStamp(){

  struct timespec time;

  clock_gettime(CLOCK_REALTIME, &time);
  uint16_t time_ms = time.tv_sec * 1000 + time.tv_nsec / 1000000;
  return time_ms;
}