//Struct for protocol packet 
typedef struct MyProtocol_pkt_s {
  uint16_t pktnum;
    uint16_t timestamp; //stores the timestamp
	uint16_t  seqnum;   //unique sequence number 
  uint8_t payload[31];
} MyProtocol_pkt_t;

int setAsyncFd(int fd);

void handleSIGIO(int sig);

void setupSIGIO();

void checkNetwork();

void handleSIGALRM(int sig);

void setupSIGALRM();

void setITIMER(uint32_t sec, uint32_t usec);

MyProtocol_pkt_t createUDP();

uint16_t getTimeStamp();