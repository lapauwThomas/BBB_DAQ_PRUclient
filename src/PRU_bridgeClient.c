/* This code will run 2 threads: one thread writes ADC data to the server, 
* the other thread will fetch the NTP time.
*/

/* Version: Sat, 20 May 2017 11:04 */
 
#include "PRU_bridgeClient.h"


#define macLength 6
#define samplePacketLength 24
#define timestampLength 4


/*	global variable	*/
int pru_adc;
int maxRetries = 5;
uint8_t packet[macLength + samplePacketLength + timestampLength];
pthread_mutex_t mutex;
uint32_t ntp_time;


int main(int argc , char *argv[])
{
	/*	Initialise server connection  */
	int sock;
    struct sockaddr_in server;
    //char message[1000] , server_reply[2000];

    //getting mac from ETH0
    char mac[6];
    getMac(mac);

    printf("device MAC: ");

    for (int i = 0; i < 6; i++){
    	printf("%02X:", mac[i]);
    }
    printf("\n");


    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
    if(argc > 1){
    	printf("Connecting to: %s \n", argv[1]);
    	server.sin_addr.s_addr = inet_addr(argv[1]);
    }else{
    	printf("Connecting to localhost \n");
    	server.sin_addr.s_addr = inet_addr("127.0.0.1");
    }
    if(argc > 2){
    	printf("Connecting to port: %s \n", argv[2]);
    	uint16_t portNumber = (uint16_t)atoi(argv[2]);
    	server.sin_port = htons( portNumber );
    }else{
    	printf("Connecting to std port 1520 \n");
    	server.sin_port = htons( 1520 );
    }
    server.sin_family = AF_INET;

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error \n");
        return 1;
    }

    printf("Connected to server\n");

    if( send(sock , mac , 6 , 0) < 0) {
    	printf("Sending MAC failed");
			return 1;
	}



	/*	Initialise NTP part	*/
	pthread_t new_thread;
	char *hostname="85.88.55.5";
	int portno=123;     //NTP is port 123
	//struct in_addr ipaddr;        //
	struct protoent *proto;     //
	struct sockaddr_in server_addr;
	int s;  // socket

	//use Socket;
	//
	//#we use the system call to open a UDP socket
	//socket(SOCKET, PF_INET, SOCK_DGRAM, getprotobyname("udp")) or die "socket: $!";
	proto=getprotobyname("udp");
	s=socket(PF_INET, SOCK_DGRAM, proto->p_proto);
	perror("socket");
	//
	//#convert hostname to ipaddress if needed
	//$ipaddr   = inet_aton($HOSTNAME);
	memset( &server_addr, 0, sizeof( server_addr ));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(hostname);
	//argv[1] );
	//i   = inet_aton(hostname,&server_addr.sin_addr);
	server_addr.sin_port=htons(portno);
	//printf("ipaddr (in hex): %x\n",server_addr.sin_addr);

	/* create struct to pass arguments to new thread */
	thread_args_s ntp_thread_arg;
	ntp_thread_arg.s = s;
	ntp_thread_arg.server_addr = server_addr;

	/* create second thread	*/
	if(pthread_create(&new_thread, NULL, ntp, (void *)(&ntp_thread_arg) ))
	{
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}
	
	/* Create mutex for shared ntp_time variable	*/
	if (pthread_mutex_init(&mutex, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
	


	// open the PRU character device
    ssize_t readpru, pru_adc_command;
    int retries = 0;
    while(retries < maxRetries){
       pru_adc = open("/dev/rpmsg_pru30", O_RDWR);

       if (pru_adc <= 0){
                printf("The pru adc OPEN command failed.\n");
                retries= retries+1;
       }else{
    	   	  //start the PRU DAQ
    	   printf("Starting DAQ \n");
		   pru_adc_command = write(pru_adc, "g", 2);
		   if (pru_adc_command < 0){
			   printf("The pru adc start command failed. \n");
			 close(pru_adc);
			 retries++;
		   }else{
			   retries = 10000; //break from loop
		   }
       }

       if(retries == maxRetries){
    	   printf("The pru adc OPEN command failed. - Stopped after %i retries \n",retries);
    	   return -1;
       }
    }


	//keep communicating with server
	uint8_t hello[] = "hello to server\n";
	send(sock , hello , 24 , 0);
	uint8_t sampleBuf[(samplePacketLength+timestampLength)*17+4]; //+4 for ntp time, which is 32 bit
	uint32_t ntp_time_send;

	printf("Starting main thread\n");

	while(1)
	{
		readpru = read(pru_adc, sampleBuf, samplePacketLength+timestampLength);
		ntp_time_send = ntp_time; //fetch ntp time ASAP when packets arrive
		sampleBuf[(samplePacketLength+timestampLength)*17] = (uint8_t)(ntp_time_send&0xFF000000);
		sampleBuf[(samplePacketLength+timestampLength)*17+1] = (uint8_t)(ntp_time_send&0x00FF0000);
		sampleBuf[(samplePacketLength+timestampLength)*17+2] = (uint8_t)(ntp_time_send&0x0000FF00);
		sampleBuf[(samplePacketLength+timestampLength)*17+3] = (uint8_t)(ntp_time_send&0x000000FF);
    	if( send(sock , sampleBuf , (samplePacketLength+timestampLength)*17+4 , 0) < 0)
		{
			puts("Send failed");
			return 1;
		}

		
			

 /*   	if( send(sock , hello , 24 , 0)  < 0)
		{
			puts("Send failed");
			return 1;
		}
*/

    }
    
    /* some management */
    pthread_join(new_thread, NULL);
    pthread_mutex_destroy(&mutex);
    close(sock);
    return 0;


}







/*	code running in second thread	*/
//void ntp(int s, struct sockaddr_in server_addr)
void * ntp( void * ntp_thread_arg )
{
	//accept arguments from pthread_create()
	thread_args_s * local_args = (thread_args_s *)ntp_thread_arg;
	int s;
	struct sockaddr_in server_addr;
	
	s = local_args->s;
	server_addr = local_args->server_addr;
	

	//local variables second thread
	int maxlen=1024;        //check our buffers
	unsigned long  buf[maxlen]; // the buffer we get back
	
	/*
	 * build a message.  Our message is all zeros except for a one in the
	 * protocol version field
	 * msg[] in binary is 00 001 000 00000000 
	 * it should be a total of 48 bytes long
	*/
	unsigned char msg[48]={010,0,0,0,0,0,0,0,0}; //NTP message
	uint32_t i, tmit;
	
	while(1)
	{
		// send NTP message
		printf("sending data..\n");
		i=sendto(s,msg,sizeof(msg),0,(struct sockaddr *)&server_addr,sizeof(server_addr));
		perror("sendto");
		
		// get response
		struct sockaddr saddr;
		socklen_t saddr_l = sizeof (saddr);
		i=recvfrom(s,buf,48,0,&saddr,&saddr_l);
		perror("recvfr:");

		//We get 12 long words back in Network order
		
		//for(i=0;i<12;i++)
		//	printf("%d\t%-8x\n",i,ntohl(buf[i]));
		
		//for(i=0;i<12;i++) printf("%d\t%-8x\n",i,buf[i]);

		/*
		 * The high word of transmit time is the 10th word we get back
		 * tmit is the time in seconds not accounting for network delays which
		 * should be way less than a second if this is a local NTP server
		 */
		 
		tmit=ntohl((time_t)buf[10]);    //# get transmit time
		//printf("tmit=%d\n",tmit);

		/*
		 * Convert time to unix standard time NTP is number of seconds since 0000
		 * UT on 1 January 1900 unix time is seconds since 0000 UT on 1 January
		 * 1970 There has been a trend to add a 2 leap seconds every 3 years.
		 * Leap seconds are only an issue the last second of the month in June and
		 * December if you don't try to set the clock then it can be ignored but
		 * this is importaint to people who coordinate times with GPS clock sources.
		 */

		tmit-= 2208988800U; 
		//printf("tmit=%d\n",tmit);
		/* use unix library function to show me the local time (it takes care
		 * of timezone issues for both north and south of the equator and places
		 * that do Summer time/ Daylight savings time.
		 */


		//#compare to system time
		printf("Time: %s",ctime(&tmit));
		i = time(0);
		//printf("%d-%d=%d\n",i,tmit,i-tmit);
		printf("System time is %d seconds off\n",i-tmit);

		/* mutex for thread synchrinisation */
		pthread_mutex_lock(&mutex);
		ntp_time = tmit;
		pthread_mutex_unlock(&mutex);
		
		usleep(100000); //sleep for 1/10th of a second
	}	
}




/*	get MAC address	*/
int getMac(char mac[6])
{
  struct ifreq s;
  int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

  strcpy(s.ifr_name, "eth0");
  if (0 == ioctl(fd, SIOCGIFHWADDR, &s)) {
    int i;
    for (i = 0; i < 6; ++i)
      mac[i] = s.ifr_addr.sa_data[i];
    close(fd);
    return 0;
  }
  close(fd);
  return 1;
}


	
	
	


