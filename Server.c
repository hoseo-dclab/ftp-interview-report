#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

//참고자료 : https://m.blog.naver.com/PostView.naver?isHttpsRedirect=true&blogId=so15284&logNo=221438613414
/*tcpLs, setLogIndex, writeLog함수는 저 본인이 직접 다 구현하였고
tcpGet, tcpPut 함수는 참고 자료의 코드를 수정하여 작업하였습니다
tcpListen, tcpCd, tcpQuit 함수는 참고한 자료의 것을 그대로 사용하였습니다*/
int tcpListen(int host, int port, int backlog);
int tcpLs(int sockMsg);
int tcpGet(int sockMsg, char *bufValue, int sockFile);
int tcpPut(int sockMsg, char *bufValue, char *comm_value, int sockFile);
int tcpCd(int sockMsg, char *bufValue);
int tcpQuit(int sockMsg);
void setLogIndex();
void writeLog(struct tm *date, double delay, int size, char *filename, char *cwd);

//로그 파일 인덱스 ex) {datatime}_{logIndex}.txt
int logIndex = 0;

int main(int argc, char *argv[]){
       //소켓 주소를 담는 구조체(IP + Port Number, address Family)
       struct sockaddr_in server, client;
       // sock1,3는 메시지 처리 소켓 담는 변수, sock2, 4는 파일 전송 처리 담는 변수
       int sock1, sock2, sock3, sock4;
       int input;
       // buf는 클라이언트에서 받은 명령어 받은 변수, command는 Server에서 명령어 처리 변수, filename는 get put 파일명 처리 변수
       char buf[100], command[5], filename[20];
       int result;
       // filehandle는 파일 전송 여부 확인
       int filehandle;
       int len;


       printf("Port 번호를 입력하세요 : ");
       scanf("%d", &input);

       // sock1_3 메시지 처리 소켓 2_4은 파일 전송 처리 소켓
       sock1 = tcpListen(INADDR_ANY, input, 5);
       sock2 = tcpListen(INADDR_ANY, input + 1, 6);
       len = sizeof(client);
       sock3 = accept(sock1, (struct sockaddr *)&client, &len);
       sock4 = accept(sock2, (struct sockaddr *)&client, &len);

       //로그 파일 인덱스 계산
       setLogIndex();

       while (1){
              //클라이언트에서 명령어를 입력받음
              recv(sock3, buf, 100, 0);
              sscanf(buf, "%s", command);

              if (!strcmp(command, "ls")){
                     // ls
                     result = tcpLs(sock3);

                     if (result == -1){
                            printf("ls 명령어 실패 !!\n");
                     }
                     else{
                            printf("ls 명령어 성공 !!\n");
                     }
              }
              else if (!strcmp(command, "get")){
                     // get
                     result = tcpGet(sock3, buf, sock4);

                     if (result == -1){
                            printf("파일 다운로드 실패!!\n");
                     }
                     else{
                            printf("파일 다운로드 성공!!\n");
                     }
              }
              else if (!strcmp(command, "put")){
                     // put
                     result = tcpPut(sock3, buf, command, sock4);

                     if (result == -1){
                            printf("파일 업로드 실패!!\n");
                     }
                     else{
                            printf("파일 업로드 성공!!\n");
                     }
              }
              else if (!strcmp(command, "cd")){
                     // cd
                     result = tcpCd(sock3, buf);

                     if (result == -1){
                            printf("\ncd 명령어 실행 실패 !!\n");
                     }
                     else{
                            printf("\ncd 명령어 실행 성공 !!\n");
                     }
              }
              else if (!strcmp(command, "quit")){
                     // quit
                     result = tcpQuit(sock3);

                     if (result == -1){
                            printf("Server 종료 실패 !!\n");
                     }
                     else{
                            printf("Server 종료 성공 !!\n");
                            exit(0);
                     }
              }
       }

       return 0;
}

int tcpListen(int host, int port, int backlog){
       int sd;
       struct sockaddr_in servaddr;

       //소켓 생성 함수(IPv4 사용 TCP/IP 프로토콜 사용)
       sd = socket(AF_INET, SOCK_STREAM, 0);

       if (sd == -1){
              //에러 내용 출력
              perror("socket fail!!");
              exit(1);
       }

       // bzero 함수를 이용해서 주소값을 초기화
       bzero((char *)&servaddr, sizeof(servaddr));
       servaddr.sin_family = AF_INET;
       // IP Address 주소 셋팅
       servaddr.sin_addr.s_addr = htonl(host);
       // Port 번호 셋팅
       servaddr.sin_port = htons(port);

       // IP주소와 포트번호를 지정
       if (bind(sd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
              perror("bind fail!!");
              exit(1);
       }

       //클라이언트의 접속 요청을 기다림
       listen(sd, backlog);

       return sd;
}

 /***********************
   * 작성자: 김영범
   * 작성일: 2022-07-12
   *
   * Param: sockMsg
   * Description: ls 기능 수행
   * Return: 제대로 동작했는지 확인하기 위한 value
   ************************/
int tcpLs(int sockMsg){       //명령어 실행 결과 리턴 변수
       int value;
       char output[4096];
       char result[4096] = "";
       FILE *fd;

       fd = popen("ls", "r");

       while (fgets(output, sizeof(output), fd) != NULL){
              strcat(result, output);
       }

       //서버나 클라이언트로 데이터를 전송
       value = send(sockMsg, (char*)result, strlen(result), 0);

       //파일 전송이 잘 되었다면 0 안 되었다면 -1
       pclose(fd);
       return value;
}

//파일 다운로드
int tcpGet(int sockMsg, char *bufValue, int sockFile){
       char filename[1024];
       char cwd[1024] = "";
       struct stat obj;
       int filehandle;
       int result;
       int size;
       double start = (double)clock() / CLOCKS_PER_SEC;
       double end;
       time_t date;

       time(&date);
       getcwd(cwd, sizeof(cwd));
       sscanf(bufValue, "%s%s", filename, filename);
       stat(filename, &obj);

       //파일을 쓸 준비를 해야됨
       filehandle = open(filename, O_RDONLY);
       size = obj.st_size;

       send(sockMsg, &filehandle, sizeof(int), 0);
       //메시지 전송
       send(sockMsg, &size, sizeof(int), 0);

       result = sendfile(sockFile, filehandle, NULL, size);
       end = (((double)clock()) / CLOCKS_PER_SEC);
       
       writeLog(localtime(&date), end-start, -1, filename, cwd);
       close(filehandle);
       return result;
}

//파일 업로드
int tcpPut(int sockMsg, char *bufValue, char *comm_value, int sockFile){
       int result;
       int content = 0;
       int len;
       char *fBuf;
       int size;
       int filehandle;
       int filename[50];
       double start = (double)clock() / CLOCKS_PER_SEC;
       double end;
       time_t date;
       char cwd[1024] = "";
       time(&date);
       getcwd(cwd, sizeof(cwd));
       sscanf(bufValue + strlen(comm_value), "%s", filename);

       recv(sockMsg, &size, sizeof(int), 0);

       filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);
       
       //빈파일일 경우
       if(size==0){
              send(sockMsg, &content, sizeof(int), 0);
              close(filehandle);
              end = (((double)clock()) / CLOCKS_PER_SEC);
              writeLog(localtime(&date), end-start, size, filename, cwd);
              return 1;
       }

       //파일 크기만큼 메모리 할당
       fBuf = calloc(size, sizeof(int));//malloc(size);
       //파일 전송
       recv(sockFile, fBuf, size, 0);
       //받은 파일을 쓰기
       content = write(filehandle, fBuf, size);
       //쓴 파일을 닫음     
       result = send(sockMsg, &content, sizeof(int), 0);
       close(filehandle);
       end = (((double)clock()) / CLOCKS_PER_SEC);
       free(fBuf);
       writeLog(localtime(&date), end-start, size, filename, cwd);
       return result;
}

// cd 명령어
int tcpCd(int sockMsg, char *bufValue){
       int value;
       int status;

       printf("%s", bufValue);

       //디렉토리 변경 함수
       if (chdir(bufValue + 3) == 0){
              status = 1;
       }
       else{
              status = 0;
       }

       //클라이언트로 메시지를 받음
       value = send(sockMsg, &status, sizeof(int), 0);

       return value;
}

// Sever 종료 함수
int tcpQuit(int sockMsg){
       int value;
       int status;

       value = send(sockMsg, &status, sizeof(int), 0);

       return value;
}
 /***********************
   * 작성자: 김영범
   * 작성일: 2022-07-13
   *
   * Param: void
   * Description: "/home/{userID}/Documents/log/"에 
   * 로그파일을 검사해 로그파일 인덱스 계산(경로는 함수 내부에서 직접 바꿔야 할 것 같습니다)
   * 같은 날짜의 로그 파일이 이미 존재하면 아직 30MB가 채워지지 않은 파일의 인덱스를 계산합니다
   * 만약 해당 날짜에 로그파일이 존재하지 않으면 logIndex를 -1로 둔 후
   * writeLog()함수에서 새파일을 만드는 플래그값으로 사용합니다
   * Return: void
   ************************/
void setLogIndex(){
       int index=-1;
       int i = 0;
       int flag = 0;
       char datatime[100] = "";
       time_t t;
       struct tm *date;
       DIR *dir;
       struct dirent *ent;

       dir = opendir("/home/ghost/Documents/log");
       time(&t);
       date = localtime(&t);
       strftime(datatime, sizeof(datatime), "%Y-%m-%d_", date);

       while ((ent = readdir(dir)) != NULL) {    
              if(strlen(ent->d_name) >= strlen(datatime)){
                     while(datatime[i] != NULL){
                            if(ent->d_name[i] == datatime[i]){
                                   i++;
                            }
                            else{
                                   i = 0;
                                   flag = 0;
                                   break;
                            }
                            flag = 1;
                     }
              }
              if(flag){
                     index++;
                     flag = 0;
              }
    }
    logIndex = index;
}

 /***********************
   * 작성자: 김영범
   * 작성일: 2022-07-13
   *
   * Param: 날짜, 소요 시간, 파일크기, 파일이름, 현재 경로
   * Description: "/home/{userID}/Documents/log/"에 로그파일을 생성 및 기록(경로는 함수 내부에서 직접 바꿔야 할 것 같습니다)
   * 로그파일명 형식 중 {datatime}_{index}.txt에서 datatime은 연도 월 일로 설정하였습니다
   * 날짜가 하루 지나면 새로 만들게 되고 같은 날짜에 30MB가 넘어가면 index를 증가시켜 파일을 생성합니다       
   * Return: void
   ************************/
void writeLog(struct tm *date, double delay, int size, char *filename, char *cwd){
       char datatime[100] = "";
       char logFilename[50] = "";
       char path[100] = "/home/ghost/Documents/log/";
       char extension[10] = "";
       char indexStr[10] = "";
       int i = 0;
       int j = 0;
       struct stat st;
       FILE *fp;

       while(filename[i] != NULL){
              if(filename[i] == '.'){
                     i++;
                     while(filename[i] != NULL){
                            extension[j] = filename[i];
                            i++;
                            j++;
                     }
                     break;
              }
              i++;
       }

       strftime(datatime, sizeof(datatime), "%Y-%m-%d_", date);
       strcat(path, datatime);
       strcat(logFilename, path);
       sprintf(indexStr, "%d", logIndex);
       strcat(path, indexStr);
       strcat(path, ".txt");
       
       if(logIndex != -1){
              stat(path, &st);
              if(st.st_size>=1024*1024*30){
                     logIndex++;
              }
              sprintf(indexStr, "%d", logIndex);
              strcat(logFilename, indexStr);
       }
       else{
              logIndex++;
              sprintf(indexStr, "%d", logIndex);
              strcat(logFilename, indexStr);
       }
       
       strftime(datatime, sizeof(datatime), "%Y-%m-%d %H:%M:%S", date);

       strcat(logFilename, ".txt");       
       fp = fopen(logFilename, "a");

       strcat(cwd, "/");
       strcat(cwd, filename);
       

       if(size != -1){
              fprintf(fp, "(put)시간 : %s, 파일명 : %s, 파일 크기 : %d, 확장자 : %s, 업로드 시간 : %f\n", datatime, cwd, size, extension, delay);
       }
       else{
              fprintf(fp, "(get)시간 : %s, 파일명 : %s, 확장자 : %s, 업로드 시간 : %f\n", datatime, cwd, extension, delay);
       }
       
       fclose(fp);
}