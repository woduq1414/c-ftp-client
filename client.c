#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>

#define BUF_SIZE 1024

void error_handling(char *message);

char currrent_command[20] = "";

char *send_message(int sock, char *command, char *msg)
{
    strcpy(currrent_command, command);
    char result[BUF_SIZE];
    sprintf(result, "%s %s\n", command, msg);
    char buf[BUF_SIZE];
    int str_len = strlen(result);
    int send_len = write(sock, result, str_len);
    if (send_len != str_len)
    {
        error_handling("send() error");
    }
    printf("\n Client -> Server : %s %s", command, msg);


void send_file_data(char *fileServIp, int fileServPort, FILE *fp)
{
    int file_sock;
    struct sockaddr_in file_serv_addr;
    char message[BUF_SIZE];

    file_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (file_sock == -1)
        error_handling("socket() error");

    memset(&file_serv_addr, 0, sizeof(file_serv_addr));
    file_serv_addr.sin_family = AF_INET;
    file_serv_addr.sin_addr.s_addr = inet_addr(fileServIp);
    file_serv_addr.sin_port = htons(fileServPort);

    if (connect(file_sock, (struct sockaddr *)&file_serv_addr, sizeof(file_serv_addr)) == -1)
        error_handling("connect() error!");

    while (1)
    {
        char buf[BUF_SIZE];
        int nbyte = fread(buf, 1, BUF_SIZE, fp);
        if (nbyte <= 0)
        {

            break;
        }
        int send_len = write(file_sock, buf, strlen(buf));

        if (send_len == -1)
        {
            error_handling("write() error!!!");
        }
        memset(&buf, 0, BUF_SIZE);
    }
    printf("\n Client -> Server : [FILE_DATA]");

    fclose(fp);
    close(file_sock);
}

int connect_server(char *ip, int port)
{
    int sock;
    struct sockaddr_in serv_addr;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error!");

    return sock;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage : %s <FileName>\n", argv[0]);
        exit(1);
    }

    int sock;
    char message[BUF_SIZE];
    int str_len;
    int port = 21;
    char ip[20];
    char userName[20];
    char password[20];
    char fname[200];
    bool isLocal = false;
    if (isLocal)
    {
        strcpy(ip, "192.168.21.1");
        strcpy(userName, "jaeyeop");
        strcpy(password, "qwe123");
    }
    else
    {
        strcpy(ip, "192.168.100.2");
        strcpy(userName, "netprog");
        strcpy(password, "netprog");
    }

    strcpy(fname, argv[1]);

    FILE *fp = NULL;

    fp = fopen(fname, "rb");

    if (fp == NULL)
    {
        printf("file open error \n");
        exit(1);
    }

    sock = connect_server(ip, port);

    int fd;
    char buf[255];
    int state;

    struct timeval tv;
    fd_set readfds;
    fd = sock;


    FD_ZERO(&readfds);

    char fileSendAddress[20];
    int fileSendPort;

    for (;;)
    {
        FD_SET(fd, &readfds);

        tv.tv_sec = 2;
        tv.tv_usec = 0;

        fflush(0);

        state = select(fd + 1, &readfds, (fd_set *)0, (fd_set *)0, &tv);
        switch (state)
        {
        case -1:
            perror("select error : ");
            exit(0);
            break;
        case 0:
            printf("\n Connection End\n");
            close(sock);

            return 0;
        default:
            memset(buf, 0, sizeof(buf));
            read(fd, buf, sizeof(buf));

            char cmd_code_char[4];
            int cmd_code;

            char cmd_msg[100];

            memcpy(&cmd_code_char, &buf, sizeof(cmd_code));
            cmd_code = atoi(cmd_code_char);
            memcpy(&cmd_msg, &buf[sizeof(cmd_code)], sizeof(cmd_msg));

            printf("\n Server -> Client : %d - %s", cmd_code, cmd_msg);
        
            int arr[6];

            switch (cmd_code)
            {
            case 200:
                if (!strcmp(currrent_command, "TYPE"))
                {
                    send_message(sock, "PASV", "");
                }
                break;
            case 215:
                send_message(sock, "TYPE", "I");

                break;
            case 221:
                close(sock);
                return 0;
                break;
            case 220:
                send_message(sock, "USER", userName);
                break;

            case 230:
                send_message(sock, "SYST", "");
                break;

            case 331:
                send_message(sock, "PASS", password);
                break;

            case 227:

                sscanf(cmd_msg, "Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &arr[0], &arr[1], &arr[2], &arr[3], &arr[4], &arr[5]);
                sprintf(fileSendAddress, "%d.%d.%d.%d", arr[0], arr[1], arr[2], arr[3]);
                fileSendPort = arr[4] * 256 + arr[5];
                send_message(sock, "STOR", fname);
                break;

            case 150:

                send_file_data(fileSendAddress, fileSendPort, fp);
                break;

            case 226:

                send_message(sock, "QUIT", "");
                break;

            default:

                printf("\n COMMAND %s ERROR, cmd_code : %d", currrent_command, cmd_code);
                break;
            }
            FD_CLR(sock, &readfds);
        }
    }

    close(sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}