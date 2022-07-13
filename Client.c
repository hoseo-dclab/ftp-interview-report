#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define MAXLINE 200
//참고자료 : https://m.blog.naver.com/PostView.naver?isHttpsRedirect=true&blogId=so15284&logNo=221438613414
/*ftpHelp()는 직접구현, 
ftpPut, ftpGet, ftpLs 함수는 참고자료의 코드를 수정하여 작업하였습니다
그 외의 함수는 참고자료의 코드를 그대로 이용하였습니다*/
int tcpConnect(int af, char *servip, unsigned int port);
int ftpPut(int sockMsg, int sockFile);
int ftpGet(int sockMsg, int sockFile);
int ftpLs(int sockMsg);
int ftpCd(int sockMsg);
int ftpQuit(int sockMsg);
void ftpHelp();
int lcd();
void ftpPrint();

int main(int argc, char *argv[]){
       struct sockaddr_in server;
       struct stat obj;
       int sock, sock1;
       char bufmsg[MAXLINE];
       int port;
       char ip[MAXLINE];
       int result;

       port = atoi(argv[2]);
       while (1){
              // sock은 메시지 처리 sock1은 파일 처리 메시지 전송은 입력한 포트 번호 파일 전송은 입력한 포트번호에 +1
              sock = tcpConnect(AF_INET, argv[1], port); // bufmsg
              sock1 = tcpConnect(AF_INET, argv[1], port + 1);

              if (sock == -1){
                     printf("FTP 연결 실패 !!\n");
                     exit(1);
              }
              else{
                     printf("FTP 연결 성공 !!\n");
                     while (1){
                            ftpPrint();
                            //메세지를 입력 받았을 경우
                            scanf("%s", bufmsg);

                            if (!strcmp(bufmsg, "get")){
                                   result = ftpGet(sock, sock1);

                                   if (result == -1){
                                          printf("get 명령어를 정상적으로 실행시키지 못했습니다.\n");
                                   }
                                   else{
                                          printf("get 명령어를 정상적으로 실행시켰습니다\n");
                                   }
                            }
                            else if (!strcmp(bufmsg, "put")){
                                   result = ftpPut(sock, sock1);

                                   if (result == -1){
                                          printf("put 명령어를 정상적으로 실행시키지 못했습니다.\n");
                                   }
                                   else{
                                          printf("put 명령어를 정상적으로 실행시켰습니다.\n");
                                   }
                            }
                            else if (!strcmp(bufmsg, "ls")){
                                   result = ftpLs(sock);

                                   if (result == -1){
                                          printf("ls 명령어를 정상적으로 실행시키지 못했습니다.\n");
                                   }
                                   else{
                                          printf("ls 명령어를 정상적으로 실행했습니다.\n");
                                   }
                            }
                            else if (!strcmp(bufmsg, "cd")){
                                   result = ftpCd(sock);

                                   if (result == -1){
                                          printf("cd 명령어를 정상적으로 실행시키지 못했습니다.\n");
                                   }
                                   else{
                                          printf("cd 명령어를 정상적으로 실행시켰습니다.\n");
                                   }
                            }
                            else if (!strcmp(bufmsg, "quit")){
                                   result = ftpQuit(sock);
                                   if (result == -1){
                                          printf("서버를 종료시키지 못했습니다.\n");
                                   }
                                   else{
                                          printf("서버를 종료합니다.\n");
                                          exit(1);
                                   }
                            }
                            else if (!strcmp(bufmsg, "lcd")){
                                   result = lcd();

                                   if (result == -1){
                                          printf("lcd 명령어를 정상적으로 실행시키지 못했습니다.\n");
                                   }
                                   else{
                                          printf("lcd 명령어를 정상적으로 실행시켰습니다.\n");
                                   }
                            }
                            else if (!strcmp(bufmsg, "help")){
                                   ftpHelp();
                            }
                            else{
                                   printf("다시 입력해주세요\n");
                            }
                     }
              }
       }
}

//연결 함수
int tcpConnect(int af, char *servip, unsigned int port){
       struct sockaddr_in servaddr;
       int sock;

       if ((sock = socket(af, SOCK_STREAM, 0)) < 0){
              return -1;
       }

       bzero((char *)&servaddr, sizeof(servaddr));
       servaddr.sin_family = af;
       inet_pton(AF_INET, servip, &servaddr.sin_addr);
       servaddr.sin_port = htons(port);

       if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
              return -1;
       }

       return sock;
}

//업로드 함수
int ftpPut(int sockMsg, int sockFile)
{
       char filename[MAXLINE];
       char temp[20];
       int filehandle;
       char buf[20];
       struct stat obj;
       int size;
       int value;
       int status;

       ftpPrint();
       scanf("%s", filename);
       //fgets(temp, MAXLINE, stdin);
       filehandle = open(filename, O_RDONLY);

       if (filehandle == -1){
              printf("파일이 없습니다.\n");
              return -1;
       }

       //버퍼에 문자열 복사
       strcpy(buf, "put ");
       // buf에 파일 이름을 합쳐놓기
       strcat(buf, filename);
       //서버로 메시지를 보내고
       send(sockMsg, buf, 100, 0);
       stat(filename, &obj);
       size = obj.st_size;
       //서버로 메시지를 보내고
       send(sockMsg, &size, sizeof(int), 0);
       //파일을 서버로 보내고
       sendfile(sockFile, filehandle, NULL, size);
       //서버로부터 메시지를 받음
       
       value = recv(sockMsg, &status, sizeof(int), 0);
       if(size==0) status = 1;

       if (status){
              printf("업로드 완료\n");
       }
       else{
              printf("업로드 실패\n");
       }
       close(filehandle);
       return value;
}

//파일 다운로드 함수
int ftpGet(int sockMsg, int sockFile){
       char buf[50];
       char filename[MAXLINE];
       char temp[MAXLINE];
       int size;
       int value;
       char *fBuf;
       int filehandle;

       ftpPrint();
       scanf("%s", filename);
       fgets(temp, MAXLINE, stdin);
       // get 문자열을 buf로 복사
       strcpy(buf, "get ");
       // buf과 filename 문자열을 합침
       strcat(buf, filename);
       //메시지를 서버로 보냄
       send(sockMsg, buf, 100, 0);
       recv(sockMsg, &filehandle, sizeof(int), 0);
       //메시지를 서버에서 받음
       recv(sockMsg, &size, sizeof(int), 0);

       //파일이 없을 경우
       if (filehandle == -1){
              printf("파일이 없습니다\n");
              return -1;
       }
       if(size == 0){
              filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);
              close(filehandle);
              return 1;
       }
       //파일이 있으면 파일 크기만큼 동적할당
       fBuf = malloc(size);
       //서버로부터 파일을 받음
       recv(sockFile, fBuf, size, 0);
       while (1){
              filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);

              if (filehandle == -1){
                     sprintf(filename + strlen(filename), "_1");
              }
              else{
                     break;
              }
       }

       //파일을 쓰고
       write(filehandle, fBuf, size, 0);
       //파일을 받음
       close(filehandle);
       free(fBuf);
       printf("다운로드 완료!!\n");

       return value;
}

// ls 명령어
int ftpLs(int sockMsg){
       char buf[100];
       int size;
       int filehandle;
       int value;
       char result[4096] = "";
       
       strcpy(buf,"ls");
       send(sockMsg, buf, 100,0);
       value = recv(sockMsg, (char*)result, sizeof(result), 0);
       
       printf("%s", result);
       return value;
}

// cd 명령어
int ftpCd(int sockMsg){
       char buf[100];
       char temp[20];
       int value;
       int status;

       strcpy(buf, "cd ");
       ftpPrint();
       scanf("%s", buf + 3);
       fgets(temp, MAXLINE, stdin);
       send(sockMsg, buf, 100, 0);
       recv(sockMsg, &status, sizeof(int), 0);

       printf("%d\n", status);

       if (status){
              printf("경로 변경 완료\n");
       }
       else{
              printf("경로 변경 실패\n");
       }

       return value;
}

// FTP 종료 명령어 보낼 시
int ftpQuit(int sockMsg){
       int status;
       char buf[100];
       strcpy(buf, "quit");
       send(sockMsg, buf, 100, 0);
       status = recv(sockMsg, &status, 100, 0);

       return status;
}

//클라이언트 cd 명령어 실행
int lcd(){
       int result;
       char input[100];

       strcpy(input, "cd ");
       ftpPrint();
       scanf("%s", input + 3);

       if (chdir(input + 3) == 0){
              result = 0;
       }
       else{
              result = -1;
       }

       return result;
}

// FTP Print 출력
void ftpPrint(){
       printf("ftp>");
}

 /***********************
   * 작성자: 김영범
   * 작성일: 2022-07-11
   *
   * Param: void
   * Description: ftp 도움말 출력
   * Return: void
   ************************/
void ftpHelp(){
       printf("cd : 서버의 CWD 변경\n");
       printf("lcd : 클라이언트의 CWD 변경\n");
       printf("ls : 서버의 CWD 파일 목록 디스플레이\n");
       printf("get : 지정 파일 다운로드\n");
       printf("put : 지정 파일 업로드\n");
       printf("quit : 서버와의 연결 해제 후 프로그램 종료\n");
}
