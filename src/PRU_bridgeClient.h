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


/*	define struct to pass arguments to pthread_create()	*/
typedef struct Thread_args_s
{
	int s;
	struct sockaddr_in server_addr;
} thread_args_s;


void * ntp( void * ntp_thread_arg );

/*	get MAC address	*/
int getMac(char mac[6]);
