/*
 ============================================================================
 Name        : Client_test2.c
 Author      : t
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdlib.h>

/*
    C ECHO client example using sockets
*/
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <inttypes.h> /* strtoumax */
#include <stdbool.h>
#include <netdb.h>
#include <linux/if.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <netdb.h>
#include <stdio.h>


int pru_adc;

int maxRetries = 5;

#define macLength 6
#define samplePacketLength 24
#define timestampLength 4


uint8_t packet[macLength + samplePacketLength + timestampLength];

int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char message[1000] , server_reply[2000];


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

    ssize_t readpru, pru_adc_command;
    int retries = 0;
    while(retries < maxRetries){
    // open the PRU character device
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
       uint8_t sampleBuf[17*(24+4)];
       uint8_t dataBuff[samplePacketLength];
    while(1)
    {

    	readpru = read(pru_adc, sampleBuf, 17*(24+4));

    	//transpose8(sampleBuf,dataBuff); //transpose the 24 data bytes

    	//memcpy(packet, mac, macLength); //add mac to packet
    //	memcpy(packet+macLength, dataBuff, samplePacketLength); //copy the data bytes to packet
    //	memcpy(packet, mac, macLength); //add timestamp to
    //	puts("Sending data \n");
    	if( send(sock , sampleBuf , 17*(24+4) , 0) < 0)
    	        {
    	            puts("Send failed");
    	            return 1;
    	        }

        /*printf("Enter message : ");
        scanf("%s" , message);

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

