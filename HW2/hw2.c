#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include <errno.h>

#define BUFLEN 1024
#define BUFTYPE unsigned char
#define _asssert(x) if (x == -1) exit(1)
#define bool _Bool
#define true 1
#define false 0

#define IHDR 1229472850
#define IDAT 1229209940
#define IEND 1229278788

#define FIX_CURE 232

void * read_to_buffer(int fd, int len) {
	BUFTYPE * buff = calloc(BUFLEN, sizeof(BUFTYPE));
	int n = read(fd, buff, len);
	if (n == -1) {
		fprintf(stderr, "Probem reading file");
		exit(1);
	}
	if (n != len) {
		fprintf(stderr, "Couldn't read requested number of bytes (%d, %d)\n", len, n);
		exit(1);
	}
	// for (int i = 0; i < n; i++)
	// 	buff[i] = htons(buff[i]);
	return buff;
}

void print_buff(BUFTYPE * buff, int len) {
	for (int i = 0; i < len; i++)
		printf("%02X ", buff[i]);
	puts("");
}

void print_str(BUFTYPE * buff, int len) {
	for (int i = 0; i < len; i++)
		putchar(buff[i]);
	puts("");
}

long buff_to_int(BUFTYPE * buff) {
	long * np = (long *) buff;
	return htonl(*np);
}

void fix_buf(BUFTYPE * buff, int len) {
	for (int i = 0; i < len; i++)
		buff[i] ^= FIX_CURE;
}

bool deal_with_chunk(int fd) {
	BUFTYPE * buf = read_to_buffer(fd, 8);
	int offset = 0;
	print_buff(buf, 8);
	print_str(buf + 4, 4);
	int len = buff_to_int(buf);
	int cd = buff_to_int(buf + 4);
	printf("%d %d\n", len, cd);
	if (cd == IEND)
		return 0;
	if (cd != IDAT) {
		offset = lseek(fd, len + 4, SEEK_CUR);
		_asssert(offset);
		return 1;
	}
	free(buf);
	buf = read_to_buffer(fd, len);
	fix_buf(buf, len);
	offset = lseek(fd, -len, SEEK_CUR);
	_asssert(offset);
	offset = write(fd, buf, len);
	_asssert(offset);
	offset = lseek(fd, 4, SEEK_CUR);
	_asssert(offset);
	free(buf);
	return 1;
}

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
	int offset = lseek(image, 8, SEEK_CUR);
	_asssert(offset);

	while (deal_with_chunk(image)) {

	}

	close(image);
	return 0;
}
