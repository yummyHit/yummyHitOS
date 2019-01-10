/*
 * NetworkTransfer.c
 *
 *  Created on: 2017. 8. 16.
 *      Author: Yummy
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

// 기타 매크로
#define DWORD			unsigned int
#define BYTE			unsigned char
#define _MIN(x, y)		(((x) < (y)) ? (x) : (y))

// 시리얼 포트 FIFO의 최대 크기
#define SERIAL_FIFO_MAXSIZE	16

int main(int argc, char **argv) {
	char fileName[256], dataBuf[SERIAL_FIFO_MAXSIZE];
	struct sockaddr_in sockAddr;
	int sock;
	BYTE ack;
	DWORD dataLen, size = 0, tmp;
	FILE *fp;

	// 파일 열기, 파일 이름 입력받음
	if(argc < 2) {
		fprintf(stderr, "Input File Name: ");
		gets(fileName);
	} else strcpy(fileName, argv[1]);	// 파일 이름을 매개변수로 넣었으면 복사

	// 파일 열기 시도
	fp = fopen(fileName, "rb");
	if(fp == NULL) {
		fprintf(stderr, "%s File Open Error !!\n", fileName);
		return 0;
	}

	// fseek() 함수를 이용해 파일 끝으로 이동 후 파일 길이 측정, 그리도 다시 파일의 처음으로 이동
	fseek(fp, 0, SEEK_END);
	dataLen = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fprintf(stderr, "File Name %s, Data Length %d Byte\n", fileName, dataLen);

	// 네트워크 접속, 접속할 QEMU의 주소 설정
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(7777);
	sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// 소켓 생성 후 QEMU에 접속 시도
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(connect(sock, (struct sockaddr*)&sockAddr, sizeof(sockAddr)) == -1) {
		fprintf(stderr, "Socket Connect Error, IP : 127.0.0.1 / Port : 7777\n");
		return 0;
	} else fprintf(stderr, "Socket Connect Success, IP : 127.0.0.1 / Port : 7777\n");

	// 데이터 전송, 데이터 길이 전송
	if(send(sock, &dataLen, 4, 0) != 4) {
		fprintf(stderr, "Data Length Send Fail, [%d] Byte...\n", dataLen);
		return 0;
	} else fprintf(stderr, "Data Length Send Succes, [%d] Byte !!\n", dataLen);
	// ACK를 수신할 때까지 대기
	if(recv(sock, &ack, 1, 0) != 1) {
		fprintf(stderr, "Ack Receive Error !!\n");
		return 0;
	}

	// 데이터 전송
	fprintf(stderr, "Now Data Transfer...");
	while(size < dataLen) {
		// 남은 크기와 FIFO의 최대 크기 중 작은 것 선택
		tmp = _MIN(dataLen - size, SERIAL_FIFO_MAXSIZE);
		size += tmp;

		if(fread(dataBuf, 1, tmp, fp) != tmp) {
			fprintf(stderr, "File Read Error !!\n");
			return 0;
		}

		// 데이터 전송
		if(send(sock, dataBuf, tmp, fp) != tmp) {
			fprintf(stderr, "Socket Send Error !!\n");
			return 0;
		}

		// ACK가 수신될 때까지 대기
		if(recv(sock, &ack, 1, 0) != 1) {
			fprintf(stderr, "Ack Receive Error !!\n");
			return 0;
		}
		// 진행 상황 표시
		fprintf(stderr, "#");
	}

	// 파일과 소켓 닫음
	fclose(fp);
	close(sock);

	// 전송 완료를 표시하고 엔터 키 대기
	fprintf(stderr, "\nSend Complete. [%d] Byte\n", size);
	fprintf(stderr, "Press Enter Key to Exit...\n");
	getchar();

	return 0;
}
