/* Output:
        OPEN    : listening on the specified port
        CLOSE   : refuse connection on the specified port
        TIMEOUT : don't answer on the specified port
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#define PORT_MIN 0
#define PORT_MAX 65535
#define TIMEOUT 1 /* in seconds */

#define DISPLAY_OPEN 0x1
#define DISPLAY_CLOSE 0x2
#define DISPLAY_TIMEOUT 0x4

int main(int argc, char **argv)
{
	int sockfd,pstart,pend;
	char mode;
	struct timeval timeout;
	struct sockaddr_in sain;
	struct hostent *host;

	if (argc < 2)
	{
		printf("usage: %s <IP/host> [<mode> <port start> <port end>]\n"
		       "-modes:"
		       "\t%#.2x display OPEN\n"
		       "\t%#.2x display CLOSE\n"
		       "\t%#.2x display TIMEOUT\n"
		       "\t(default is OPEN)\n",
		       argv[0],DISPLAY_OPEN,DISPLAY_CLOSE,DISPLAY_TIMEOUT);
		exit(1);
	}

	if ((inet_aton(argv[1],&(sain.sin_addr))) == 0)
	{
		if ((host = gethostbyname(argv[1])) == 0)
		{
			perror("gethostbyname");
			exit(1);
		}
		else
		{
			memcpy(&(sain.sin_addr),host->h_addr,
			sizeof(sain.sin_addr));
		}
	}
	
	if (argc > 2)
	{
		mode = (char) atoi(argv[2]);
		if (!(mode & DISPLAY_OPEN) && !(mode & DISPLAY_CLOSE)
		    && !(mode & DISPLAY_TIMEOUT))
		{
			printf("error: wrong mode\n");
			exit(1);
		}
	}
	else
	{
		mode = DISPLAY_OPEN;
	}
	sain.sin_family = AF_INET;
	memset(sain.sin_zero,0,sizeof(sain.sin_zero));

	timeout.tv_sec = TIMEOUT;
	timeout.tv_usec = 0;

	if (argc > 3)
		pstart = atoi(argv[3]);
	else
		pstart = PORT_MIN;
	
	if (argc > 4)
		pend = atoi(argv[4]);
	else
		pend = PORT_MAX;
	
	if (pstart > pend) 
	{
		sockfd = pend;
		pend = pstart;
		pstart = sockfd;
	}

	for (;pstart <= pend;pstart++)
	{
		sain.sin_port = htons(pstart);
		if ((sockfd = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) <= 0)
			perror("socket");

		if (setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,
		    &timeout,sizeof(timeout)))
			perror("setsockopt");

		if (connect(sockfd,(struct sockaddr *)&sain,
		    sizeof(struct sockaddr)))
		{
			if (errno == ECONNREFUSED) 
			{
				if (mode & DISPLAY_CLOSE)
					printf("\015%hu CLOSE\n",
					       ntohs(sain.sin_port)); 
				else
				{
					printf("\015%u",pstart);
					fflush(stdout);
				}
			}
			else if ((errno == ETIMEDOUT) || (errno == EINPROGRESS))
			{
				if (mode & DISPLAY_TIMEOUT)
					printf("\015%hu TIMEOUT\n",
					       ntohs(sain.sin_port));
				else
				{
					printf("\015%u",pstart);
					fflush(stdout);
				}
			}
			else
			{
				perror("connect");
			}
		}
		else
		{
			if (mode & DISPLAY_OPEN)
				printf("\015%hu OPEN\n",ntohs(sain.sin_port));
			else
			{
				printf("\015%u",pstart);
				fflush(stdout);
			}
		}
		close(sockfd);
	}
	return 0;
}
