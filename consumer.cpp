// ! 客户端

#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <set>

#pragma comment(lib, "ws2_32.lib")

const char *SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 8080;

int main(int argc, char *argv[])
{
    std::cout << "Consumer started!" << std::endl;

    std::string MainID;
    if (argv[1] == NULL)
    {
        MainID = "0";
    }
    else
    {
        MainID = argv[1];
    }

    std::string proMutexName = std::string(MainID) + std::string("proMutex");
    std::string conMutexName = std::string(MainID) + std::string("conMutex");

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup failed!" << std::endl;
        return -1;
    }

    SOCKET ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ClientSocket == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in ServerAddr;
    memset(&ServerAddr, 0, sizeof(ServerAddr));
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    ServerAddr.sin_port = htons(SERVER_PORT);

    if (connect(ClientSocket, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
        closesocket(ClientSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Connected to server!" << std::endl;

    // HANDLE Mutex = OpenSemaphore(SEMAPHORE_MODIFY_STATE, FALSE, "Mutex");
    HANDLE proMutex = OpenSemaphore(SEMAPHORE_MODIFY_STATE, FALSE, proMutexName.c_str());
    HANDLE conMutex = OpenSemaphore(SEMAPHORE_MODIFY_STATE, FALSE, conMutexName.c_str());
    if (proMutex == NULL || conMutex == NULL)
    {
        std::cerr << "Failed to open semaphores Error code: " << GetLastError() << std::endl;
        return -1;
    }

    // 检查可用的命名管道
    if (!WaitNamedPipe("\\\\.\\pipe\\MyPipe", 20))
    {
        std::cerr << "Failed to wait named pipe. Error code: " << GetLastError() << std::endl;
        return -1;
    }

    // 打卡可用的命名管道进行通信
    HANDLE hPipe = CreateFile(
        "\\\\.\\pipe\\MyPipe",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to open pipe. Error code: " << GetLastError() << std::endl;
        // CloseHandle(Mutex);

        closesocket(ClientSocket);

        WSACleanup();

        CloseHandle(proMutex);
        CloseHandle(conMutex);

        return -1;
    }

    int select = 0;
    std::cout << "-----------------Modeling Select-----------------" << std::endl;
    std::cout << "Input 1 to translate a word, 2 to translate a file: ";
    std::cin >> select;
    std::cout << "-------------------------------------------------" << std::endl;

    while (select==1 || select==2)
    {
        std::string query;
        std::string filename;
        if (select == 1)
        {
            std::cout << "Enter a word to translate (type 'exit' to quit):";
            std::cin >> query;
        }
        else if (select == 2)
        {
            std::cout << "Enter the file name (or 'exit' to quit): ";
            std::cin >> filename;
        }

        if (select == 1)
        {
            if (query == "exit")
            {
                // std::cout << "P(conMutex)";
                WaitForSingleObject(conMutex, INFINITE);
                DWORD bytesWritten;
                WriteFile(hPipe, query.c_str(), query.length(), &bytesWritten, NULL);
                std::cout << "Exiting..." << std::endl;

                // std::cout << "V(proMutex)" << std::endl;
                ReleaseSemaphore(proMutex, 1, NULL);

                break;
            }
            else
            {
                // 变长消息分界符号
                query += "\n";

                if (send(ClientSocket, query.c_str(), query.length(), 0) == SOCKET_ERROR)
                {
                    std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
                    closesocket(ClientSocket);
                    WSACleanup();
                    return -1;
                }

                char buffer[1024];
                int bytesRead;
                std::string translation;

                while ((bytesRead = recv(ClientSocket, buffer, sizeof(buffer) - 1, 0)) > 0)
                {
                    buffer[bytesRead] = '\0';
                    // std::cout << buffer << std::endl;
                    translation += buffer;

                    if (translation.find("\n") != std::string::npos)
                    {
                        translation.erase(translation.find("\n"));
                        break;
                    }
                }

                if (bytesRead == SOCKET_ERROR)
                {
                    std::cerr << "Receive failed: " << WSAGetLastError() << std::endl;
                    closesocket(ClientSocket);
                    WSACleanup();
                    return -1;
                }

                // buffer[bytesRead] = '\0';

                // 查找换行符并删除
                size_t found = query.find("\n");
                if (found != std::string::npos)
                    query.erase(found);

                std::cout << query << " : " << translation << std::endl;
                // std::cout << "Enter a word to translate (type 'exit' to quit): ";
            }
        }
        else if (select == 2)
        {
            // std::cout << "P(conMutex)" << std::endl;
            WaitForSingleObject(conMutex, INFINITE);

            DWORD bytesWritten;
            WriteFile(hPipe, filename.c_str(), filename.length(), &bytesWritten, NULL);
            // std::cout << "Exiting..." << std::endl;
            std::cout << "fielname passed" << std::endl;

            // std::cout << "V(proMutex)" << std::endl;
            ReleaseSemaphore(proMutex, 1, NULL);


            if(filename=="exit")
            {
                break;
            }

            // std::cout << "P(conMutex)" << std::endl;
            WaitForSingleObject(conMutex, INFINITE);

            char buffer[4096] = {0};
            DWORD bytesRead;
            if (ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0)
            {
                buffer[bytesRead] = '\0';
                std::string words(buffer);
                // std::cout << "Words: \n" << words << std::endl;
            }

            // std::cout << "V(conMutex)" << std::endl;
            ReleaseSemaphore(conMutex, 1, NULL);

            std::string words(buffer);
            std::istringstream iss(words);
            std::set<std::string> wordList;

            // 分割字符串并存储到set中
            do
            {
                std::string word;
                iss >> word;
                wordList.insert(word);
            } while (iss);

            // 查询每个单词
            for (auto query : wordList)
            {
                query += "\n";
                if (send(ClientSocket, query.c_str(), query.length(), 0) == SOCKET_ERROR)
                {
                    std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
                    closesocket(ClientSocket);
                    WSACleanup();
                    return -1;
                }

                char buffer[1024];
                int bytesRead;
                std::string translation;

                while ((bytesRead = recv(ClientSocket, buffer, sizeof(buffer) - 1, 0)) > 0)
                {
                    buffer[bytesRead] = '\0';
                    // std::cout << buffer << std::endl;
                    translation += buffer;

                    if (translation.find("\n") != std::string::npos)
                    {
                        translation.erase(translation.find("\n"));
                        break;
                    }
                }

                if (bytesRead == SOCKET_ERROR)
                {
                    std::cerr << "Receive failed: " << WSAGetLastError() << std::endl;
                    closesocket(ClientSocket);
                    WSACleanup();
                    return -1;
                }

                // buffer[bytesRead] = '\0';

                // 查找换行符并删除
                size_t found = query.find("\n");
                if (found != std::string::npos)
                    query.erase(found);

                std::cout << query << " : " << translation << std::endl;
            }
        }
    }
    if(select!=1 && select!= 2)
    {
        std::string EXITstr = "exit";
        WaitForSingleObject(conMutex, INFINITE);
        DWORD bytesWritten;
        WriteFile(hPipe, EXITstr.c_str(), EXITstr.length(), &bytesWritten, NULL);
        std::cout << "Exiting..." << std::endl;

        // std::cout << "V(proMutex)" << std::endl;
        ReleaseSemaphore(proMutex, 1, NULL);
    }
    
    CloseHandle(hPipe);
    // CloseHandle(Mutex);
    CloseHandle(proMutex);
    CloseHandle(conMutex);

    closesocket(ClientSocket);

    WSACleanup();

    return 0;
}
