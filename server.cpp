// ! 服务器端，采用TCP套接字编程

/**
 * mingw和msvc不同,导致链接库失效
 * mingw: g++ server.cpp -lws2_32 -o server.exe
 * msvc: 直接运行
 */

#include <iostream>
#include <string>
#include <thread>

// ? 文件处理
#include <unordered_map>
#include <locale>
#include <codecvt>
#include <fstream>

// ? 网络编程
// #include "stdafx.h"
#include <winsock2.h>

// ? Windows系统调用
#include <Windows.h>

const int PORT = 8080;
const std::string FILE_PATH = "./dictionary.txt";

#pragma comment(lib, "ws2_32.lib") // ! attention!!!

void LoadDictionary(std::unordered_map<std::string, std::string> &dictionary)
{   
    std::ifstream file(FILE_PATH);
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            /* code */
            size_t pos = line.find(",");
            if (pos != std::string::npos && pos < line.length() - 1)
            {
                std::string word = line.substr(0, pos);
                std::string translation = line.substr(pos + 1);
                dictionary[word] = translation;
                std::cout<<"Word: "<<word<<"\tTranslation: "<<translation<<std::endl;
            }
        }
        file.close();
    }
}


// ! 多线程处理客户端连接
void HandleClient(SOCKET ClientSocket,const std::unordered_map<std::string, std::string> &dictionary)
{
    char buffer[1024];
    std::string query;

    // 进行多次交互
    while(true)
    {
        // ? 接受要查询的单词
        int bytesRead;
        while ((bytesRead = recv(ClientSocket, buffer, sizeof(buffer) - 1, 0)) > 0)
        {
            /* code */
            buffer[bytesRead] = '\0';
            query += buffer;
            


            if (query.find('\n') != std::string::npos)
            {
                query.erase(query.find('\n'));

                // std::cout<<"The query: "<<query<<std::endl;

                break;
            }
        }

        if(bytesRead <=0)
        {
            /**
             * 只有当接收数据时出现错误（recv 返回值小于等于 0）时，
             * 才会退出循环并关闭连接。
             * 这样客户端可以在不关闭连接的情况下发送多个查询。
             * 当客户端关闭连接时，recv 将返回 0，循环也会退出并关闭连接
             */
            break;
        }


        // ? 处理查询
        std::string translation = "Translation not found";
        auto it = dictionary.find(query);
        if (it != dictionary.end())
        {
            translation = it->second;
        }
        translation += "\n";

        send(ClientSocket, translation.c_str(), translation.length(), 0);
        query.clear();
    }

    SOCKADDR_IN ClientAddr;
    int ClientAddrLen = sizeof(ClientAddr);
    getpeername(ClientSocket, (SOCKADDR*)&ClientAddr, &ClientAddrLen);
    std::cout << "Close: " << inet_ntoa(ClientAddr.sin_addr) << ":" << ntohs(ClientAddr.sin_port) << std::endl;

    closesocket(ClientSocket);
}


int main()
{
    WSADATA wasData;
    int result = WSAStartup(MAKEWORD(2, 2), &wasData);
    if(result!=0)
    {
        std::cerr<<"WSAStartup failed with error: "<<result<<std::endl;
        return 1;
    }

    // ? 初始化哈希表
    std::unordered_map<std::string, std::string> dictionary;
    LoadDictionary(dictionary);


    // 创建套接字
    SOCKET ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(ServerSocket==INVALID_SOCKET)
    {
        std::cerr<<"socket creation failed with error: "<<WSAGetLastError()<<std::endl;
        WSACleanup();
        return 1;
    }

    //  设置套接字
    SOCKADDR_IN ServerAddr;
    memset(&ServerAddr, 0, sizeof(ServerAddr));
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(PORT);
    ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 绑定套接字
    if(bind(ServerSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr))==SOCKET_ERROR)
    {
        std::cerr<<"bind failed with error: "<<WSAGetLastError()<<std::endl;
        closesocket(ServerSocket);
        WSACleanup();
        return 1;
    }


    // 监听套接字，最大连接3个
    if(listen(ServerSocket, 3)==SOCKET_ERROR)
    {
        std::cerr<<"listen failed with error: "<<WSAGetLastError()<<std::endl;
        closesocket(ServerSocket);
        WSACleanup();
        return 1;
    }

    std::cout<< "Server listening on port "<<PORT<<" with a maximum queue size of 3"<<std::endl;

    while (1)
    {
        /* code */
        SOCKADDR_IN ClientAddr;
        int ClientAddrLen = sizeof(ClientAddr);

        SOCKET ClientSocket = accept(ServerSocket, (SOCKADDR*)&ClientAddr, &ClientAddrLen);
        if(ClientSocket==INVALID_SOCKET)
        {
            std::cerr<<"accept failed with error: "<<WSAGetLastError()<<std::endl;
            closesocket(ServerSocket);
            WSACleanup();
            return 1;
        }

        // 客户端信息
        // getpeername(ClientSocket, (SOCKADDR *)&ClientAddr, &ClientAddrLen);
        std::cout<<"Client connected with : "<<inet_ntoa(ClientAddr.sin_addr)<<":"<<ntohs(ClientAddr.sin_port)<<std::endl;

        // 创建线程处理客户端请求
        std::thread(HandleClient, ClientSocket, std::cref(dictionary)).detach();

    }

    closesocket(ServerSocket);

    WSACleanup();

    return 0;
}

