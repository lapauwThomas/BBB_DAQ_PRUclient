/* This code will run 2 threads: one thread writes ADC data to the server, 
* the other thread will fetch the NTP time.
*/
 
#include <stdio.h> //printf
#include <string.h> //strelen
#include <sys/socket.h> //socket
#include <arpa/inet.h> //inet_addr
#include <inttypes.h> //strtoumax
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <linux/if.h>
#include <sys/ioctl.h>


#define macLength 6
#define samplePacketLength 24
#define timestampLength 4


/*	global variable	*/
int pru_adc;
int maxRetries = 5;
uint8_t packet[macLength + samplePacketLength + timestampLength];


/*	define struct to pass arguments to pthread_create()	*/
typedef struct Thread_args_s
{
	int s;
	struct sockaddr_in server_addr;
} thread_args_s;


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
	int i, tmit;
	
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
		i=time(0);
		//printf("%d-%d=%d\n",i,tmit,i-tmit);
		printf("System time is %d seconds off\n",i-tmit);

		
		sleep(1);
	}	
}


/*	24 times 8 channel parallel data is transformed into 8 channel 24bit serial data*/
//this can be optimized by doing in an intelligent fashion
void transpose8(uint8_t A[24], uint8_t B[24]) {
	uint8_t a0, a1, a2, a3, a4, a5, a6, a7;
	for(int i=0;i<3;i++){

		a0 = A[i*8];  a1 = A[i*8+1];  a2 = A[i*8+2];  a3 = A[i*8+3];
		   a4 = A[i*8+4];  a5 = A[i*8+5];  a6 = A[i*8+6];  a7 = A[i*8+7];

		   B[i] 		= (a0 & 128)    | (a1 & 128)/2  | (a2 & 128)/4  | (a3 & 128)/8 |
				   		(a4 & 128)/16 | (a5 & 128)/32 | (a6 & 128)/64 | (a7      )/128;
		   B[i+3]		= (a0 &  64)*2  | (a1 &  64)    | (a2 &  64)/2  | (a3 &  64)/4 |
				   	   (a4 &  64)/8  | (a5 &  64)/16 | (a6 &  64)/32 | (a7 &  64)/64;
		   B[i+3*2] 	= (a0 &  32)*4  | (a1 &  32)*2  | (a2 &  32)    | (a3 &  32)/2 |
				   	   (a4 &  32)/4  | (a5 &  32)/8  | (a6 &  32)/16 | (a7 &  32)/32;
		   B[i+3*3] 	= (a0 &  16)*8  | (a1 &  16)*4  | (a2 &  16)*2  | (a3 &  16)   |
				   	   (a4 &  16)/2  | (a5 &  16)/4  | (a6 &  16)/8  | (a7 &  16)/16;
		   B[i+3*4] 	= (a0 &   8)*16 | (a1 &   8)*8  | (a2 &   8)*4  | (a3 &   8)*2 |
				   	   (a4 &   8)    | (a5 &   8)/2  | (a6 &   8)/4  | (a7 &   8)/8;
		   B[i+3*5]		= (a0 &   4)*32 | (a1 &   4)*16 | (a2 &   4)*8  | (a3 &   4)*4 |
				   	   (a4 &   4)*2  | (a5 &   4)    | (a6 &   4)/2  | (a7 &   4)/4;
		   B[i+3*6] 	= (a0 &   2)*64 | (a1 &   2)*32 | (a2 &   2)*16 | (a3 &   2)*8 |
				   	   (a4 &   2)*4  | (a5 &   2)*2  | (a6 &   2)    | (a7 &   2)/2;
		   B[i+3*7]	 	= (a0      )*128| (a1 &   1)*64 | (a2 &   1)*32 | (a3 &   1)*16|
				   	   (a4 &   1)*8  | (a5 &   1)*4  | (a6 &   1)*2  | (a7 &   1);
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
	uint8_t sampleBuf[samplePacketLength+timestampLength];
	uint8_t dataBuff[samplePacketLength];
	
	printf("Starting main thread\n");
	
	while(1)
	{
		//readpru = read(pru_adc, sampleBuf, samplePacketLength+timestampLength);
    	if( send(sock , dataBuff , 24 , 0) < 0)
    	        {
    	            puts("Send failed");
    	            return 1;
    	        }
    	        
 
        //Send some data
        if( send(sock , message , strlen(message) , 0) < 0)
        {
            puts("Send failed");
            return 1;
        }

        //Receive a reply from the server
        if( recv(sock , server_reply , 2000 , 0) < 0)
        {
            puts("recv failed");
            break;
        }

        puts("Server reply :");
        puts(server_reply);*/
    }

    close(sock);
    return 0;
		

}
	



	
	
	
	
	



