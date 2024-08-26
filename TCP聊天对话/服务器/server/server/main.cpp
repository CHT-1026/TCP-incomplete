#include <stdio.h>
#include <string.h>
#include <WinSock2.h>
#include <windows.h>
#include <tchar.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")


//ֻ���������û����û���
int user_num = 0;
SOCKET users[2] = { 0 };

// ��������
int SendData(SOCKET client_socket, char data_type, const char* data, int length) {
    // ������������
    if (send(client_socket, &data_type, 1, 0) == SOCKET_ERROR) {
        return -1;
    }

    // �������ݳ��ȣ����賤�Ȳ����� 65535��
    unsigned short data_length = (unsigned short)length;
    if (send(client_socket, (char*)&data_length, sizeof(data_length), 0) == SOCKET_ERROR) {
        return -1;
    }

    // ������������
    if (send(client_socket, data, length, 0) == SOCKET_ERROR) {
        return -1;
    }
    return 1;
}

// ��������
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

// ���� users ��ȫ�ֱ��������������û��� SOCKET
extern SOCKET users[2];

int recvData(SOCKET client_socket, char* r_char_buffer, int buffer_size) {
    if (client_socket == INVALID_SOCKET) {
        printf("��Ч�Ŀͻ��� socket��\n");
        return -1;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_socket, &read_fds);

    struct timeval timeout = { 0 };
    int result = select(0, &read_fds, NULL, NULL, &timeout);
    if (result == SOCKET_ERROR) {
        printf("select ����ʧ�ܣ��������: %d\n", WSAGetLastError());
        return -1;
    }
    if (result > 0 && FD_ISSET(client_socket, &read_fds)) {
        char data_type;
        int recv_len = recv(client_socket, &data_type, 1, 0);
        if (recv_len <= 0) {
            return -2;
        }

        if (recv_len == 1) {
            // ��ȡ���ݳ���
            unsigned short data_length;
            recv_len = recv(client_socket, (char*)&data_length, sizeof(data_length), 0);
            if (recv_len != sizeof(data_length)) {
                printf("�������ݳ���ʧ�ܣ��������: %d\n", WSAGetLastError());
                return -1;
            }

            // ȷ�����ݳ��Ȳ�����������
            if (data_length > buffer_size) {
                data_length = buffer_size;
            }

            if (data_type == 'T') {
                // �ı�����
                int total_received = 0;
                while (total_received < data_length) {
                    recv_len = recv(client_socket, r_char_buffer + total_received, data_length - total_received, 0);
                    if (recv_len <= 0) {
                        printf("�����ı�����ʧ�ܣ��������: %d\n", WSAGetLastError());
                        return -2;
                    }
                    total_received += recv_len;
                }
                r_char_buffer[total_received] = '\0'; // ��� null ��β
                printf("���յ����� %llu ���ı�: %s\n", (unsigned long long)client_socket, r_char_buffer);

                // �ط���ͬ���ı�����
                SOCKET target_socket = (client_socket == users[0]) ? users[1] : users[0];
                SendData(target_socket, 'T', r_char_buffer, total_received);

            }
            else if (data_type == 'I') {
                // ����ͼƬ����
                FILE* file = fopen("received_image.png", "wb");
                if (file == NULL) {
                    printf("�޷���ͼƬ�ļ�����д�룡�������: %d\n", GetLastError());
                    return -1;
                }

                int bytesRead;
                while ((bytesRead = recv(client_socket, r_char_buffer, buffer_size, 0)) > 0) {
                    fwrite(r_char_buffer, 1, bytesRead, file);
                }
                fclose(file);

                // ����ͼ�����ݵ���һ���ͻ���
                SOCKET target_socket = (client_socket == users[0]) ? users[1] : users[0];
                FILE* fp = fopen("C://Users//�ܺ���//Desktop//1.png", "rb");
                if (fp == NULL) {
                    printf("�޷���ͼƬ�ļ����ж�ȡ���������: %d\n", GetLastError());
                    return -1;
                }

                char data_type = 'I';
                //printf("%c", data_type);
                if (send(target_socket, &data_type, 1, 0) == SOCKET_ERROR) {
                    printf("����ͼƬ���ʹ��󣡴������: %d\n", WSAGetLastError());
                    fclose(fp);
                    return -1;
                }

                fseek(fp, 0, SEEK_END);
                long file_size = ftell(fp);
                fseek(fp, 0, SEEK_SET);

                unsigned short data_length = (unsigned short)file_size;
                //printf("%d", data_length);
                if (send(target_socket, (char*)&data_length, sizeof(data_length), 0) == SOCKET_ERROR) {
                    printf("����ͼƬ���ȴ��󣡴������: %d\n", WSAGetLastError());
                    fclose(fp);
                    return -1;
                }
                char s_char_buffer[1024] = { 0 };
                while ((bytesRead = fread(s_char_buffer, 1, sizeof(s_char_buffer), fp)) > 0) {
                    if (send(target_socket, s_char_buffer, bytesRead, 0) == SOCKET_ERROR) {
                        printf("����ͼƬ���ݴ��󣡴������: %d\n", WSAGetLastError());
                        return -1;
                    }
                    //printf("%s",s_char_buffer);
                }

                fclose(fp);
                printf("ͼƬ���ͳɹ���\n");

            }
            else {
                printf("δ֪���������ͣ�\n");
                return -1;
            }
        }
        else {
            return -1;
        }
        return 0;
    }
    return -1;
}
// �̺߳���
DWORD WINAPI thread_func(LPVOID lpThreadParameter) {
    SOCKET client_socket = *(SOCKET*)lpThreadParameter;
    free(lpThreadParameter);

    while (1) {
        char buffer[1024] = { 0 }; // ��ʼ��Ϊ 0��ȷ�� buffer �Ŀ�ʼ�Ǹɾ���
        int ret = recvData(client_socket, buffer, sizeof(buffer)); // ʹ�� sizeof(buffer) ȷ����ȷ�Ļ�������С
        if (ret > 0) {
            printf("Client %llu: %s\n", (unsigned long long)client_socket, buffer);
        }
        else if (ret == -2) {
            break;
        }
    }

    // ȷ���׽��ֹر�
    closesocket(client_socket);
    printf("Client %llu disconnected!\n", (unsigned long long)client_socket);
    user_num--;
    return 0;
}

int main() {
    WSADATA wsaDATA;
    if (WSAStartup(MAKEWORD(2, 2), &wsaDATA) != 0) {
        printf("WSAStartup failed! Error code: %d\n", WSAGetLastError());
        return 1;
    }

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == INVALID_SOCKET) {
        printf("Create listen socket failed! Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in local = { 0 };
    local.sin_addr.s_addr = inet_addr("0.0.0.0");
    local.sin_family = AF_INET;
    local.sin_port = htons(8080);

    if (bind(listen_socket, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
        printf("Bind socket failed! Error code: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    if (listen(listen_socket, 10) == SOCKET_ERROR) {
        printf("Listen failed! Error code: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    while (1) {
        if (user_num <= 2)
        {
            SOCKET client_socket = accept(listen_socket, NULL, NULL);
            if (client_socket == INVALID_SOCKET) {
                printf("Accept failed! Error code: %d\n", WSAGetLastError());
                continue;
            }
            printf("Client connected: %llu\n", (unsigned long long)client_socket);
            users[user_num] = client_socket;
            SOCKET* sockfd = (SOCKET*)malloc(sizeof(SOCKET));
            *sockfd = client_socket;

            HANDLE hThread = CreateThread(NULL, 0, thread_func, sockfd, 0, NULL);
            if (hThread == NULL) {
                printf("Error creating thread.\n");
                free(sockfd);
                closesocket(client_socket);
            }
            else {
                CloseHandle(hThread);
            }
            user_num++;
            if (user_num == 2)
            {
                printf("ͨ���û��Ѵ����");
            }
        }

    }

    closesocket(listen_socket);
    WSACleanup();
    return 0;
}