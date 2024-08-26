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
//一些需要的宏定义
static TCHAR szWindowClass[] = _T("DesktopApp");
static TCHAR szTitle[] = _T("网络通信客户端");
static HWND hListBox;

Bitmap* bitmap = nullptr;
ULONG_PTR gdiplusToken;
HWND newWindow = nullptr;

void AddItemWithLeadingSpaces(HWND hListBox, const TCHAR* itemText, size_t numSpaces) {
    // 创建一个足够大的缓冲区来存放空格和文本
    const size_t bufferSize = 1024;  // 确保这个大小足够大
    TCHAR buffer[bufferSize];

    // 计算空格和文本的总长度
    size_t textLength = _tcslen(itemText);
    size_t totalLength = numSpaces + textLength;

    // 确保缓冲区足够大
    if (totalLength >= bufferSize) {
        MessageBox(NULL, _T("Buffer size is not sufficient"), _T("Error"), MB_OK | MB_ICONERROR);
        return;
    }

    // 填充缓冲区的开头部分为所需数量的空格
    wmemset(buffer, L' ', numSpaces);

    // 复制原始文本到缓冲区的空格后面
    wcscpy_s(buffer + numSpaces, bufferSize - numSpaces, itemText);

    // 使用 SendMessage 函数将新项添加到列表框中
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)buffer);
}

//转换错误代码
void DisplayErrorMessage(HWND hWnd, int errorCode) {
    TCHAR errorMessage[256];
    _stprintf_s(errorMessage, sizeof(errorMessage) / sizeof(TCHAR), _T("错误代码: %d"), errorCode);
    MessageBox(hWnd, errorMessage, _T("提示"), MB_OK | MB_ICONINFORMATION);
}

int IsSocketConnected(SOCKET sock) {
    char buf[1];
    int result = recv(sock, buf, sizeof(buf), MSG_PEEK);

    if (result == 0) {
        return 0; // 连接已关闭
    }
    else if (result == SOCKET_ERROR) {
        int errorCode = WSAGetLastError();
        if (errorCode == WSAENOTCONN) {
            return 0; // 套接字未连接
        }
        return -1; // 发生了其他错误
    }
    return 1; // 连接正常
}

int SendData(SOCKET client_socket, const char data_type, const char* data, int length) {
    // 发送数据类型

    if (send(client_socket, &data_type, 1, 0) == SOCKET_ERROR) {
        return -1;
    }

    // 发送数据长度（假设长度不超过 65535）
    unsigned short data_length = (unsigned short)length;
    if (send(client_socket, (char*)&data_length, sizeof(data_length), 0) == SOCKET_ERROR) {
        return -1;
    }

    // 发送数据内容
    if (send(client_socket, data, length, 0) == SOCKET_ERROR) {
        return -1;
    }
    return 1;
}
// 文件选择函数
bool SelectFile(TCHAR* filePath, size_t size) {
    OPENFILENAME ofn;       // common dialog box structure
    TCHAR szFile[MAX_PATH] = { 0 }; // buffer for file name

    if (filePath == nullptr || size == 0) {
        return false;
    }

    // 初始化结构体
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(TCHAR);
    ofn.lpstrFilter = _T("所有文件 (*.*)\0*.*\0图片文件 (*.png)\0*.png\0");
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
//窗口主体函数
int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    WSADATA wsaDATA;
    WSAStartup(MAKEWORD(2, 2), &wsaDATA);



    //这个结构体的作用是捕获主窗口的一些有用的功能
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);       //目前设置结构的大小
    wcex.style = CS_HREDRAW | CS_VREDRAW;   //结构的类样式
    wcex.lpfnWndProc = WndProc;             //指向窗口过程的指针
    wcex.cbClsExtra = 0;                    //要根据窗口结构需要分配的额外字节数，默认为0
    wcex.cbWndExtra = 0;                    //在窗口实例之后分配的额外字节数。 系统将字节初始化为零。
                                            //WNDCLASSEX 注册使用资源文件中的 CLASS 指令创建的对话框，则必须将此成员设置为 DLGWINDOWEXTRA。
    wcex.hInstance = hInstance;             //实例的句柄，该实例包含类的窗口过程。
    wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);//类图标的句柄
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW); //类游标的句柄
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);//类背景画笔的句柄
    wcex.lpszMenuName = NULL;//指向以 null 结尾的字符串的指针，该字符串指定类菜单的资源名称
    wcex.lpszClassName = szWindowClass;//指定窗口类名
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);




    //向windows注册窗口类
    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }


    //创建窗口函数，函数返回一个句柄
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

    //定时检查消息的定时器
    SetTimer(hWnd, 1, 100, NULL); // 100 毫秒检查一次

    HWND button_connect = CreateWindow(
        _T("BUTTON"),  // Predefined class; Unicode assumed 
        _T("建立连接"),  // Button text 
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
        _T("发送消息"),  // Button text 
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
        _T("关闭连接"),  // Button text 
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
        _T("发送图片"),  // Button text 
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
        0,                   // 扩展样式
        TEXT("EDIT"),        // 控件类名
        NULL,                // 初始文本
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, // 控件样式
        10, 400,              // 控件位置 (x, y)
        250, 20,             // 控件大小 (宽, 高)
        hWnd,                // 父窗口句柄
        (HMENU)INPUT_1,     // 控件 ID
        GetModuleHandle(NULL), // 模块句柄
        NULL                 // 额外的创建参数
    );




    //使窗口对windows可见
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    //处理消息的主循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);//用于处理虚拟键消息
        DispatchMessage(&msg);//将相应的消息传递到过程处理函数中
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

        // 清除背景
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        if (hWnd == newWindow)
        {
            Graphics graphics(hdc);

            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            int windowWidth = clientRect.right - clientRect.left;
            int windowHeight = clientRect.bottom - clientRect.top;

            // 计算缩放比例
            float widthRatio = static_cast<float>(windowWidth) / bitmap->GetWidth();
            float heightRatio = static_cast<float>(windowHeight) / bitmap->GetHeight();
            float scaleRatio = min(widthRatio, heightRatio);

            // 计算缩放后的尺寸
            int displayWidth = static_cast<int>(bitmap->GetWidth() * scaleRatio);
            int displayHeight = static_cast<int>(bitmap->GetHeight() * scaleRatio);

            // 确定绘制位置，使其居中
            int x = (windowWidth - displayWidth) / 2;
            int y = (windowHeight - displayHeight) / 2;

            // 绘制图片
            graphics.DrawImage(bitmap, x, y, displayWidth, displayHeight);
        }

        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        // 关闭新窗口时不退出程序
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
    // 检查套接字是否有效
    if (client_socket == INVALID_SOCKET) {
        // 处理套接字无效的情况
        return -1;
    }

    // 检查是否有数据可读
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_socket, &read_fds);
    struct timeval timeout = { 0 }; // 不等待
    int result = select(0, &read_fds, NULL, NULL, &timeout);

    if (result > 0 && FD_ISSET(client_socket, &read_fds)) {
        char data_type;
        int recv_len = recv(client_socket, &data_type, 1, 0);
        if (recv_len != 1) {
            // 处理接收数据类型错误
            return -2; // 可以返回不同的错误码以便诊断
        }

        unsigned short data_length;
        recv_len = recv(client_socket, (char*)&data_length, sizeof(data_length), 0);
        if (recv_len != sizeof(data_length)) {
            // 处理接收数据长度错误
            return -3;
        }

        // 确保数据长度不超过缓冲区
        if (data_length > buffer_size) {
            data_length = buffer_size;
        }

        recv_len = recv(client_socket, r_char_buffer, data_length, 0);
        if (recv_len != data_length) {
            // 处理接收数据错误
            return -4;
        }

        if (data_type == 'T') {
            // 确定足够大的缓冲区
            wchar_t wide_buffer[2048] = { 0 };

            // 转换 UTF-8 到宽字符
            int wide_len = MultiByteToWideChar(CP_THREAD_ACP, 0, r_char_buffer, recv_len, wide_buffer, sizeof(wide_buffer) / sizeof(wchar_t));

            if (wide_len > 0) {
                // 确保以'\0'结尾

                AddItemWithLeadingSpaces(hListBox, wide_buffer, 0);
                // 更新文本框内容
                /*SetWindowTextW(hTextBox, wide_buffer);*/
            }
            else {
                // 处理转换错误
                DWORD error_code = GetLastError();
                TCHAR error_message[256];
                _stprintf_s(error_message, sizeof(error_message) / sizeof(TCHAR), TEXT("字符转换失败，错误代码: %lu"), error_code);
                MessageBox(NULL, error_message, TEXT("错误"), MB_OK | MB_ICONERROR);
            }
        }
        else if (data_type == 'I') {
            // 处理图片数据
            FILE* file = fopen("received_image.png", "wb");
            if (file != NULL) {
                while (recv(client_socket, r_char_buffer, data_length < buffer_size ? data_length : buffer_size, 0)>0) {
                    fwrite(r_char_buffer, 1, recv_len, file);
                }
                fclose(file);
            }
            else {
                // 处理文件打开错误
                MessageBox(NULL, TEXT("无法创建文件！"), TEXT("错误"), MB_OK | MB_ICONERROR);
                return -5;
            }

            // 创建窗口显示图片
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
                bitmap // 传递bitmap对象
            );

            ShowWindow(newWindow, SW_SHOW);
            UpdateWindow(newWindow);
        }
        else {
            // 处理未知数据类型
            MessageBox(NULL, TEXT("未知的数据类型！"), TEXT("错误"), MB_OK | MB_ICONERROR);
            return -6;
        }
    }
    return 0; // 成功
}
//窗口过程函数，用来处理发生事件时应用程序从 Windows 收到的消息
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //开启一个客户端接口
    static SOCKET client_socket = INVALID_SOCKET; // 使用静态套接字
    TCHAR greeting[] = _T("Welcome to HT signal communicate simulation experiment!");

    //用于收到不同消息的时候进行不同的处理
    switch (message)
    {

    case WM_CREATE:
    {
        if (hWnd == newWindow)
        {

        }
        else
        {
            // 创建套接字
            client_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (client_socket == INVALID_SOCKET)
            {
                MessageBox(hWnd, _T("创建客户端套接字失败！"), _T("错误"), MB_OK | MB_ICONERROR);
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
            // 假设lParam包含要添加的字符串
            LPCTSTR newItem = (LPCTSTR)lParam;
            SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)newItem);
            break;
        }

        case WM_TIMER:
        {
            char buffer[2048] = { 0 }; // 初始化为 0，确保 buffer 的开始是干净的
            int ret = recvData(client_socket, buffer, sizeof(buffer)); // 使用 sizeof(buffer) 确保正确的缓冲区大小

            break;
        }


        case WM_COMMAND:
        {
            struct sockaddr_in target;
            target.sin_family = AF_INET;
            target.sin_port = htons(8080);
            target.sin_addr.s_addr = inet_addr("127.0.0.1");
            //与服务器建立连接的按钮
            if (LOWORD(wParam) == BUTTON_1)
            {
                int con_ret = connect(client_socket, (struct sockaddr*)&target, sizeof(target));
                if (-1 == con_ret)
                {
                    printf("connect failed!  error code:%d", GetLastError());
                    MessageBox(hWnd, _T("建立连接失败！"), _T("警告"), MB_OK | MB_ICONINFORMATION);
                    closesocket(client_socket);
                    return -1;
                }
                else
                {
                    MessageBox(hWnd, _T("已与服务器建立连接！"), _T("提示"), MB_OK | MB_ICONINFORMATION);

                       
                }


            }
            //发送消息的按钮
            else if (LOWORD(wParam) == BUTTON_2) // 发送消息
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
                            MessageBox(hWnd, _T("发送消息失败！"), _T("错误"), MB_OK | MB_ICONERROR);
                        }
                        AddItemWithLeadingSpaces(hListBox, s_buffer, 30);

                       
                    }

                }
                else
                {
                    MessageBox(hWnd, _T("未连接到服务器！"), _T("警告"), MB_OK | MB_ICONWARNING);
                }
            }

            else if (LOWORD(wParam) == BUTTON_3)
            {
                closesocket(client_socket);
                MessageBox(hWnd, _T("连接已关闭！"), _T("提示"), MB_OK | MB_ICONWARNING);
            }

            //向服务器发送一张图片
            else if (LOWORD(wParam) == BUTTON_4)
            {
                char s_char_buffer[1024] = {0};
                TCHAR filePath[MAX_PATH];

                // 显示文件选择对话框
                if (!SelectFile(filePath, MAX_PATH)) {
                    DisplayErrorMessage(hWnd, CommDlgExtendedError());
                    MessageBox(hWnd, _T("未选择文件!"), _T("提示"), MB_OK | MB_ICONINFORMATION);
                    return 0;
                }

                if (client_socket != INVALID_SOCKET)
                {
                    FILE* fp;
                    errno_t err = _tfopen_s(&fp, filePath, _T("rb"));
                    if (err != 0 || fp == NULL) {
                        MessageBox(hWnd, _T("无法打开文件!"), _T("错误"), MB_OK | MB_ICONERROR);
                        return 0;
                    }

                    // 发送数据类型
                    char data_type = 'I';
                    int ret = send(client_socket, &data_type, 1, 0);
                    if (ret == SOCKET_ERROR)
                    {
                        MessageBox(hWnd, _T("发送类型错误!"), _T("提示"), MB_OK | MB_ICONINFORMATION);
                        fclose(fp);
                        return 0;
                    }

                    // 获取文件大小并发送数据长度
                    fseek(fp, 0, SEEK_END);
                    long file_size = ftell(fp);
                    fseek(fp, 0, SEEK_SET);

                    unsigned short data_length = (unsigned short)file_size;
                    ret = send(client_socket, (char*)&data_length, sizeof(data_length), 0);
                    if (ret == SOCKET_ERROR)
                    {
                        MessageBox(hWnd, _T("发送长度错误!"), _T("提示"), MB_OK | MB_ICONINFORMATION);
                        fclose(fp);
                        return 0;
                    }
                    
                    // 发送图像数据
                    int bytesRead;
                    while ((bytesRead = fread(s_char_buffer, 1, sizeof(s_char_buffer), fp)) > 0)
                    {
                        ret = send(client_socket, s_char_buffer, bytesRead, 0);
                        if (ret == SOCKET_ERROR)
                        {
                            MessageBox(hWnd, _T("发送图片数据错误!"), _T("提示"), MB_OK | MB_ICONINFORMATION);
                            break;
                        }
                    }
                    MessageBox(hWnd, _T("图片传送成功!"), _T("提示"), MB_OK | MB_ICONINFORMATION);

                    fclose(fp);

                }
            }
            break;
        }

        // 绘制消息的时候
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
        // 窗口正在销毁
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


