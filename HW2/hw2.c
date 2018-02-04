#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include <errno.h>

#define BUFLEN 1024


int main(int argc, char const *argv[]) {
	if (argc != 2) {
		printf("Usage: %s image_name \n", argv[0]);
		return 1;
	}
    int image = open(argv[1], O_RDWR);
    if (image == -1) {
		printf("There was error opening file %s", argv[1]);
		return 1;
	}

	close(image);
	return 0;
}
