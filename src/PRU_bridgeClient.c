/*
 ============================================================================
 Name        : Client_test2.c
 Author      : t
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

/*
int main(void) {
	puts("!!!Hello World met bananenbroodbomen!!!");  prints !!!Hello World!!!
	return EXIT_SUCCESS;
}
*/
/*
    C ECHO client example using sockets
*/
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <fcntl.h>
#include <sys/types.h>

int pru_adc;

int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char message[1000] , server_reply[2000];

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    server.sin_addr.s_addr = inet_addr("192.168.7.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 1520 );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    puts("Connected\n");


    ssize_t readpru, pru_adc_command;
    //  Now open the PRU1 clock control char device and start the clock.
       pru_adc = open("/dev/rpmsg_pru30", O_RDWR);
       if (pru_adc_command < 0){
                puts("The pru adc OPEN command failed.");
                return -1;
       }

       pru_adc_command = write(pru_adc, "g", 2);
       if (pru_adc_command < 0){
         puts("The pru adc start command failed.");
         return -1;
       }

    //keep communicating with server
       uint8_t hello[] = "hello to server\n";
       send(sock , hello , 24 , 0);
       uint8_t sampleBuf[24];
       uint8_t dataBuff[24];
    while(1)
    {




    	puts("reading data \n");
    	readpru = read(pru_adc, sampleBuf, 24);
    	transpose8(sampleBuf,dataBuff);

    	puts("Sending data \n");
    	if( send(sock , dataBuff , 24 , 0) < 0)
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
