#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include <errno.h>

#define BUFLEN 1024
#define BUFTYPE unsigned char

BUFTYPE s[100];

long buff_to_int(BUFTYPE * buff) {
	long * np = (long *) buff;
	return htonl(*np);
}

int main(int argc, char const *argv[]) {
	while (scanf("%s", s) > 0) {
		int n = strlen(s);
		for (int i = 0; i < n; i++)
			printf("%02X ", s[i]);
		puts("");
		printf("%d\n", (int)buff_to_int(s));
	}
}
