#pragma comment(lib, "ws2_32.lib")

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define FNameMax 32 /* Max length of File Name */
#define FileMax 32 /* Max number of Files */
#define baseM 6 /* base number */
#define ringSize 64 /* ringSize = 2^baseM */
#define fBufSize 1024 /* file buffer size */
#define PRIME_MULT 1717

#define mainThread 0
#define procRecvMsgThread 1
#define FixFingerThread 2
#define PingPongThread 3

#define typetemp 0
#define typebuffer 1

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <winsock2.h>
#include <string.h>
#include <windows.h>
#include <process.h> 

/*자료의 구조체*/
typedef struct { /* Node Info Type Structure */
	int ID; /* ID */
	struct sockaddr_in addrInfo; /* Socket address */
} nodeInfoType;

typedef struct { /* File Type Structure */
	char Name[FNameMax]; /* File Name */
	int Key; /* File Key */
	nodeInfoType owner; /* Owner's Node */
	nodeInfoType refOwner; /* Ref Owner's Node */
} fileRefType;

typedef struct { /* Global Information of Current Files */
	unsigned int fileNum; /* Number of files */
	fileRefType fileRef[FileMax]; /* The Number of Current Files */
} fileInfoType;

typedef struct { /* Finger Table Structure */
	nodeInfoType Pre; /* Predecessor pointer */
	nodeInfoType finger[baseM]; /* Fingers (array of pointers) */
} fingerInfoType;

typedef struct { /* Chord Information Structure */
	fileInfoType FRefInfo; /* File Ref Own Information */
	fingerInfoType fingerInfo; /* Finger Table Information */
} chordInfoType;

typedef struct { /* Node Structure */
	nodeInfoType nodeInfo; /* Node's IPv4 Address */
	fileInfoType fileInfo; /* File Own Information */
	chordInfoType chordInfo; /* Chord Data Information */
} nodeType;

typedef struct {
	unsigned short msgID; // message ID
	unsigned short msgType; // message type (0: request, 1: response)
	nodeInfoType nodeInfo;// node address info
	short moreInfo; // more info
	fileRefType fileInfo; // file (reference) info
	unsigned int bodySize; // body size in Bytes
} chordHeaderType; // CHORD message header type

static const unsigned char sTable[256] =
{
	0xa3,0xd7,0x09,0x83,0xf8,0x48,0xf6,0xf4,0xb3,0x21,0x15,0x78,0x99,0xb1,0xaf,0xf9,
	0xe7,0x2d,0x4d,0x8a,0xce,0x4c,0xca,0x2e,0x52,0x95,0xd9,0x1e,0x4e,0x38,0x44,0x28,
	0x0a,0xdf,0x02,0xa0,0x17,0xf1,0x60,0x68,0x12,0xb7,0x7a,0xc3,0xe9,0xfa,0x3d,0x53,
	0x96,0x84,0x6b,0xba,0xf2,0x63,0x9a,0x19,0x7c,0xae,0xe5,0xf5,0xf7,0x16,0x6a,0xa2,
	0x39,0xb6,0x7b,0x0f,0xc1,0x93,0x81,0x1b,0xee,0xb4,0x1a,0xea,0xd0,0x91,0x2f,0xb8,
	0x55,0xb9,0xda,0x85,0x3f,0x41,0xbf,0xe0,0x5a,0x58,0x80,0x5f,0x66,0x0b,0xd8,0x90,
	0x35,0xd5,0xc0,0xa7,0x33,0x06,0x65,0x69,0x45,0x00,0x94,0x56,0x6d,0x98,0x9b,0x76,
	0x97,0xfc,0xb2,0xc2,0xb0,0xfe,0xdb,0x20,0xe1,0xeb,0xd6,0xe4,0xdd,0x47,0x4a,0x1d,
	0x42,0xed,0x9e,0x6e,0x49,0x3c,0xcd,0x43,0x27,0xd2,0x07,0xd4,0xde,0xc7,0x67,0x18,
	0x89,0xcb,0x30,0x1f,0x8d,0xc6,0x8f,0xaa,0xc8,0x74,0xdc,0xc9,0x5d,0x5c,0x31,0xa4,
	0x70,0x88,0x61,0x2c,0x9f,0x0d,0x2b,0x87,0x50,0x82,0x54,0x64,0x26,0x7d,0x03,0x40,
	0x34,0x4b,0x1c,0x73,0xd1,0xc4,0xfd,0x3b,0xcc,0xfb,0x7f,0xab,0xe6,0x3e,0x5b,0xa5,
	0xad,0x04,0x23,0x9c,0x14,0x51,0x22,0xf0,0x29,0x79,0x71,0x7e,0xff,0x8c,0x0e,0xe2,
	0x0c,0xef,0xbc,0x72,0x75,0x6f,0x37,0xa1,0xec,0xd3,0x8e,0x62,0x8b,0x86,0x10,0xe8,
	0x08,0x77,0x11,0xbe,0x92,0x4f,0x24,0xc5,0x32,0x36,0x9d,0xcf,0xf3,0xa6,0xbb,0xac,
	0x5e,0x6c,0xa9,0x13,0x57,0x25,0xb5,0xe3,0xbd,0xa8,0x3a,0x01,0x05,0x59,0x2a,0x46
};

//전역변수//////
nodeType myNode = { 0 };
SOCKET rqSock, rpSock, flSock, fsSock[10], ppSock, ffSock, frSock;
WSADATA wsaData;
int netCheck = 0;
int exitTmp = 0;
int *exitFlag = &exitTmp;
HANDLE hMutex;
chordHeaderType File = { 0 }; //파일 이름 넣는 전역변수

int modPlus(int modN, int addend1, int addend2);
int modMinus(int modN, int minuend, int subtrand);
int twoPow(int power);
int modIn(int modN, int targNum, int range1, int range2, int leftmode, int rightmode);
unsigned int str_hash(const char *str);
void showComand(void);

unsigned WINAPI procRecvMsg(void *arg);
unsigned WINAPI procFixFinger(void *arg);
unsigned WINAPI procPingPong(void *arg);
unsigned WINAPI fileReceiver(void *arg);

int recvn(SOCKET s, char *buf, int len, int flags);

void msgNotice(int num, int type);
void filePrint(nodeType curNode);
int mute = 0; //뮤트
			  ////////////////


			  //파일 관련
FILE *r_fp[10];
int sendCount = 0;
unsigned WINAPI fileSender(chordHeaderType* temp);
//////////////
void main(int argc, char *argv[]) {

	char ip[32];
	char strInput = "";

	HANDLE hThread[4];
	chordHeaderType temp, buffer;

	int retVal;
	int port = 0;
	int optVal = 3000;
	int closetCheck = 0;
	int keyNum = 0;
	int moveflag = 0;
	int down = 0;
	int deleteCheck = 0;

	char fileName[50];
	int fileID;

	struct sockaddr_in myAddr;
	struct sockaddr_in predAddr;
	struct sockaddr_in closetpredAddr;
	struct sockaddr_in moveKeyAddr;
	struct sockaddr_in ownerAddr;


	myAddr.sin_family = AF_INET;
	predAddr.sin_family = AF_INET;
	closetpredAddr.sin_family = AF_INET;
	moveKeyAddr.sin_family = AF_INET;
	ownerAddr.sin_family = AF_INET;

	hMutex = CreateMutex(NULL, FALSE, NULL);

	nodeInfoType predNode;
	nodeInfoType fileSuccNode;
	nodeInfoType fileRefNode;


	//윈속 초기화
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { // Load Winsock 2.2 DLL 
		fprintf(stderr, "WSAStartup() failed");
		exit(1);
	}

	//1. 전역변수 myNode 설정[현재 노드]
	myNode.nodeInfo.addrInfo.sin_family = AF_INET;
	myNode.nodeInfo.addrInfo.sin_addr.s_addr = inet_addr(argv[1]);
	myNode.nodeInfo.addrInfo.sin_port = htons(atoi(argv[2]));

	//1. 해시함수로 ID할당
	myNode.nodeInfo.ID = str_hash(&argv[2]);

	showComand();


	while (1) {

		//사용자 입력
		printf(">>");
		scanf("%c", &strInput);

		switch (strInput) {

			// create
		case 'c':
			if (netCheck == 1)
				printf("<%d>[main]create or join이 이미 수행되었습니다.\n", myNode.nodeInfo.ID);

			else {

				netCheck = 1;

				// 현재 노드의 predecessor와 fingers 현재 노드로 초기화
				myNode.chordInfo.fingerInfo.Pre = myNode.nodeInfo;
				for (int i = 0; i < baseM; i++)
					myNode.chordInfo.fingerInfo.finger[i] = myNode.nodeInfo;


				//UDP 소켓 생성
				rqSock = socket(AF_INET, SOCK_DGRAM, 0); //request
				rpSock = socket(AF_INET, SOCK_DGRAM, 0); //response
				ffSock = socket(AF_INET, SOCK_DGRAM, 0); //fixfinger
				ppSock = socket(AF_INET, SOCK_DGRAM, 0); //PingPong

														 //UDP 소켓 옵션 설정 (타임아웃)
				retVal = setsockopt(rqSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&optVal, sizeof(optVal));
				retVal = setsockopt(ffSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&optVal, sizeof(optVal));
				retVal = setsockopt(ppSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&optVal, sizeof(optVal));

				//rpSock 바인딩
				if (bind(rpSock, (struct sockaddr *) &myNode.nodeInfo.addrInfo, sizeof(myNode.nodeInfo.addrInfo)) < 0) {
					perror("binding error!");
					exit(1);
				}

				//TCP 소켓 생성
				flSock = socket(AF_INET, SOCK_STREAM, 0); //파일 수신 연결 대기 소켓
				if (bind(flSock, (SOCKADDR*)&myNode.nodeInfo.addrInfo, sizeof(myNode.nodeInfo.addrInfo)) < 0) {
					printf("binding error!\n");
					exit(1);
				}

				//[1]. 입력한 인자 값 myNode에 제대로 저장+노드 ID
				printf("[main]IP Addr: %s, Port No: %d, ID: %d\n", inet_ntoa(myNode.nodeInfo.addrInfo.sin_addr), ntohs(myNode.nodeInfo.addrInfo.sin_port), myNode.nodeInfo.ID);

				//Thread 생성
				hThread[0] = (HANDLE)_beginthreadex(NULL, 0, (void *)procRecvMsg, (void *)&exitFlag, 0, NULL);
				hThread[1] = (HANDLE)_beginthreadex(NULL, 0, (void *)procFixFinger, (void *)&exitFlag, 0, NULL);
				hThread[2] = (HANDLE)_beginthreadex(NULL, 0, (void *)procPingPong, (void *)&exitFlag, 0, NULL);
				hThread[3] = (HANDLE)_beginthreadex(NULL, 0, (void *)fileReceiver, (void *)&exitFlag, 0, NULL);

			}
			break;

			//join
		case 'j':

			if (netCheck == 1)
				printf("<%d>[main]create or join이 이미 수행되었습니다.\n", myNode.nodeInfo.ID);

			else {
				netCheck = 1;

				// 현재 노드의 predecessor와 fingers -1(null)로 초기화
				myNode.chordInfo.fingerInfo.Pre.ID = -1;
				for (int i = 0; i < baseM; i++)
					myNode.chordInfo.fingerInfo.finger[i].ID = -1;


				//UDP 소켓 생성
				rqSock = socket(AF_INET, SOCK_DGRAM, 0); // for request
				rpSock = socket(AF_INET, SOCK_DGRAM, 0); // for response
				ffSock = socket(AF_INET, SOCK_DGRAM, 0); // fixfinger 
				ppSock = socket(AF_INET, SOCK_DGRAM, 0); // PingPong

														 //UDP 소켓 옵션 설정 (타임아웃)
				retVal = setsockopt(rqSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&optVal, sizeof(optVal));
				retVal = setsockopt(ffSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&optVal, sizeof(optVal));
				retVal = setsockopt(ppSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&optVal, sizeof(optVal));

				//rpSock 바인딩
				if (bind(rpSock, (struct sockaddr *) &myNode.nodeInfo.addrInfo, sizeof(myNode.nodeInfo.addrInfo)) < 0) {
					perror("[main]Error: bind failed!");
					exit(1);
				}

				//TCP 소켓 생성
				flSock = socket(AF_INET, SOCK_STREAM, 0); //파일 수신 연결 대기 소켓

														  //flSock 바인딩
				if (bind(flSock, (SOCKADDR*)&myNode.nodeInfo.addrInfo, sizeof(myNode.nodeInfo.addrInfo)) < 0) {
					printf("binding error!\n");
					exit(1);
				}



				//help 노드 주소 정보 입력
				printf(">>연결할 노드의 아이피주소 입력 : ");
				scanf("%s", &ip);
				printf(">>연결할 노드의 포트번호 입력 : ");
				scanf("%d", &port);

				myAddr.sin_addr.s_addr = inet_addr(&ip);
				myAddr.sin_port = htons(port);
				if (mute == 1)
					//help 노드 확인용
					printf("<%d>[main]<help>IP Addr: %s, Port No: %d\n", myNode.nodeInfo.ID, inet_ntoa(myAddr.sin_addr), ntohs(myAddr.sin_port));

				//1. join info

				//join_Info_request
				memset(&temp, 0, sizeof(temp));
				temp.msgID = 1;
				temp.msgType = 0;
				temp.nodeInfo = myNode.nodeInfo;
				temp.moreInfo = 0;
				temp.bodySize = 0;

				printf("<보냄>");
				msgNotice(temp.msgID, temp.msgType);

				//request msg 보내기
				sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &myAddr, sizeof(myAddr));

				//response msg 받기
				retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
				printf("<받음>");
				msgNotice(buffer.msgID, buffer.msgType);


				myNode.chordInfo.fingerInfo.finger[0] = buffer.nodeInfo;
				if (mute == 1)
					printf("<%d>[main]>> : successor 설정: %d \n\n", myNode.nodeInfo.ID, myNode.chordInfo.fingerInfo.finger[0].ID);


				//2.moveykey
				moveflag = 0;

				moveKeyAddr = myNode.chordInfo.fingerInfo.finger[0].addrInfo;

				memset(&temp, 0, sizeof(temp));
				temp.msgID = 2;
				temp.msgType = 0;
				temp.nodeInfo = myNode.nodeInfo;
				temp.moreInfo = 0;
				temp.bodySize = 0;

				printf("<보냄>");
				msgNotice(temp.msgID, temp.msgType);

				//request msg 보내기
				sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &moveKeyAddr, sizeof(moveKeyAddr));

				//response msg 받기
				retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);


				printf("<받음>");
				msgNotice(buffer.msgID, buffer.msgType);

				moveflag = buffer.moreInfo;


				keyNum = buffer.moreInfo;
				printf("받아온 keyNum 값 :%d \n", keyNum);

				if (keyNum == 0)
					printf("받을 키가 없습니다.\n");

				else {
					WaitForSingleObject(hMutex, INFINITE);
					myNode.chordInfo.FRefInfo.fileRef[myNode.chordInfo.FRefInfo.fileNum] = buffer.fileInfo;
					myNode.chordInfo.FRefInfo.fileRef[myNode.chordInfo.FRefInfo.fileNum].refOwner = myNode.nodeInfo;
					/////////////////////////////////
					ownerAddr = myNode.chordInfo.FRefInfo.fileRef[myNode.chordInfo.FRefInfo.fileNum].owner.addrInfo;
					memset(&temp, 0, sizeof(temp));
					temp.msgID = 13;
					temp.msgType = 0;
					temp.nodeInfo = myNode.nodeInfo;
					temp.moreInfo = myNode.chordInfo.FRefInfo.fileRef[myNode.chordInfo.FRefInfo.fileNum].Key;
					temp.bodySize = 0;
					printf("<보냄>");
					msgNotice(temp.msgID, temp.msgType);

					//request msg 보내기
					sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &ownerAddr, sizeof(ownerAddr));

					//response msg 받기
					retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
					printf("<받음>");
					msgNotice(buffer.msgID, buffer.msgType);


					//////////////////////

					myNode.chordInfo.FRefInfo.fileNum++;
					ReleaseMutex(hMutex);
					if (moveflag != 1) {
						for (int i = 1; i < keyNum; i++) {
							memset(&temp, 0, sizeof(temp));
							temp.msgID = 2;
							temp.msgType = 0;
							temp.nodeInfo = myNode.nodeInfo;
							temp.moreInfo = 0;
							temp.bodySize = 0;

							printf("<보냄>");
							msgNotice(temp.msgID, temp.msgType);
							//request msg 보내기
							sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &moveKeyAddr, sizeof(moveKeyAddr));

							retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);

							printf("<받음>");
							msgNotice(buffer.msgID, buffer.msgType);



							keyNum = buffer.moreInfo;
							if (keyNum == 0) {
								printf("받을 키가 없습니다.\n");
								break;
							}
							else {
								WaitForSingleObject(hMutex, INFINITE);
								myNode.chordInfo.FRefInfo.fileRef[myNode.chordInfo.FRefInfo.fileNum] = buffer.fileInfo;
								myNode.chordInfo.FRefInfo.fileRef[myNode.chordInfo.FRefInfo.fileNum].refOwner = myNode.nodeInfo;
								/////////////////////////////////
								ownerAddr = myNode.chordInfo.FRefInfo.fileRef[myNode.chordInfo.FRefInfo.fileNum].owner.addrInfo;
								memset(&temp, 0, sizeof(temp));
								temp.msgID = 13;
								temp.msgType = 0;
								temp.nodeInfo = myNode.nodeInfo;
								temp.moreInfo = myNode.chordInfo.FRefInfo.fileRef[myNode.chordInfo.FRefInfo.fileNum].Key;
								temp.bodySize = 0;
								printf("<보냄>");
								msgNotice(temp.msgID, temp.msgType);

								//request msg 보내기
								sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &ownerAddr, sizeof(ownerAddr));

								//response msg 받기
								retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
								printf("<받음>");
								msgNotice(buffer.msgID, buffer.msgType);


								//////////////////////

								myNode.chordInfo.FRefInfo.fileNum++;
								ReleaseMutex(hMutex);
							}



						}
					}
				}


				//3.join의 successor에게 predecessor 요청

				//predecessor_info_request
				memset(&temp, 0, sizeof(temp));
				temp.msgID = 3;
				temp.msgType = 0;
				temp.nodeInfo = myNode.chordInfo.fingerInfo.finger[0];
				temp.moreInfo = 0;
				temp.bodySize = 0;

				printf("<보냄>");
				msgNotice(temp.msgID, temp.msgType);

				//request msg 보내기
				sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &myNode.chordInfo.fingerInfo.finger[0].addrInfo, sizeof(struct sockaddr));

				//response msg 받기
				retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
				printf("<받음>");
				msgNotice(buffer.msgID, buffer.msgType);


				myNode.chordInfo.fingerInfo.Pre = buffer.nodeInfo;
				printf("<%d>[main]>> : predecessor 설정: %d \n", myNode.nodeInfo.ID, myNode.chordInfo.fingerInfo.Pre.ID);


				//4.successor의 predecessor에게 (위에서 알아옴)

				//successor_update_request
				memset(&temp, 0, sizeof(temp));
				temp.msgID = 6;
				temp.msgType = 0;
				temp.nodeInfo = myNode.nodeInfo;
				temp.moreInfo = 0;
				temp.bodySize = 0;

				printf("<보냄>");
				msgNotice(temp.msgID, temp.msgType);

				//request msg 보내기
				sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &buffer.nodeInfo.addrInfo, sizeof(struct sockaddr));

				//response msg 받기
				retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
				printf("<받음>");
				msgNotice(buffer.msgID, buffer.msgType);

				//5. join노드의 successor에게

				//predecessor_update_request
				memset(&temp, 0, sizeof(temp));
				temp.msgID = 5;
				temp.msgType = 0;
				temp.nodeInfo = myNode.nodeInfo;
				temp.moreInfo = 0;
				temp.bodySize = 0;

				printf("<보냄>");
				msgNotice(temp.msgID, temp.msgType);

				//request msg 보내기
				sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &myNode.chordInfo.fingerInfo.finger[0].addrInfo, sizeof(struct sockaddr));

				//response msg 받기
				retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
				printf("<받음>");
				msgNotice(buffer.msgID, buffer.msgType);


				//Thread 생성
				hThread[0] = (HANDLE)_beginthreadex(NULL, 0, (void *)procRecvMsg, (void *)&exitFlag, 0, NULL);
				hThread[1] = (HANDLE)_beginthreadex(NULL, 0, (void *)procFixFinger, (void *)&exitFlag, 0, NULL);
				hThread[2] = (HANDLE)_beginthreadex(NULL, 0, (void *)procPingPong, (void *)&exitFlag, 0, NULL);
				hThread[3] = (HANDLE)_beginthreadex(NULL, 0, (void *)fileReceiver, (void *)&exitFlag, 0, NULL);

			}
			break;

		case 'l':
			//leave에서는 이 과정만 이루어지고 사실 상 predecessor, successor의 설정은 pingpong에서 처리되어야함
			//-> leave된 노드의 predecessor에서 stabilize 이루어짐

			//1. leave_keys
			keyNum = myNode.chordInfo.FRefInfo.fileNum;
			for (int i = 0; i < myNode.chordInfo.FRefInfo.fileNum; i++) {

				memset(&temp, 0, sizeof(temp)); //temp 초기화
				temp.msgID = 11;
				temp.msgType = 0;
				temp.fileInfo = myNode.chordInfo.FRefInfo.fileRef[i]; //소유하고 있는 파일 ref 정보를 준다
				temp.moreInfo = keyNum;
				temp.bodySize = sizeof(myNode.chordInfo.FRefInfo.fileRef[i]);


				printf("<보냄>");
				msgNotice(temp.msgID, temp.msgType);

				//request msg 보내기
				sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &myNode.chordInfo.fingerInfo.finger[0].addrInfo, sizeof(struct sockaddr));


				//response msg 받기
				retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
				printf("<받음>");
				msgNotice(buffer.msgID, buffer.msgType);


				/////////////////////////////////
				ownerAddr = myNode.chordInfo.FRefInfo.fileRef[i].owner.addrInfo;
				memset(&temp, 0, sizeof(temp));
				temp.msgID = 13;
				temp.msgType = 0;
				temp.nodeInfo = myNode.chordInfo.fingerInfo.finger[0];
				temp.moreInfo = myNode.chordInfo.FRefInfo.fileRef[i].Key;
				temp.bodySize = 0;
				printf("<보냄>");
				msgNotice(temp.msgID, temp.msgType);

				//request msg 보내기
				sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &ownerAddr, sizeof(ownerAddr));

				//response msg 받기
				retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
				printf("<받음>");
				msgNotice(buffer.msgID, buffer.msgType);


				//////////////////////

				keyNum--;
			}


			//2. file_reference_delete
			for (int i = 0; i < myNode.fileInfo.fileNum; i++) {
				memset(&temp, 0, sizeof(temp));
				temp.msgID = 9;
				temp.msgType = 0;
				temp.nodeInfo = myNode.nodeInfo;
				temp.moreInfo = myNode.fileInfo.fileRef[i].Key;
				temp.bodySize = 0;

				printf("<보냄>");
				msgNotice(temp.msgID, temp.msgType);

				//request msg 보내기
				sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &myNode.fileInfo.fileRef[i].refOwner.addrInfo, sizeof(struct sockaddr));

				//response msg 받기
				retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
				printf("<받음>");
				msgNotice(buffer.msgID, buffer.msgType);


			}


			printf("<%d>노드 leave \n", myNode.nodeInfo.ID);

			exitFlag = 1;
			WaitForMultipleObjects(4, hThread, TRUE, INFINITE);
			WaitForMultipleObjects(sendCount, fileSender, TRUE, INFINITE);
			printf("프로그램 종료\n");
			exit(1);
			break;


		case 'a':

			if (netCheck == 0) {
				printf("<%d>[add]create or join이 아직 수행되지 않았습니다.\n", myNode.nodeInfo.ID);
				continue;
			}

			printf(">>추가할 파일 이름 : ");
			scanf("%s", fileName);
			printf("<%d>[add]fileName : %s\n", myNode.nodeInfo.ID, fileName);

			if ((fopen(fileName, "rb")) == NULL)
			{
				printf(">>파일이 존재하지 않습니다.  \n");
				continue;
			}

			fileID = str_hash(fileName);
			printf("<%d>[add]fileID : %d\n", myNode.nodeInfo.ID, fileID);


			//fileID에 해당하는 find_successor 과정 필요
			if (myNode.nodeInfo.ID == fileID)
				fileSuccNode = myNode.nodeInfo;

			else if (myNode.nodeInfo.ID == myNode.chordInfo.fingerInfo.finger[0].ID)
				fileSuccNode = myNode.nodeInfo;

			//find_predecessor 과정
			else {

				closetCheck = 0;

				for (int i = baseM - 1; i >= 0; i--) {
					if (myNode.chordInfo.fingerInfo.finger[i].ID == -1)
						continue;

					if (modIn(ringSize, myNode.chordInfo.fingerInfo.finger[i].ID, myNode.nodeInfo.ID, fileID, 0, 0)) {
						closetpredAddr = myNode.chordInfo.fingerInfo.finger[i].addrInfo;
						printf("<%d>[main] closetpredAddr에 저장된 노드의 번호 : %d\n", myNode.nodeInfo.ID, myNode.chordInfo.fingerInfo.finger[i].ID);
						closetCheck = 1;
						break;
					}
				}

				//find_closet_predecessor가 필요한 경우
				if (closetCheck == 1) {

					memset(&temp, 0, sizeof(temp)); //temp 초기화
					temp.msgID = 7;
					temp.msgType = 0;
					temp.nodeInfo = myNode.nodeInfo; //&
					temp.moreInfo = fileID;
					temp.bodySize = 0;

					printf("<보냄>");
					msgNotice(temp.msgID, temp.msgType);

					//request msg 보내기
					sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &closetpredAddr, sizeof(closetpredAddr));

					//response msg 받기
					retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
					printf("<받음>");
					msgNotice(buffer.msgID, buffer.msgType);

					predNode = buffer.nodeInfo;
					predAddr = buffer.nodeInfo.addrInfo;

				}

				//슈도코드의 find_closet_predecessor의 마지막라인 : 위의 반복문 if에걸리지 않으면 자신의 노드 반환
				else if (closetCheck == 0) {
					predNode = myNode.nodeInfo;
					predAddr = myNode.nodeInfo.addrInfo;
				}

				printf("<%d>[main] predNode : %d\n", myNode.nodeInfo.ID, predNode.ID);

				//successor_info 필요

				memset(&temp, 0, sizeof(temp)); //temp 초기화
				temp.msgID = 4;
				temp.msgType = 0;
				temp.nodeInfo = predNode;
				temp.bodySize = 0;

				printf("<보냄>");
				msgNotice(temp.msgID, temp.msgType);
				sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &predAddr, sizeof(struct sockaddr));

				retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
				printf("<받음>");
				msgNotice(buffer.msgID, buffer.msgType);

				fileSuccNode = buffer.nodeInfo;

			}


			printf("<%d>[main] 입력된 파일[ %s - %d]의 successor : %d .\n", myNode.nodeInfo.ID, fileName, fileID, fileSuccNode.ID);

			//fileInfoType에 저장///////////////////////////////
			WaitForSingleObject(hMutex, INFINITE);
			strcpy(myNode.fileInfo.fileRef[myNode.fileInfo.fileNum].Name, fileName);
			myNode.fileInfo.fileRef[myNode.fileInfo.fileNum].Key = fileID;
			myNode.fileInfo.fileRef[myNode.fileInfo.fileNum].owner = myNode.nodeInfo;
			myNode.fileInfo.fileRef[myNode.fileInfo.fileNum].refOwner = fileSuccNode;
			ReleaseMutex(hMutex);

			filePrint(myNode);

			myNode.fileInfo.fileNum++;
			printf("<%d>[add] 현재 노드의 파일 소유 수 : %d\n", myNode.nodeInfo.ID, myNode.fileInfo.fileNum);


			//file에 대한 reference 정보 

			//file_Reference_add
			memset(&temp, 0, sizeof(temp)); //temp 초기화
			temp.msgID = 8;
			temp.msgType = 0;
			temp.nodeInfo = myNode.nodeInfo; //&
			temp.fileInfo.Key = fileID;
			temp.fileInfo.owner = myNode.nodeInfo;
			temp.fileInfo.refOwner = fileSuccNode;
			strcpy(temp.fileInfo.Name, fileName);

			printf("<보냄>");

			sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &fileSuccNode.addrInfo, sizeof(struct sockaddr));

			retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
			printf("<받음>");
			msgNotice(buffer.msgID, buffer.msgType);


			break;



		case 'd':
			deleteCheck = 0;
			if (netCheck == 0) {
				printf("<%d>[delete]create or join이 아직 수행되지 않았습니다.\n", myNode.nodeInfo.ID);
				continue;
			}

			printf(">>삭제할 파일 이름 : ");
			scanf("%s", fileName);
			printf("<%d>[delete]fileName : %s\n", myNode.nodeInfo.ID, fileName);

			for (int i = 0; i < myNode.fileInfo.fileNum; i++) {
				if (strcmp(myNode.fileInfo.fileRef[i].Name, fileName) == 0)
				{
					fileID = str_hash(fileName);
					printf("fileName : %s , fileID :%d\n", fileName, fileID);
					deleteCheck = 1;
					fileRefNode = myNode.fileInfo.fileRef[i].refOwner;
					printf("삭제할 노드의 refowner : %d\n", fileRefNode.ID);

					//소유 정보 먼저 삭제
					WaitForSingleObject(hMutex, INFINITE);
					memset(&myNode.fileInfo.fileRef[i], 0, sizeof(myNode.fileInfo.fileRef[i]));
					myNode.fileInfo.fileNum--;
					for (int j = 0; j < myNode.fileInfo.fileNum; j++)
						myNode.fileInfo.fileRef[i] = myNode.fileInfo.fileRef[i + 1];
					ReleaseMutex(hMutex);

					break;
				}
			}

			//refowner

			//현재 노드가 소유하고 있는 파일이라면 삭제 진행
			if (deleteCheck == 1) {
				memset(&temp, 0, sizeof(temp));
				temp.msgID = 9;
				temp.msgType = 0;
				temp.nodeInfo = myNode.nodeInfo;
				temp.moreInfo = fileID;
				temp.bodySize = 0;

				printf("<보냄>");
				msgNotice(temp.msgID, temp.msgType);

				//request msg 보내기
				sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &fileRefNode.addrInfo, sizeof(struct sockaddr));

				//response msg 받기
				retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
				printf("<받음>");
				msgNotice(buffer.msgID, buffer.msgType);


				printf("reference 정보 삭제 완료\n");


			}

			else
				printf("현재 노드가 소유하고 있는 파일이 아닙니다.\n");

			break;

		case 's':
			printf("\n------------fileSearch----------- \n");
			printf(">> 검색할 파일 이름 : ");
			scanf("%s", fileName);

			strcpy(File.fileInfo.Name, fileName);

			fileID = str_hash(fileName);
			printf("파일 이름 : %s _파일 아이디 : %d \n", File.fileInfo.Name, fileID);

			closetCheck = 0;

			//file add 부분과 동일함
			//fileID에 해당하는 find_successor 과정 필요
			if (myNode.nodeInfo.ID == fileID)
				fileSuccNode = myNode.nodeInfo;

			else if (myNode.nodeInfo.ID == myNode.chordInfo.fingerInfo.finger[0].ID)
				fileSuccNode = myNode.nodeInfo;

			//find_predecessor 과정
			else {

				closetCheck = 0;

				for (int i = baseM - 1; i >= 0; i--) {
					if (myNode.chordInfo.fingerInfo.finger[i].ID == -1)
						continue;

					if (modIn(ringSize, myNode.chordInfo.fingerInfo.finger[i].ID, myNode.nodeInfo.ID, fileID, 0, 0)) {
						closetpredAddr = myNode.chordInfo.fingerInfo.finger[i].addrInfo;
						printf("<%d>[main] closetpredAddr에 저장된 노드의 번호 : %d\n", myNode.nodeInfo.ID, myNode.chordInfo.fingerInfo.finger[i].ID);
						closetCheck = 1;
						break;
					}
				}

				//find_closet_predecessor가 필요한 경우
				if (closetCheck == 1) {

					memset(&temp, 0, sizeof(temp)); //temp 초기화
					temp.msgID = 7;
					temp.msgType = 0;
					temp.nodeInfo = myNode.nodeInfo; //&
					temp.moreInfo = fileID;
					temp.bodySize = 0;

					printf("<보냄>");
					msgNotice(temp.msgID, temp.msgType);

					//request msg 보내기
					sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &closetpredAddr, sizeof(closetpredAddr));

					//response msg 받기
					retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
					printf("<받음>");
					msgNotice(buffer.msgID, buffer.msgType);


					predNode = buffer.nodeInfo;
					predAddr = buffer.nodeInfo.addrInfo;

				}

				//슈도코드의 find_closet_predecessor의 마지막라인 : 위의 반복문 if에걸리지 않으면 자신의 노드 반환
				else if (closetCheck == 0) {
					predNode = myNode.nodeInfo;
					predAddr = myNode.nodeInfo.addrInfo;
				}

				printf("<%d>[main] predNode : %d\n", myNode.nodeInfo.ID, predNode.ID);

				//successor_info 필요

				memset(&temp, 0, sizeof(temp)); //temp 초기화
				temp.msgID = 4;
				temp.msgType = 0;
				temp.nodeInfo = predNode;
				temp.bodySize = 0;

				printf("<보냄>");
				msgNotice(temp.msgID, temp.msgType);

				sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &predAddr, sizeof(struct sockaddr));

				retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
				printf("<받음>");
				msgNotice(buffer.msgID, buffer.msgType);


				fileSuccNode = buffer.nodeInfo;
			}

			printf("<%d>[main] 검색한 파일[ %s - %d]의 successor : %d .\n", myNode.nodeInfo.ID, fileName, fileID, fileSuccNode.ID);

			//fileSuccNode에게
			//file_reference_info 
			memset(&temp, 0, sizeof(temp)); //temp 초기화
			temp.msgID = 10;
			temp.msgType = 0;
			temp.nodeInfo = myNode.nodeInfo;
			temp.moreInfo = fileID;
			strcpy(temp.fileInfo.Name, fileName); //굳이 넣어주어야하는 정보인가?일단 넣어본다
			temp.bodySize = 0;

			printf("<보냄>");
			msgNotice(temp.msgID, temp.msgType);
			//////msgPrint(myNode.nodeInfo.ID, mainThread, temp, typetemp);
			sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &fileSuccNode.addrInfo, sizeof(struct sockaddr));

			//response msg 받기
			retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
			printf("<받음>");
			msgNotice(buffer.msgID, buffer.msgType);
			////////msgPrint(myNode.nodeInfo.ID, mainThread, buffer, typebuffer);

			printf("\n검색한 파일 명 : %s\n", fileName);
			printf("buffer.fileInfo.Name : %s\n", buffer.fileInfo.Name);

			if (strcmp(buffer.fileInfo.Name, fileName) != 0)
			{
				printf(" < %s > 이름의 파일이 존재하지 않습니다\n", fileName);
				break;
			}

			////////////////////////////////////////////////////////////////////////////////
			//file download

			printf("==================================================\n");
			printf("\n      %s 파일을 다운 받으시겠습니까?\n", fileName);
			printf("        1: 예    2 : 아니오 \n");
			printf("\n================================================");

			scanf("%d", &down);

			printf(" %d \n", down);

			if (down == 1) {

				memset(&temp, 0, sizeof(temp));
				temp.msgID = 12;
				temp.msgType = 0;
				temp.nodeInfo = myNode.nodeInfo;
				temp.fileInfo = buffer.fileInfo;

				printf("<보냄>");
				msgNotice(temp.msgID, temp.msgType);
				////////msgPrint(myNode.nodeInfo.ID, mainThread, temp, typetemp);
				sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &buffer.fileInfo.owner.addrInfo, sizeof(struct sockaddr));

				retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
				printf("<받음>");
				msgNotice(buffer.msgID, buffer.msgType);
				////////msgPrint(myNode.nodeInfo.ID, mainThread, buffer, typebuffer);


				break;
			}
			else
				printf(">>");

			break;

		case 'i':
			printf("=====================현재 노드 정보=====================\n");
			printf("<Info> IP Addr: %s, Port No: %d ID: %d\n", inet_ntoa(myNode.nodeInfo.addrInfo.sin_addr), ntohs(myNode.nodeInfo.addrInfo.sin_port), myNode.nodeInfo.ID);
			printf("<Info> Reference 수 : %d 개\n", myNode.chordInfo.FRefInfo.fileNum);
			printf("<Info> File 수 : %d 개\n", myNode.fileInfo.fileNum);

			for (int i = 0; i < myNode.fileInfo.fileNum; i++) {
				printf("\n========== File Info ==============\n");
				printf("<Info>File[%d] key: %d \n", i, myNode.fileInfo.fileRef[i].Key);
				printf("<Info>File[%d] RefOwner: %d \n", i, myNode.fileInfo.fileRef[i].refOwner.ID);
				printf("<Info>File name: %s \n", myNode.fileInfo.fileRef[i].Name);
				printf("============================================\n");
			}
			for (int i = 0; i < myNode.chordInfo.FRefInfo.fileNum; i++) {
				printf("\n========== Reference Info ==============\n");
				printf("<Info> File[%d] key: %d \n", i, myNode.chordInfo.FRefInfo.fileRef[i].Key);
				printf("<Info> File[%d] Owner: %d \n", i, myNode.chordInfo.FRefInfo.fileRef[i].owner.ID);
				printf("<Info> File name: %s \n", myNode.chordInfo.FRefInfo.fileRef[i].Name);
				printf("========================================================\n");
			}

			printf("========================================================\n\n");
			break;


		case 'f':
			printf("Predecessor IP Addr: (%s) PortNum: (%d) ID: (%d)\n", inet_ntoa(myNode.chordInfo.fingerInfo.Pre.addrInfo.sin_addr), ntohs(myNode.chordInfo.fingerInfo.Pre.addrInfo.sin_port), myNode.chordInfo.fingerInfo.Pre.ID);
			for (int i = 0; i < baseM; i++)
				printf(" [%d번째] Finger  IP Addr:(%s) PortNum:(%d)  ID:(%d)  \n", i, inet_ntoa(myNode.chordInfo.fingerInfo.finger[i].addrInfo.sin_addr), ntohs(myNode.chordInfo.fingerInfo.finger[i].addrInfo.sin_port), myNode.chordInfo.fingerInfo.finger[i].ID);

			break;


		case 'm':
			//mute 0 == 사일런트 모드
			//mute 1 == 사일런트 모드X
			if (mute == 0) {
				mute = 1;
				printf(">>Slient 모드 OFF\n\n");
			}
			else {
				mute = 0;
				printf(">>Slient 모드 ON\n\n");
			}

			break;


		case 'q':
			exitFlag = 1;
			WaitForMultipleObjects(4, hThread, TRUE, INFINITE);
			WaitForMultipleObjects(sendCount, fileSender, TRUE, INFINITE);
			printf("\n프로그램을 종료합니다\n");
			exit(1);
			break;
		}

	}


}


unsigned WINAPI procRecvMsg(void *arg) {


	HANDLE sendThread[10];
	nodeInfoType succNode;
	nodeInfoType predNode;

	chordHeaderType buffer; //받는 메세지 저장
	chordHeaderType temp; // 보낼 메세지

	int retVal;
	int closetCheck = 0;
	int joinkey = 0; //join요청한 노드의 id값
	int tempkey = 0;
	int keyNum = 0; //movekey 수행 시 movekey개수
	int index = 0;
	int filesize;

	struct sockaddr_in sendAddr; //클라이언트 주소
	int sendAddrLen = sizeof(struct sockaddr); //recv하기위한 
	struct sockaddr_in closetpredAddr;
	struct sockaddr_in predAddr;
	struct sockaddr_in ownerAddr;

	closetpredAddr.sin_family = AF_INET;
	predAddr.sin_family = AF_INET;
	ownerAddr.sin_family = AF_INET;

	while (!(*exitFlag)) {
		keyNum = 0;

		retVal = recvfrom(rpSock, (char *)&buffer, sizeof(buffer), 0, (struct sockaddr*)&sendAddr, &sendAddrLen);
		printf("<받음>");
		msgNotice(buffer.msgID, buffer.msgType);
		////////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, buffer, typebuffer);

		switch (buffer.msgID) {

			//0.ping_pong
		case 0:

			memset(&temp, 0, sizeof(temp)); //temp 초기화
			temp.msgID = 0;
			temp.msgType = 1;
			temp.nodeInfo = myNode.nodeInfo;
			temp.moreInfo = 0;
			temp.bodySize = 0;

			//printf("<보냄>");
			//msgNotice(temp.msgID, temp.msgType);
			//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
			sendto(rpSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &sendAddr, sizeof(struct sockaddr));

			break;

			//1.join_Info
		case 1:

			printf("<%d>[procRecvMsg] temp.nodeInfo <- buffer.nodeInfo의 successor \n", myNode.nodeInfo.ID);
			printf("=>find_successor과정 필요\n");

			//find_successor
			//join 노드의 successor 찾아야함
			//joinkey = join노드의 id값
			joinkey = buffer.nodeInfo.ID;

			if (myNode.nodeInfo.ID == buffer.nodeInfo.ID)
				succNode = myNode.nodeInfo;

			else if (myNode.nodeInfo.ID == myNode.chordInfo.fingerInfo.finger[0].ID)
				succNode = myNode.nodeInfo;

			//find_predecessor 과정
			else {

				closetCheck = 0;
				if (mute == 1)
					printf("<%d>[procRecvMsg]>> : find_predecessor과정 필요 \n", myNode.nodeInfo.ID);

				for (int i = baseM - 1; i >= 0; i--) {
					if (myNode.chordInfo.fingerInfo.finger[i].ID == -1)
						continue;

					if (modIn(ringSize, myNode.chordInfo.fingerInfo.finger[i].ID, myNode.nodeInfo.ID, buffer.nodeInfo.ID, 0, 0)) {
						closetpredAddr = myNode.chordInfo.fingerInfo.finger[i].addrInfo;
						if (mute == 1)
							printf("<%d>[procRecvMsg] closetpredAddr에 저장된 노드의 아이디 : %d\n", myNode.nodeInfo.ID, myNode.chordInfo.fingerInfo.finger[i].ID);
						closetCheck = 1;
						break;
					}
				}

				//////////////////////////////////////////////////////////////////////////////////////
				//find_closet_predecessor가 필요한 경우
				if (closetCheck == 1) {

					memset(&temp, 0, sizeof(temp)); //temp 초기화
					temp.msgID = 7;
					temp.msgType = 0;
					temp.nodeInfo = myNode.nodeInfo; //&
					temp.moreInfo = joinkey;
					temp.bodySize = 0;


					printf("<보냄>");
					msgNotice(temp.msgID, temp.msgType);
					//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
					//request msg 보내기
					sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &closetpredAddr, sizeof(closetpredAddr));

					//response msg 받기
					retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
					printf("<받음>");
					msgNotice(buffer.msgID, buffer.msgType);
					//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, buffer, typebuffer);

					predNode = buffer.nodeInfo;
					predAddr = predNode.addrInfo;
					if (mute == 1)
						printf("<%d>[procRecvMsg]>> : predNode : %d (closet케이스)\n", myNode.nodeInfo.ID, predNode.ID);
				}


				//슈도코드의 find_closet_predecessor의 마지막라인 : 위의 반복문 if에걸리지 않으면 자신의 노드 반환
				else if (closetCheck == 0) {
					predNode = myNode.nodeInfo;
					if (mute == 1)
						printf("<%d>[procRecvMsg]>> : predNode : %d (자신노드반환케이스)\n", myNode.nodeInfo.ID, predNode.ID);

				}
				////////////////////////////////////////////////////////////////////////////////////////


				//predNode로 succNode알아내야함 :successor_info 필요
				if (predNode.ID != myNode.nodeInfo.ID) {

					//successor_info_request
					memset(&temp, 0, sizeof(temp)); //temp 초기화
					temp.msgID = 4;
					temp.msgType = 0;
					temp.nodeInfo = predNode;
					temp.bodySize = 0;


					printf("<보냄>");
					msgNotice(temp.msgID, temp.msgType);
					//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
					sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &predAddr, sizeof(struct sockaddr));

					retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
					printf("<받음>");
					msgNotice(buffer.msgID, buffer.msgType);
					//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, buffer, typebuffer);

					succNode = buffer.nodeInfo;
					if (mute == 1)
						printf("<%d>[procRecvMsg]>> : succNode : %d (통신o)\n", myNode.nodeInfo.ID, succNode.ID);
				}

				else {
					succNode = myNode.chordInfo.fingerInfo.finger[0];
					if (mute == 1)
						printf("<%d>[procRecvMsg]>> : succNode : %d (통신x)\n", myNode.nodeInfo.ID, succNode.ID);
				}


			}


			//response msg 생성	
			memset(&temp, 0, sizeof(temp)); //temp 초기화
			temp.msgID = 1;
			temp.msgType = 1;
			temp.nodeInfo = succNode;
			temp.bodySize = 0;

			printf("<보냄>");
			msgNotice(temp.msgID, temp.msgType);
			//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
			sendto(rpSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &sendAddr, sizeof(struct sockaddr));

			break;

			//2. Move_keys
		case 2:
			if (mute == 1)
				printf("\n\n[Move_Keys] myNode.chordInfo.FRefInfo.fileNum = %d \n\n", myNode.chordInfo.FRefInfo.fileNum);

			//file reference 정보 없는 경우 (movekey할 필요가 없음)
			if (myNode.chordInfo.FRefInfo.fileNum == 0) {
				break;
			}

			//Move_keys할 개수 keyNum 구하기
			for (int i = 0; i < myNode.chordInfo.FRefInfo.fileNum; i++) {
				if (modIn(ringSize, buffer.nodeInfo.ID, myNode.chordInfo.FRefInfo.fileRef[i].Key, myNode.nodeInfo.ID, 1, 0)) {
					keyNum++;
					if (mute == 1)
						if (mute == 1)
							printf("case2에서 옮겨야하는 keyNum 계산 : %d\n", keyNum);
				}
			}

			if (keyNum != 0) {

				printf("\n<%d>[procRecvMsg_MoveKeys] keyNum : %d \n\n", myNode.nodeInfo.ID, keyNum);

				WaitForSingleObject(hMutex, INFINITE);
				for (int i = 0; i < myNode.chordInfo.FRefInfo.fileNum; i++) {

					if (modIn(ringSize, buffer.nodeInfo.ID, myNode.chordInfo.FRefInfo.fileRef[i].Key, myNode.nodeInfo.ID, 1, 0)) {
						if (mute == 1)
							printf("**i값 : %d **\n", i);
						memset(&temp, 0, sizeof(temp)); //temp 초기화
						temp.msgID = 2;
						temp.msgType = 1;
						temp.fileInfo = myNode.chordInfo.FRefInfo.fileRef[i]; //소유하고 있는 파일 ref 정보를 준다
						temp.moreInfo = keyNum;
						temp.bodySize = sizeof(myNode.chordInfo.FRefInfo.fileRef[i]);



						printf("<보냄>");
						msgNotice(temp.msgID, temp.msgType);
						//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
						sendto(rpSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &sendAddr, sizeof(struct sockaddr));



						//보내준 finger는 초기화
						memset(&myNode.chordInfo.FRefInfo.fileRef[i], 0, sizeof(myNode.chordInfo.FRefInfo.fileRef[i]));
						break;
					}

				}
				printf("fileNum : %d\n\n", myNode.chordInfo.FRefInfo.fileNum);

				for (int i = 0; i < myNode.chordInfo.FRefInfo.fileNum; i++) {
					myNode.chordInfo.FRefInfo.fileRef[i] = myNode.chordInfo.FRefInfo.fileRef[i + 1];
					if (mute == 1)
						printf("밀어주기\n");
				}

				myNode.chordInfo.FRefInfo.fileNum--;
				if (mute == 1)
					printf("밀어주고 난다음의?fileNum : %d\n\n", myNode.chordInfo.FRefInfo.fileNum);

				ReleaseMutex(hMutex);
			}

			else {
				//response msg 생성	
				memset(&temp, 0, sizeof(temp)); //temp 초기화
				temp.msgID = 2;
				temp.msgType = 1;
				temp.moreInfo = keyNum;//

				printf("movekey 완료 or 할 거 없음\n");
				printf("<보냄>");
				msgNotice(temp.msgID, temp.msgType);
				//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
				sendto(rpSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &sendAddr, sizeof(struct sockaddr));
			}


			break;

			//3. predecessor_info
		case 3:

			memset(&temp, 0, sizeof(temp)); //temp 초기화
			temp.msgID = 3;
			temp.msgType = 1;
			temp.nodeInfo = myNode.chordInfo.fingerInfo.Pre;
			temp.moreInfo = 1;
			temp.bodySize = 0;

			printf("<보냄>");
			msgNotice(temp.msgID, temp.msgType);
			//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
			sendto(rpSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &sendAddr, sizeof(struct sockaddr));

			break;


			//4. successor_info
		case 4:

			memset(&temp, 0, sizeof(temp)); //temp 초기화
			temp.msgID = 4;
			temp.msgType = 1;
			temp.nodeInfo = myNode.chordInfo.fingerInfo.finger[0];
			temp.moreInfo = 1;
			temp.bodySize = 0;

			printf("<보냄>");
			msgNotice(temp.msgID, temp.msgType);
			//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
			sendto(rpSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &sendAddr, sizeof(struct sockaddr));

			break;

			//5. predecessor_update
		case 5:

			myNode.chordInfo.fingerInfo.Pre = buffer.nodeInfo;

			printf("<%d>[procRecvMsg]>> : predecessor_update됨 : pre : %d \n", myNode.nodeInfo.ID, myNode.chordInfo.fingerInfo.Pre.ID);

			memset(&temp, 0, sizeof(temp)); //temp 초기화
			temp.msgID = 5;
			temp.msgType = 1;
			temp.moreInfo = 0;
			temp.bodySize = 0;

			printf("<보냄>");
			msgNotice(temp.msgID, temp.msgType);
			//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
			sendto(rpSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &sendAddr, sizeof(struct sockaddr));

			break;

			//6. successor_update
		case 6:

			myNode.chordInfo.fingerInfo.finger[0] = buffer.nodeInfo;
			printf("<%d>[procRecvMsg]>> : successor_update됨 : finger[0] : %d \n", myNode.nodeInfo.ID, myNode.chordInfo.fingerInfo.finger[0].ID);

			memset(&temp, 0, sizeof(temp)); //temp 초기화
			temp.msgID = 6;
			temp.msgType = 1;
			temp.moreInfo = 0;
			temp.bodySize = 0;

			printf("<보냄>");
			msgNotice(temp.msgID, temp.msgType);
			//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
			sendto(rpSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &sendAddr, sizeof(struct sockaddr));

			break;

			//7.find_predecessor
		case 7:

			closetCheck = 0;
			tempkey = buffer.moreInfo; // tempkey값의 find_predecessor 과정 

			if (myNode.nodeInfo.ID == myNode.chordInfo.fingerInfo.finger[0].ID) {
				predNode = myNode.nodeInfo;
				if (mute == 1)
					printf("<%d>[procRecvMsg] (초기 노드) predNode : %d\n", myNode.nodeInfo.ID, predNode.ID);
			}


			//find_closet_predecessor가 필요한 경우
			else {

				for (int i = baseM - 1; i >= 0; i--) {
					if (myNode.chordInfo.fingerInfo.finger[i].ID == -1)
						continue;

					if (modIn(ringSize, myNode.chordInfo.fingerInfo.finger[i].ID, myNode.nodeInfo.ID, buffer.moreInfo, 0, 0)) {
						closetpredAddr = myNode.chordInfo.fingerInfo.finger[i].addrInfo;
						if (mute == 1)
							printf("<%d>[procRecvMsg] closetpredAddr에 저장된 노드의 아이디 : %d\n", myNode.nodeInfo.ID, myNode.chordInfo.fingerInfo.finger[i].ID);
						closetCheck = 1; //find_closet_predecessor 필요
						break;
					}
				}

				//find_closet_predecessor가 필요한 경우
				if (closetCheck == 1) {

					memset(&temp, 0, sizeof(temp)); //temp 초기화
					temp.msgID = 7;
					temp.msgType = 0;
					temp.nodeInfo = myNode.nodeInfo;
					temp.moreInfo = buffer.moreInfo;
					temp.bodySize = 0;


					printf("<보냄>");
					msgNotice(temp.msgID, temp.msgType);
					//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
					sendto(rqSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &closetpredAddr, sizeof(closetpredAddr));

					//response msg 받기
					retVal = recvfrom(rqSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
					printf("<받음>");
					msgNotice(buffer.msgID, buffer.msgType);
					//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, buffer, typebuffer);

					predNode = buffer.nodeInfo;
					if (mute == 1)
						printf("<%d>[procRecvMsg]>> : predNode : %d (closet케이스)\n", myNode.nodeInfo.ID, predNode.ID);

				}

				//슈도코드의 find_closet_predecessor의 마지막라인 : 위의 반복문 if에걸리지 않으면 자신의 노드 반환
				//if : if (modIn(ringSize, myNode.chordInfo.fingerInfo.finger[i].ID, myNode.nodeInfo.ID, buffer.moreInfo, 0, 0)) 
				else {
					predNode = myNode.nodeInfo;
					if (mute == 1)
						printf("<%d>[procRecvMsg]>> : predNode : %d (자신노드반환케이스)\n", myNode.nodeInfo.ID, predNode.ID);
				}

			}

			memset(&temp, 0, sizeof(temp)); //temp 초기화
			temp.msgID = 7;
			temp.msgType = 1;
			temp.nodeInfo = predNode; //&
			temp.moreInfo = 1;
			temp.bodySize = 0;

			printf("<보냄>");
			msgNotice(temp.msgID, temp.msgType);
			//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
			sendto(rpSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &sendAddr, sizeof(struct sockaddr));

			break;

			//8. file_reference_add
		case 8:
			if (myNode.chordInfo.FRefInfo.fileNum + 1 > FileMax) {
				printf("[Add] : 최대 파일 개수 초과\n");
				break;
			}

			WaitForSingleObject(hMutex, INFINITE);
			myNode.chordInfo.FRefInfo.fileRef[myNode.chordInfo.FRefInfo.fileNum] = buffer.fileInfo;
			myNode.chordInfo.FRefInfo.fileNum++;
			ReleaseMutex(hMutex);

			memset(&temp, 0, sizeof(temp));
			temp.msgID = 8;
			temp.msgType = 1;
			temp.moreInfo = 1; // succese
			temp.bodySize = 0;
			msgNotice(temp.msgID, temp.msgType);
			//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);

			sendto(rpSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &sendAddr, sizeof(struct sockaddr));


			break;

			//9. file_reference_delete
		case 9:

			for (int i = 0; i < myNode.chordInfo.FRefInfo.fileNum; i++) {
				if (myNode.chordInfo.FRefInfo.fileRef[i].Key == buffer.moreInfo) {
					WaitForSingleObject(hMutex, INFINITE);
					myNode.chordInfo.FRefInfo.fileNum--;
					for (int j = 0; j < myNode.chordInfo.FRefInfo.fileNum; j++)
						myNode.chordInfo.FRefInfo.fileRef[i] = myNode.chordInfo.FRefInfo.fileRef[i + 1];
					ReleaseMutex(hMutex);
					break;
				}

			}


			memset(&temp, 0, sizeof(temp));
			temp.msgID = 9;
			temp.msgType = 1;
			temp.moreInfo = 1; // succese
			temp.bodySize = 0;

			printf("<보냄>");
			msgNotice(temp.msgID, temp.msgType);
			//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
			sendto(rpSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &sendAddr, sizeof(struct sockaddr));


			break;

			//10. file_reference_info
		case 10:
			for (int i = 0; i < myNode.chordInfo.FRefInfo.fileNum; i++) {
				if (myNode.chordInfo.FRefInfo.fileRef[i].Key == buffer.moreInfo) {

					memset(&temp, 0, sizeof(temp));
					temp.msgID = 10;
					temp.msgType = 1;
					temp.fileInfo = myNode.chordInfo.FRefInfo.fileRef[i];
					temp.moreInfo = 1;
					temp.bodySize = 0;
				}
				printf("<보냄>");
				msgNotice(temp.msgID, temp.msgType);
				//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
				sendto(rpSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &sendAddr, sizeof(struct sockaddr));

				break;
			}

			break;

			//11. Leave_Keys
		case 11:
			//이 메시지를 보낸 노드는 leave는 노드
			//이 메시지를 받은 노드는 leave노드의 successor
			//leave노드에서 보내준 키 값을 successor에 더해주어야함 

			keyNum = buffer.moreInfo;
			printf("받아온 keyNum 값 :%d \n", keyNum);

			if (keyNum == 0)
				printf("받을 키가 없습니다.\n");

			else {
				WaitForSingleObject(hMutex, INFINITE);

				myNode.chordInfo.FRefInfo.fileRef[myNode.chordInfo.FRefInfo.fileNum] = buffer.fileInfo;
				myNode.chordInfo.FRefInfo.fileRef[myNode.chordInfo.FRefInfo.fileNum].refOwner = myNode.nodeInfo;

				myNode.chordInfo.FRefInfo.fileNum++;
				ReleaseMutex(hMutex);
			}

			memset(&temp, 0, sizeof(temp)); //temp 초기화
			temp.msgID = 11;
			temp.msgType = 1;
			temp.nodeInfo = myNode.nodeInfo; //&
			temp.moreInfo = 1;
			temp.bodySize = 0;

			printf("<보냄>");
			msgNotice(temp.msgID, temp.msgType);
			//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
			sendto(rpSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &sendAddr, sizeof(struct sockaddr));

			break;

			//12. fileDown
		case 12:

			for (int i = 0; i < myNode.fileInfo.fileNum; i++) {
				if (myNode.fileInfo.fileRef[i].Key == buffer.fileInfo.Key) {
					index = i;
					break;
				}
			}
			if (mute == 1)
				printf(" index   %d \n", index);

			r_fp[sendCount] = fopen(myNode.fileInfo.fileRef[index].Name, "rb");
			if (mute == 1)
				printf("sendCount    %d  \n", sendCount);
			//파일 크기 알아내기//
			fseek(r_fp[sendCount], 0, SEEK_END);	//파일 끝으로 
			filesize = ftell(r_fp[sendCount]);        // 파일사이즈
													  /////////////////////

			memset(&temp, 0, sizeof(temp));
			temp.msgID = 12;
			temp.msgType = 1;
			temp.nodeInfo = buffer.nodeInfo;
			temp.fileInfo = myNode.fileInfo.fileRef[index];
			temp.moreInfo = sendCount;
			temp.bodySize = filesize;

			sendto(rpSock, (char *)&temp, sizeof(temp), 0,
				(struct sockaddr *) &sendAddr, sizeof(struct sockaddr));



			sendThread[sendCount] = (HANDLE)_beginthreadex(NULL, 0, (void *)fileSender, (void *)&temp, 0, NULL);
			//파일 전송 스레드 여러개
			printf("sendThread 시작 \n");
			sendCount += 1;

			break;

			//13. refOwner_update
		case 13:

			for (int i = 0; i < myNode.fileInfo.fileNum; i++) {
				if (myNode.fileInfo.fileRef[i].Key == buffer.moreInfo) {
					WaitForSingleObject(hMutex, INFINITE);
					myNode.fileInfo.fileRef[i].refOwner = buffer.nodeInfo;
					ReleaseMutex(hMutex);
					break;
				}
			}
			memset(&temp, 0, sizeof(temp));
			temp.msgID = 13;
			temp.msgType = 1;

			printf("<보냄>");
			msgNotice(temp.msgID, temp.msgType);
			//////msgPrint(myNode.nodeInfo.ID, procRecvMsgThread, temp, typetemp);
			sendto(rpSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &sendAddr, sizeof(struct sockaddr));


			break;

		}
	}
}


unsigned WINAPI procFixFinger(void *arg) {

	chordHeaderType temp, buffer;
	nodeInfoType succNode;
	nodeInfoType predNode;

	int closetCheck = 0; //find_closet_predecessor 여부 판별
	int retVal;
	int tempkey = 0; //각 핑거들의 modplus계산값

	struct sockaddr_in predAddr;
	struct sockaddr_in closetpredAddr;
	predAddr.sin_family = AF_INET;
	closetpredAddr.sin_family = AF_INET;

	//finger[1]~finger[baseM-1]의 find_successor 과정 요구되는 것
	while ((!(*exitFlag))) {
		Sleep(20000);
		printf("\n");

		for (int i = 1; i < baseM; i++) {
			closetCheck = 0;
			tempkey = modPlus(ringSize, myNode.nodeInfo.ID, twoPow(i));
			if (mute == 1)
				printf("[FixFinger] finger[%d] 현재 finger : %d  \n", myNode.nodeInfo.ID, i, myNode.chordInfo.fingerInfo.finger[i].ID);

			//tempkey에대한 find_successor과정

			if (myNode.nodeInfo.ID == tempkey) {
				succNode = myNode.nodeInfo;
				WaitForSingleObject(hMutex, INFINITE);
				myNode.chordInfo.fingerInfo.finger[i] = succNode;
				ReleaseMutex(hMutex);
			}

			else if (myNode.nodeInfo.ID == myNode.chordInfo.fingerInfo.finger[0].ID) {
				succNode = myNode.nodeInfo;
				WaitForSingleObject(hMutex, INFINITE);
				myNode.chordInfo.fingerInfo.finger[i] = succNode;
				ReleaseMutex(hMutex);
			}

			//find_predecessor과정
			else {

				closetCheck = 0;

				for (int i = baseM - 1; i >= 0; i--) {
					if (myNode.chordInfo.fingerInfo.finger[i].ID == -1)
						continue;

					if (modIn(ringSize, myNode.chordInfo.fingerInfo.finger[i].ID, myNode.nodeInfo.ID, tempkey, 0, 0)) {
						closetpredAddr = myNode.chordInfo.fingerInfo.finger[i].addrInfo;
						if (mute == 1)
							printf("<%d>[FixFinger] closetpredAddr에 저장된 노드의 번호 : %d\n", myNode.nodeInfo.ID, myNode.chordInfo.fingerInfo.finger[i].ID);
						closetCheck = 1;
						break;
					}
				}

				//find_closet_predecessor가 필요한 경우
				if (closetCheck == 1) {

					memset(&temp, 0, sizeof(temp)); //temp 초기화
					temp.msgID = 7;
					temp.msgType = 0;
					temp.nodeInfo = myNode.nodeInfo; //&
					temp.moreInfo = tempkey;
					temp.bodySize = 0;



					//request msg 보내기
					sendto(ffSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &closetpredAddr, sizeof(closetpredAddr));
					if (retVal != SOCKET_ERROR) {
						//response msg 받기
						retVal = recvfrom(ffSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);

						predNode = buffer.nodeInfo;
						predAddr = buffer.nodeInfo.addrInfo;
					}
					else {
						printf("  [FixFinger] 에러\n");
						break;
					}
				}


				//슈도코드의 find_closet_predecessor의 마지막라인 : 위의 반복문 if에걸리지 않으면 자신의 노드 반환
				else if (closetCheck == 0) {
					predNode = myNode.nodeInfo;
					predAddr = myNode.nodeInfo.addrInfo;
				}
				if (mute == 1)
					printf("<%d>[FixFinger] predNode : %d\n", myNode.nodeInfo.ID, predNode.ID);

				//successor_info 필요

				memset(&temp, 0, sizeof(temp)); //temp 초기화
				temp.msgID = 4;
				temp.msgType = 0;
				temp.nodeInfo = predNode;
				temp.bodySize = 0;

				sendto(ffSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr*) &predAddr, sizeof(struct sockaddr));


				retVal = recvfrom(ffSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);
				if (retVal != SOCKET_ERROR) {


					WaitForSingleObject(hMutex, INFINITE);
					myNode.chordInfo.fingerInfo.finger[i] = buffer.nodeInfo;
					ReleaseMutex(hMutex);

					if (mute == 1)
						printf("\n<%d>[FixFinger] 결정된 finger[%d]: %d\n", myNode.nodeInfo.ID, i, myNode.chordInfo.fingerInfo.finger[i].ID);
				}
				else {
					printf("/////////////////ff중에러\n");
					break;
				}
			}
		}
		printf(">>픽스 핑거 수행 완료 <<\n");

	}
}

unsigned WINAPI procPingPong(void *arg) {
	//myNode와 연결된 모든 노드들에게 pingpong수행

	chordHeaderType temp, buffer;

	int retVal;
	int leaveCheck = 0;// pingpong 수행 시 finger[0]이 null이 되면 해당 : 즉 현재 노드가 leave된 노드의 predecessor임
	int tempkey = 0; //현재 ping을 보낼 노드의 ID -> 만약 pong을 얻지 못한다면 tempkey값의 노드가 이탈함을 알 수 있다.

	nodeInfoType tempNode; //대체할 노드 설정

	struct sockaddr_in pingAddr;
	pingAddr.sin_family = AF_INET;

	while ((!(*exitFlag))) {
		Sleep(25000);
		printf("\n");

		for (int i = 0; i <= baseM; i++) {


			leaveCheck = 0;
			tempNode.ID = -2; //초기 설정 

							  //ping 보낼 주소 pingAddr 설정
							  //tempkey에도 ping보낼 node 저장 ->leave되는 경우 아이디 값을 사용하기 위함임

							  //predecessor
			if (i == baseM) {
				pingAddr = myNode.chordInfo.fingerInfo.Pre.addrInfo;
				tempkey = myNode.chordInfo.fingerInfo.Pre.ID;
			}

			//fingers
			else {
				pingAddr = myNode.chordInfo.fingerInfo.finger[i].addrInfo;
				tempkey = myNode.chordInfo.fingerInfo.finger[i].ID;
			}

			// 보내줄 필요는 없지만 출력로그를 쉽게 파악하기 위해 nodeInfo에 현재 노드 정보 넣어줌
			memset(&temp, 0, sizeof(temp)); //temp 초기화
			temp.msgID = 0;
			temp.msgType = 0;
			temp.nodeInfo = myNode.nodeInfo;
			temp.moreInfo = 0;
			temp.bodySize = 0;
			if (mute == 1) {
				if (i < 6) {
					printf("[PingPong] %d번째 핑거에 핑 보냄\n", i);

					if (myNode.nodeInfo.ID == myNode.chordInfo.fingerInfo.finger[i].ID) {
						printf("자신 노드에게 보냄\n");
					}
				}

				else {
					printf("[PingPong] 프레데세서 핑거에 핑 보냄\n");
				}
			}

			//printf("<보냄>");
			//msgNotice(temp.msgID, temp.msgType);
			//////msgPrint(myNode.nodeInfo.ID, PingPongThread, temp, typetemp);
			//request msg 보내기
			sendto(ppSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &pingAddr, sizeof(pingAddr));


			retVal = recvfrom(ppSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);

			//pong을 수신하지 못한 경우 ->tempkey 노드가 leave된 것으로 판정
			if (retVal == SOCKET_ERROR) {


				printf("사라진 노드 : %d\n", tempkey);

				//fingerInfo정보에 tempkey 노드있는 경우  다 null(-1)로 변경	
				for (int j = 0; j <= baseM; j++) {

					//predecessor
					if (j == baseM && myNode.chordInfo.fingerInfo.Pre.ID == tempkey) {
						WaitForSingleObject(hMutex, INFINITE);
						myNode.chordInfo.fingerInfo.Pre.ID = -1;
						ReleaseMutex(hMutex);
					}

					//fingers	
					else if (myNode.chordInfo.fingerInfo.finger[j].ID == tempkey) {

						if (j == 0) {
							//successor가 leave된 경우 myNode는 leave된 노드의 predecesssor로 stabilize를 진행한다.
							leaveCheck = 1;
						}

						WaitForSingleObject(hMutex, INFINITE);
						myNode.chordInfo.fingerInfo.finger[j].ID = -1;
						ReleaseMutex(hMutex);
					}
				}

				// -1(null)이 된 fingerInfo정보 채워주기 -predecessor제외

				for (int j = baseM - 1; j >= 0; j--) {

					if (myNode.chordInfo.fingerInfo.finger[j].ID != -1) {
						tempNode = myNode.chordInfo.fingerInfo.finger[j];
						if (mute == 1)
							printf("<<<<<<<<<<<<<tempNode결정 : %d >>>>>>>>>>>\n", tempNode.ID);
						continue;
					}

					else if (myNode.chordInfo.fingerInfo.finger[j].ID == -1) {

						if (tempNode.ID == -2) {


							if (myNode.chordInfo.fingerInfo.Pre.ID == -1) {

								WaitForSingleObject(hMutex, INFINITE);
								tempNode = myNode.nodeInfo;
								myNode.chordInfo.fingerInfo.finger[j] = tempNode;
								printf("************* finger[%d] 결정 :%d\n", j, myNode.chordInfo.fingerInfo.finger[j].ID);
								ReleaseMutex(hMutex);
							}
							else {

								WaitForSingleObject(hMutex, INFINITE);
								tempNode = myNode.chordInfo.fingerInfo.Pre;
								myNode.chordInfo.fingerInfo.finger[j] = tempNode;
								printf("************* finger[%d] 결정 :%d\n", j, myNode.chordInfo.fingerInfo.finger[j].ID);
								ReleaseMutex(hMutex);
							}
						}

						else {
							WaitForSingleObject(hMutex, INFINITE);
							myNode.chordInfo.fingerInfo.finger[j] = tempNode;
							printf("************* finger[%d] 결정 :%d\n", j, myNode.chordInfo.fingerInfo.finger[j].ID);
							ReleaseMutex(hMutex);
						}
					}

				}

				//predecessor 처리
				if (myNode.chordInfo.fingerInfo.Pre.ID == -1) {
					WaitForSingleObject(hMutex, INFINITE);
					myNode.chordInfo.fingerInfo.Pre = myNode.chordInfo.fingerInfo.finger[baseM - 1];
					printf("************* finger[pre] 결정 :%d\n", myNode.chordInfo.fingerInfo.Pre.ID);
					ReleaseMutex(hMutex);
				}

				//////////////////////////////////////////////////////////////////////////////////////////

				//myNode의 finger[0]이 null이 되었었다면 stabilize진행
				if (leaveCheck == 1) {
					//predecessor_info 요구해서 leave된 노드라면 현재노드로 predecessor update하면됨

					//predecessor_Info_request
					memset(&temp, 0, sizeof(temp));
					temp.msgID = 3;
					temp.msgType = 0;
					temp.nodeInfo = myNode.nodeInfo;
					temp.moreInfo = 0;
					temp.bodySize = 0;

					sendto(ppSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &myNode.chordInfo.fingerInfo.finger[0].addrInfo, sizeof(myNode.chordInfo.fingerInfo.finger[0].addrInfo));

					//response msg 받기
					retVal = recvfrom(ppSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);

					//predecessor_update_request
					memset(&temp, 0, sizeof(temp));
					temp.msgID = 5;
					temp.msgType = 0;
					temp.nodeInfo = myNode.nodeInfo;
					temp.moreInfo = 0;
					temp.bodySize = 0;



					sendto(ppSock, (char *)&temp, sizeof(temp), 0, (struct sockaddr *) &myNode.chordInfo.fingerInfo.finger[0].addrInfo, sizeof(myNode.chordInfo.fingerInfo.finger[0].addrInfo));

					//response msg 받기
					retVal = recvfrom(ppSock, (char *)&buffer, sizeof(buffer), 0, NULL, NULL);




				}


			}

			//정상적으로  pong을 수신한 경우
			else {
				if (mute == 1) {
					if (i < 6)
						printf("[PingPong] %d번째 핑거 퐁 받음 \n\n", i);
					else
						printf("[PingPong] 프레데세서 핑거 퐁 받음\n\n");
				}
				//////msgPrint(myNode.nodeInfo.ID, PingPongThread, buffer, typebuffer);
			}

		}

	}
}

unsigned WINAPI fileReceiver(void *arg) {
	int retVal;
	int addrlen;
	struct sockaddr_in frAddr;
	char buf[fBufSize];
	char fname[100];
	strcpy(fname, File.fileInfo.Name);


	retVal = listen(flSock, SOMAXCONN);
	if (retVal < 0) {
		printf("listen error!\n");
		exit(1);
	}

	while (!(*exitFlag)) {
		//accept();	
		addrlen = sizeof(frAddr);
		frSock = accept(flSock, (struct sockaddr *)&frAddr, &addrlen);
		if (frSock == INVALID_SOCKET) {
			printf("에러\n");
			continue;
		}
		printf("\nFileSender 접속: IP 주소=%s, 포트 번호=%d\n", inet_ntoa(frAddr.sin_addr), ntohs(frAddr.sin_port));


		printf("-> 받을 파일 이름: %s \n", File.fileInfo.Name);

		//파일 크기 받기 
		int totalbytes;
		retVal = recvn(frSock, (char *)&totalbytes, sizeof(totalbytes), 0);
		if (retVal == SOCKET_ERROR) {
			printf("recive error\n");
			closesocket(frSock);
			continue;
		}
		printf("-> 받을 파일 크기: %d\n", totalbytes);

		//파일 열기
		FILE *fp = fopen(File.fileInfo.Name, "wb");

		if (fp == NULL) {
			printf("파일 입출력 오류 \n");
			closesocket(frSock);
			continue;
		}


		//파일 데이터 받기
		int numtotal = 0;
		while (1) {
			retVal = recvn(frSock, buf, fBufSize, 0);
			if (retVal == SOCKET_ERROR) {
				printf("오류 \n");
				break;
			}
			else if (retVal == 0)
				break;
			else {
				fwrite(buf, 1, retVal, fp);
				numtotal += retVal;
			}
			fclose(fp);

			if (numtotal == totalbytes) {
				printf("파일 받기완료~ \n");
				break;
			}
			else {
				printf("파일 받기 실패...\n");
				break;
			}
			closesocket(frSock);
			printf("FileReceiver 종료: IP 주소=%s, 포트 번호=%d\n", inet_ntoa(frAddr.sin_addr), ntohs(frAddr.sin_port));

		}
	}

}

unsigned WINAPI fileSender(chordHeaderType* temp) {

	printf("센드 파일 시작 \n");
	int retVal, numRead;
	int numTotal = 0;
	chordHeaderType buffer = *temp;
	char fileBuf[fBufSize];
	struct sockaddr_in targetAddr;

	int filesize = temp->bodySize;
	printf("파일의 이름 %s  \n", temp->fileInfo.Name);
	printf("파일의 크기 %d \n", temp->bodySize);
	fsSock[sendCount - 1] = socket(AF_INET, SOCK_STREAM, 0);
	printf("send file 안의 sendCount    %d\n\n", sendCount - 1);

	printf("buffer의 nodeinfo의 IP    %s ", inet_ntoa(temp->nodeInfo.addrInfo.sin_addr));
	printf("buffer의 nodeinfo의 Port    %d ", ntohs(temp->nodeInfo.addrInfo.sin_port));


	targetAddr = temp->nodeInfo.addrInfo;

	retVal = connect(fsSock[sendCount - 1], (struct sockaddr *)&targetAddr, sizeof(targetAddr));
	if (retVal == SOCKET_ERROR) {
		printf("\a[ERROR] The file receiving socket of the peer node cannot be connected! \n");
	}
	//파일 사이즈 보내기
	retVal = send(fsSock[sendCount - 1], (char *)&temp->bodySize, sizeof(temp->bodySize), 0);

	// sending file
	rewind(r_fp[sendCount - 1]); // move file pointer to the starting point


	while (1) {
		numRead = fread(fileBuf, 1, fBufSize, r_fp[sendCount - 1]);
		if (numRead > 0) {
			retVal = send(fsSock[sendCount - 1], fileBuf, numRead, 0);

			if (retVal == SOCKET_ERROR) {
				printf("\a[ERROR] Error occurred in send()!   %d\n", temp);
				break;
			}
			numTotal += numRead;
		}
		else if (numRead == 0 && numTotal == buffer.bodySize) {

			printf("전송 완료~\n");
			break;
		}
		else {

			printf("파일 입출력 오류\n");
			break;
		}
	}

	fclose(r_fp[sendCount - 1]);
	closesocket(fsSock[sendCount - 1]);
	sendCount--;
}


//사용자 정의 데이터 수신 함수 (컴네 소켓 수업 자료)
int recvn(SOCKET s, char *buf, int len, int flags) {
	int received;
	char *ptr = buf;
	int left = len;
	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;

		//남아있는 경우 대비
		left -= received;
		ptr += received;
	}
	return (len - left);
}

void filePrint(nodeType curNode) {

	printf("<%d>[file] 파일 이름 : %s\n", curNode.nodeInfo.ID, curNode.fileInfo.fileRef[curNode.fileInfo.fileNum].Name);
	printf("<%d>[file] 파일 아이디 : %d\n", curNode.nodeInfo.ID, curNode.fileInfo.fileRef[curNode.fileInfo.fileNum].Key);
	printf("<%d>[file] 파일 소유노드 : %d\n", curNode.nodeInfo.ID, curNode.fileInfo.fileRef[curNode.fileInfo.fileNum].owner.ID);
	printf("<%d>[file] 파일 참조정보 소유노드 : %d\n", curNode.nodeInfo.ID, curNode.fileInfo.fileRef[curNode.fileInfo.fileNum].refOwner.ID);

}

void msgNotice(int num, int type) {

	//printf("\n");
	if (type == 0)
		printf("[REQUEST]");
	else
		printf("[RESPONSE]");

	switch (num) {

	case 0:
		printf("<<ping_pong>>\n");
		break;
	case 1:
		printf("<<join_info>>\n");
		break;
	case 2:
		printf("<<move_keys>>\n");
		break;
	case 3:
		printf("<<predecessor_info>>\n");
		break;
	case 4:
		printf("<<successor_info>>\n");
		break;
	case 5:
		printf("<<predecessor_update>>\n");
		break;
	case 6:
		printf("<<successor_update>>\n");
		break;
	case 7:
		printf("<<find_predecessor>>\n");
		break;
	case 8:
		printf("<<file_ref_add>>\n");
		break;
	case 9:
		printf("<<file_ref_delete>>\n");
		break;
	case 10:
		printf("<<file_ref_info>>\n");
		break;
	case 11:
		printf("<<Leave_keys>>\n");
		break;
	case 12:
		printf("<<file_down>>\n");
		break;
	case 13:
		printf("<<refOwner_update>>\n");
		break;

	default:
		printf("<<default! error>>\n");
		break;

	}


}

/////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int str_hash(const char *str)  /* Hash: String to Key */
{
	unsigned int len = sizeof(str);
	unsigned int hash = len, i;


	for (i = 0; i != len; i++, str++) {
		hash ^= sTable[(*str + i) & 255];
		hash = hash * PRIME_MULT;
	}

	return hash % ringSize;
}

void showComand(void) {
	printf("CHORD> Enter a command - (c)reate: Create the chord network\n");
	printf("CHORD> Enter a command - (j)oin: Join the chord network\n");
	printf("CHORD> Enter a command - (l)eave: Leave the chord network\n");
	printf("CHORD> Enter a command - (a)dd: Add a file to the network\n");
	printf("CHORD> Enter a command - (d)elete: Delete a file to the network\n");
	printf("CHORD> Enter a command - (s)earch: File search and download\n");
	printf("CHORD> Enter a command - (f)inger: Show the finger table\n");
	printf("CHORD> Enter a command - (i)nfo: Show the node information\n");
	printf("CHORD> Enter a command - (m)ute: Toggle the slient mode\n");
	printf("CHORD> Enter a command - (h)elp: Show the help message\n");
	printf("CHORD> Enter a command - (q)uit: Quit the program\n\n\n");
}

int modIn(int modN, int targNum, int range1, int range2, int leftmode, int rightmode)
// leftmode, rightmode: 0 => range boundary not included, 1 => range boundary included   
{
	int result = 0;

	if (range1 == range2) {
		if ((leftmode == 0) || (rightmode == 0))
			return 0;
	}

	if (modPlus(ringSize, range1, 1) == range2) {
		if ((leftmode == 0) && (rightmode == 0))
			return 0;
	}

	if (leftmode == 0)
		range1 = modPlus(ringSize, range1, 1);
	if (rightmode == 0)
		range2 = modMinus(ringSize, range2, 1);

	if (range1 < range2) {
		if ((targNum >= range1) && (targNum <= range2))
			result = 1;
	}
	else if (range1 > range2) {
		if (((targNum >= range1) && (targNum < modN)) || ((targNum >= 0) && (targNum <= range2)))
			result = 1;
	}
	else if ((targNum == range1) && (targNum == range2))
		result = 1;

	return result;
}

//2의 거듭제곱 함수 return 2^power 
int twoPow(int power)
{
	int i;
	int result = 1;

	if (power >= 0)
		for (i = 0; i < power; i++)
			result *= 2;
	else
		result = -1;

	return result;
}

int modMinus(int modN, int minuend, int subtrand)
{
	//인자2-인자3>=0   => return 인자2-인자3
	if (minuend - subtrand >= 0)
		return minuend - subtrand;

	//return 인자1-인자3+인자2
	else
		return (modN - subtrand) + minuend;
}

int modPlus(int modN, int addend1, int addend2)
{
	//인자2+인자3<인자1 => return 인자2+인자3
	if (addend1 + addend2 < modN)
		return addend1 + addend2;
	//return 인자2+인자3-인자1
	else
		return (addend1 + addend2) - modN;
}
