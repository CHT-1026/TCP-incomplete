#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <cwchar>
#include"resource.h"
#include<WinSock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#include <commdlg.h>
#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")

#pragma comment (lib,"gdiplus.lib")
using namespace Gdiplus;


#define WM_UPDATE_LISTBOX (WM_USER + 1)
//һЩ��Ҫ�ĺ궨��
static TCHAR szWindowClass[] = _T("DesktopApp");
static TCHAR szTitle[] = _T("����ͨ�ſͻ���");
static HWND hListBox;

Bitmap* bitmap = nullptr;
ULONG_PTR gdiplusToken;
HWND newWindow = nullptr;

void AddItemWithLeadingSpaces(HWND hListBox, const TCHAR* itemText, size_t numSpaces) {
    // ����һ���㹻��Ļ���������ſո���ı�
    const size_t bufferSize = 1024;  // ȷ�������С�㹻��
    TCHAR buffer[bufferSize];

    // ����ո���ı����ܳ���
    size_t textLength = _tcslen(itemText);
    size_t totalLength = numSpaces + textLength;

    // ȷ���������㹻��
    if (totalLength >= bufferSize) {
        MessageBox(NULL, _T("Buffer size is not sufficient"), _T("Error"), MB_OK | MB_ICONERROR);
        return;
    }

    // ��仺�����Ŀ�ͷ����Ϊ���������Ŀո�
    wmemset(buffer, L' ', numSpaces);

    // ����ԭʼ�ı����������Ŀո����
    wcscpy_s(buffer + numSpaces, bufferSize - numSpaces, itemText);

    // ʹ�� SendMessage ������������ӵ��б����
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)buffer);
}

//ת���������
void DisplayErrorMessage(HWND hWnd, int errorCode) {
    TCHAR errorMessage[256];
    _stprintf_s(errorMessage, sizeof(errorMessage) / sizeof(TCHAR), _T("�������: %d"), errorCode);
    MessageBox(hWnd, errorMessage, _T("��ʾ"), MB_OK | MB_ICONINFORMATION);
}

int IsSocketConnected(SOCKET sock) {
    char buf[1];
    int result = recv(sock, buf, sizeof(buf), MSG_PEEK);

    if (result == 0) {
        return 0; // �����ѹر�
    }
    else if (result == SOCKET_ERROR) {
        int errorCode = WSAGetLastError();
        if (errorCode == WSAENOTCONN) {
            return 0; // �׽���δ����
        }
        return -1; // ��������������
    }
    return 1; // ��������
}

int SendData(SOCKET client_socket, const char data_type, const char* data, int length) {
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
// �ļ�ѡ����
bool SelectFile(TCHAR* filePath, size_t size) {
    OPENFILENAME ofn;       // common dialog box structure
    TCHAR szFile[MAX_PATH] = { 0 }; // buffer for file name

    if (filePath == nullptr || size == 0) {
        return false;
    }

    // ��ʼ���ṹ��
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(TCHAR);
    ofn.lpstrFilter = _T("�����ļ� (*.*)\0*.*\0ͼƬ�ļ� (*.png)\0*.png\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        _tcscpy_s(filePath, size, ofn.lpstrFile);
        return true;
    }
    return false;
}



LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
//�������庯��
int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    WSADATA wsaDATA;
    WSAStartup(MAKEWORD(2, 2), &wsaDATA);



    //����ṹ��������ǲ��������ڵ�һЩ���õĹ���
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);       //Ŀǰ���ýṹ�Ĵ�С
    wcex.style = CS_HREDRAW | CS_VREDRAW;   //�ṹ������ʽ
    wcex.lpfnWndProc = WndProc;             //ָ�򴰿ڹ��̵�ָ��
    wcex.cbClsExtra = 0;                    //Ҫ���ݴ��ڽṹ��Ҫ����Ķ����ֽ�����Ĭ��Ϊ0
    wcex.cbWndExtra = 0;                    //�ڴ���ʵ��֮�����Ķ����ֽ����� ϵͳ���ֽڳ�ʼ��Ϊ�㡣
                                            //WNDCLASSEX ע��ʹ����Դ�ļ��е� CLASS ָ����ĶԻ�������뽫�˳�Ա����Ϊ DLGWINDOWEXTRA��
    wcex.hInstance = hInstance;             //ʵ���ľ������ʵ��������Ĵ��ڹ��̡�
    wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);//��ͼ��ľ��
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW); //���α�ľ��
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);//�౳�����ʵľ��
    wcex.lpszMenuName = NULL;//ָ���� null ��β���ַ�����ָ�룬���ַ���ָ����˵�����Դ����
    wcex.lpszClassName = szWindowClass;//ָ����������
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);




    //��windowsע�ᴰ����
    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }


    //�������ں�������������һ�����
    HWND hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 500,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    if (!hWnd)
    {
        MessageBox(NULL,
            _T("Call to CreateWindowEx failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }

    //��ʱ�����Ϣ�Ķ�ʱ��
    SetTimer(hWnd, 1, 100, NULL); // 100 ������һ��

    HWND button_connect = CreateWindow(
        _T("BUTTON"),  // Predefined class; Unicode assumed 
        _T("��������"),  // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        300,  // x position 
        360,  // y position 
        100,  // Button width
        30,  // Button height
        hWnd,  // Parent window
        (HMENU)BUTTON_1,  // Button ID
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),  // Instance handle
        NULL);  // Pointer not needed

    HWND button_send = CreateWindow(
        _T("BUTTON"),  // Predefined class; Unicode assumed 
        _T("������Ϣ"),  // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        300,  // x position 
        400,  // y position 
        100,  // Button width
        30,  // Button height
        hWnd,  // Parent window
        (HMENU)BUTTON_2,  // Button ID
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),  // Instance handle
        NULL);  // Pointer not needed


    HWND button_close = CreateWindow(
        _T("BUTTON"),  // Predefined class; Unicode assumed 
        _T("�ر�����"),  // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        300,  // x position 
        320,  // y position 
        100,  // Button width
        30,  // Button height
        hWnd,  // Parent window
        (HMENU)BUTTON_3,  // Button ID
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),  // Instance handle
        NULL);  // Pointer not needed

    HWND button_send_pic = CreateWindow(
        _T("BUTTON"),  // Predefined class; Unicode assumed 
        _T("����ͼƬ"),  // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        300,  // x position 
        280,  // y position 
        100,  // Button width
        30,  // Button height
        hWnd,  // Parent window
        (HMENU)BUTTON_4,  // Button ID
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),  // Instance handle
        NULL);  // Pointer not needed

    HWND hEdit = CreateWindowEx(
        0,                   // ��չ��ʽ
        TEXT("EDIT"),        // �ؼ�����
        NULL,                // ��ʼ�ı�
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, // �ؼ���ʽ
        10, 400,              // �ؼ�λ�� (x, y)
        250, 20,             // �ؼ���С (��, ��)
        hWnd,                // �����ھ��
        (HMENU)INPUT_1,     // �ؼ� ID
        GetModuleHandle(NULL), // ģ����
        NULL                 // ����Ĵ�������
    );




    //ʹ���ڶ�windows�ɼ�
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    //������Ϣ����ѭ��
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);//���ڴ����������Ϣ
        DispatchMessage(&msg);//����Ӧ����Ϣ���ݵ����̴�������
    }

    return (int)msg.wParam;
}


LRESULT CALLBACK newProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // �������
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        if (hWnd == newWindow)
        {
            Graphics graphics(hdc);

            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            int windowWidth = clientRect.right - clientRect.left;
            int windowHeight = clientRect.bottom - clientRect.top;

            // �������ű���
            float widthRatio = static_cast<float>(windowWidth) / bitmap->GetWidth();
            float heightRatio = static_cast<float>(windowHeight) / bitmap->GetHeight();
            float scaleRatio = min(widthRatio, heightRatio);

            // �������ź�ĳߴ�
            int displayWidth = static_cast<int>(bitmap->GetWidth() * scaleRatio);
            int displayHeight = static_cast<int>(bitmap->GetHeight() * scaleRatio);

            // ȷ������λ�ã�ʹ�����
            int x = (windowWidth - displayWidth) / 2;
            int y = (windowHeight - displayHeight) / 2;

            // ����ͼƬ
            graphics.DrawImage(bitmap, x, y, displayWidth, displayHeight);
        }

        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        // �ر��´���ʱ���˳�����
        delete bitmap;
        GdiplusShutdown(gdiplusToken);
        DestroyWindow(hWnd);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


int recvData(SOCKET client_socket, char* r_char_buffer, int buffer_size) {
    // ����׽����Ƿ���Ч
    if (client_socket == INVALID_SOCKET) {
        // �����׽�����Ч�����
        return -1;
    }

    // ����Ƿ������ݿɶ�
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_socket, &read_fds);
    struct timeval timeout = { 0 }; // ���ȴ�
    int result = select(0, &read_fds, NULL, NULL, &timeout);

    if (result > 0 && FD_ISSET(client_socket, &read_fds)) {
        char data_type;
        int recv_len = recv(client_socket, &data_type, 1, 0);
        if (recv_len != 1) {
            // ��������������ʹ���
            return -2; // ���Է��ز�ͬ�Ĵ������Ա����
        }

        unsigned short data_length;
        recv_len = recv(client_socket, (char*)&data_length, sizeof(data_length), 0);
        if (recv_len != sizeof(data_length)) {
            // ����������ݳ��ȴ���
            return -3;
        }

        // ȷ�����ݳ��Ȳ�����������
        if (data_length > buffer_size) {
            data_length = buffer_size;
        }

        recv_len = recv(client_socket, r_char_buffer, data_length, 0);
        if (recv_len != data_length) {
            // ����������ݴ���
            return -4;
        }

        if (data_type == 'T') {
            // ȷ���㹻��Ļ�����
            wchar_t wide_buffer[2048] = { 0 };

            // ת�� UTF-8 �����ַ�
            int wide_len = MultiByteToWideChar(CP_THREAD_ACP, 0, r_char_buffer, recv_len, wide_buffer, sizeof(wide_buffer) / sizeof(wchar_t));

            if (wide_len > 0) {
                // ȷ����'\0'��β

                AddItemWithLeadingSpaces(hListBox, wide_buffer, 0);
                // �����ı�������
                /*SetWindowTextW(hTextBox, wide_buffer);*/
            }
            else {
                // ����ת������
                DWORD error_code = GetLastError();
                TCHAR error_message[256];
                _stprintf_s(error_message, sizeof(error_message) / sizeof(TCHAR), TEXT("�ַ�ת��ʧ�ܣ��������: %lu"), error_code);
                MessageBox(NULL, error_message, TEXT("����"), MB_OK | MB_ICONERROR);
            }
        }
        else if (data_type == 'I') {
            // ����ͼƬ����
            FILE* file = fopen("received_image.png", "wb");
            if (file != NULL) {
                while (recv(client_socket, r_char_buffer, data_length < buffer_size ? data_length : buffer_size, 0)>0) {
                    fwrite(r_char_buffer, 1, recv_len, file);
                }
                fclose(file);
            }
            else {
                // �����ļ��򿪴���
                MessageBox(NULL, TEXT("�޷������ļ���"), TEXT("����"), MB_OK | MB_ICONERROR);
                return -5;
            }

            // ����������ʾͼƬ
            Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(L"received_image.png");

            WNDCLASS wc = { 0 };
            wc.lpfnWndProc = newProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"ImageWindowClass";
            RegisterClass(&wc);

            HWND newWindow = CreateWindowEx(
                0,
                L"ImageWindowClass",
                L"New Image Window",
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                NULL,
                NULL,
                GetModuleHandle(NULL),
                bitmap // ����bitmap����
            );

            ShowWindow(newWindow, SW_SHOW);
            UpdateWindow(newWindow);
        }
        else {
            // ����δ֪��������
            MessageBox(NULL, TEXT("δ֪���������ͣ�"), TEXT("����"), MB_OK | MB_ICONERROR);
            return -6;
        }
    }
    return 0; // �ɹ�
}
//���ڹ��̺����������������¼�ʱӦ�ó���� Windows �յ�����Ϣ
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //����һ���ͻ��˽ӿ�
    static SOCKET client_socket = INVALID_SOCKET; // ʹ�þ�̬�׽���
    TCHAR greeting[] = _T("Welcome to HT signal communicate simulation experiment!");

    //�����յ���ͬ��Ϣ��ʱ����в�ͬ�Ĵ���
    switch (message)
    {

    case WM_CREATE:
    {
        if (hWnd == newWindow)
        {

        }
        else
        {
            // �����׽���
            client_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (client_socket == INVALID_SOCKET)
            {
                MessageBox(hWnd, _T("�����ͻ����׽���ʧ�ܣ�"), _T("����"), MB_OK | MB_ICONERROR);
                return -1;
            }

            hListBox = CreateWindowEx(
                0, WC_LISTBOX, L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
                10, 50, 400, 200,
                hWnd, (HMENU)LIST_BOX_1, GetModuleHandle(NULL), NULL
            );

            GdiplusStartupInput gdiplusStartupInput;
            GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

        }


        break;
    }
        case WM_UPDATE_LISTBOX: {
            // ����lParam����Ҫ��ӵ��ַ���
            LPCTSTR newItem = (LPCTSTR)lParam;
            SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)newItem);
            break;
        }

        case WM_TIMER:
        {
            char buffer[2048] = { 0 }; // ��ʼ��Ϊ 0��ȷ�� buffer �Ŀ�ʼ�Ǹɾ���
            int ret = recvData(client_socket, buffer, sizeof(buffer)); // ʹ�� sizeof(buffer) ȷ����ȷ�Ļ�������С

            break;
        }


        case WM_COMMAND:
        {
            struct sockaddr_in target;
            target.sin_family = AF_INET;
            target.sin_port = htons(8080);
            target.sin_addr.s_addr = inet_addr("127.0.0.1");
            //��������������ӵİ�ť
            if (LOWORD(wParam) == BUTTON_1)
            {
                int con_ret = connect(client_socket, (struct sockaddr*)&target, sizeof(target));
                if (-1 == con_ret)
                {
                    printf("connect failed!  error code:%d", GetLastError());
                    MessageBox(hWnd, _T("��������ʧ�ܣ�"), _T("����"), MB_OK | MB_ICONINFORMATION);
                    closesocket(client_socket);
                    return -1;
                }
                else
                {
                    MessageBox(hWnd, _T("����������������ӣ�"), _T("��ʾ"), MB_OK | MB_ICONINFORMATION);

                       
                }


            }
            //������Ϣ�İ�ť
            else if (LOWORD(wParam) == BUTTON_2) // ������Ϣ
            {
                if (client_socket != INVALID_SOCKET)
                {
                    TCHAR s_buffer[256];
                    GetWindowText(GetDlgItem(hWnd, INPUT_1), s_buffer, 256);

                    char s_char_buffer[1024];
                    int length = WideCharToMultiByte(CP_ACP, 0, s_buffer, -1, s_char_buffer, sizeof(s_char_buffer), NULL, NULL);
                    if (length > 0)
                    {
                        if (SendData(client_socket, 'T', s_char_buffer, strlen(s_char_buffer)) != 1)
                        {
                            MessageBox(hWnd, _T("������Ϣʧ�ܣ�"), _T("����"), MB_OK | MB_ICONERROR);
                        }
                        AddItemWithLeadingSpaces(hListBox, s_buffer, 30);

                       
                    }

                }
                else
                {
                    MessageBox(hWnd, _T("δ���ӵ���������"), _T("����"), MB_OK | MB_ICONWARNING);
                }
            }

            else if (LOWORD(wParam) == BUTTON_3)
            {
                closesocket(client_socket);
                MessageBox(hWnd, _T("�����ѹرգ�"), _T("��ʾ"), MB_OK | MB_ICONWARNING);
            }

            //�����������һ��ͼƬ
            else if (LOWORD(wParam) == BUTTON_4)
            {
                char s_char_buffer[1024] = {0};
                TCHAR filePath[MAX_PATH];

                // ��ʾ�ļ�ѡ��Ի���
                if (!SelectFile(filePath, MAX_PATH)) {
                    DisplayErrorMessage(hWnd, CommDlgExtendedError());
                    MessageBox(hWnd, _T("δѡ���ļ�!"), _T("��ʾ"), MB_OK | MB_ICONINFORMATION);
                    return 0;
                }

                if (client_socket != INVALID_SOCKET)
                {
                    FILE* fp;
                    errno_t err = _tfopen_s(&fp, filePath, _T("rb"));
                    if (err != 0 || fp == NULL) {
                        MessageBox(hWnd, _T("�޷����ļ�!"), _T("����"), MB_OK | MB_ICONERROR);
                        return 0;
                    }

                    // ������������
                    char data_type = 'I';
                    int ret = send(client_socket, &data_type, 1, 0);
                    if (ret == SOCKET_ERROR)
                    {
                        MessageBox(hWnd, _T("�������ʹ���!"), _T("��ʾ"), MB_OK | MB_ICONINFORMATION);
                        fclose(fp);
                        return 0;
                    }

                    // ��ȡ�ļ���С���������ݳ���
                    fseek(fp, 0, SEEK_END);
                    long file_size = ftell(fp);
                    fseek(fp, 0, SEEK_SET);

                    unsigned short data_length = (unsigned short)file_size;
                    ret = send(client_socket, (char*)&data_length, sizeof(data_length), 0);
                    if (ret == SOCKET_ERROR)
                    {
                        MessageBox(hWnd, _T("���ͳ��ȴ���!"), _T("��ʾ"), MB_OK | MB_ICONINFORMATION);
                        fclose(fp);
                        return 0;
                    }
                    
                    // ����ͼ������
                    int bytesRead;
                    while ((bytesRead = fread(s_char_buffer, 1, sizeof(s_char_buffer), fp)) > 0)
                    {
                        ret = send(client_socket, s_char_buffer, bytesRead, 0);
                        if (ret == SOCKET_ERROR)
                        {
                            MessageBox(hWnd, _T("����ͼƬ���ݴ���!"), _T("��ʾ"), MB_OK | MB_ICONINFORMATION);
                            break;
                        }
                    }
                    MessageBox(hWnd, _T("ͼƬ���ͳɹ�!"), _T("��ʾ"), MB_OK | MB_ICONINFORMATION);

                    fclose(fp);

                }
            }
            break;
        }

        // ������Ϣ��ʱ��
       case WM_PAINT:
       {
           PAINTSTRUCT ps;
           HDC hdc = BeginPaint(hWnd, &ps);

           TextOut(hdc,
               5, 5,
               greeting, _tcslen(greeting));

           EndPaint(hWnd, &ps);
           break;
       }
        // ������������
        case WM_DESTROY:
        {

            delete bitmap;
            GdiplusShutdown(gdiplusToken);
            PostQuitMessage(0);
            break;
            
            
        }

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


