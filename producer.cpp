// ! 解析文件程序

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <Windows.h>

#define BUFFER_SIZE 4096

const std::string FILE_NAME = "Init.txt";

void RemoveNonAlphanumeric(std::string &str)
{
    str.erase(std::remove_if(str.begin(), str.end(), [](char c){ return !isalnum(c); }),str.end());
}

int main(int argc, char* argv[])
{
    std::cout<<"Producer started!"<<std::endl;

    std::string MainID;
    if (argv[1] == NULL)
    {
        MainID="0";
    }else
    {
        MainID=argv[1];
    }


    std::string proMutexName = std::string(MainID) + std::string("proMutex");
    std::string conMutexName = std::string(MainID) + std::string("conMutex");

    // ? 互斥量
    // HANDLE Mutex = OpenSemaphore(SEMAPHORE_MODIFY_STATE, FALSE, "Mutex");
    // HANDLE proMutex = OpenSemaphore(SEMAPHORE_MODIFY_STATE, FALSE, "proMutex");
    // HANDLE conMutex = OpenSemaphore(SEMAPHORE_MODIFY_STATE, FALSE, "conMutex");

    // HANDLE Mutex = CreateSemaphore(NULL, 1, 1, "Mutex");
    HANDLE proMutex = CreateSemaphore(NULL, 0, 1, proMutexName.c_str());
    HANDLE conMutex = CreateSemaphore(NULL, 1, 1, conMutexName.c_str());

    if (conMutex == NULL || proMutex == NULL)
    {
        std::cerr << "Failed to open semaphores.! Error code: " << GetLastError() << std::endl;
        return -1;
    }

    // ? 创建管道
    HANDLE hPipe = CreateNamedPipe(
        "\\\\.\\pipe\\MyPipe",
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        BUFFER_SIZE,
        BUFFER_SIZE,
        0,
        NULL);
    if(hPipe==INVALID_HANDLE_VALUE)
    {
        std::cerr<<"Failed to create named pipe. Error code: "<<GetLastError()<<std::endl;

        return -1;
    }
    if (!ConnectNamedPipe(hPipe, NULL))
    {
        std::cerr << "Failed to connect pipe! Error code: " << GetLastError() << std::endl;
        CloseHandle(hPipe);
        // CloseHandle(Mutex);
        CloseHandle(proMutex);
        CloseHandle(conMutex);
        return -1;
    }
    

    while (true)
    {
        /* code */

        char buffer[256];
        DWORD bytesRead;

        // int temp;
        // std::cout << "Get filename: ";
        // std::cin >> temp;

        // std::cout << "P(proMutex)";
        WaitForSingleObject(proMutex, INFINITE);
        // std::cout<<"Get(promutex)"<<std::endl;

        // std::cout << "P(Mutex)->";
        // WaitForSingleObject(Mutex, INFINITE);
        // std::cout << "Get(Mutex)" << std::endl;

        if(ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead>0 )
        {
            // std::cout << "Get filename..." << std::endl;
            buffer[bytesRead] = '\0';   
            std::string fileName(buffer);
            std::cout << "Received file name: " << fileName << std::endl;

            if(fileName=="exit")
            {
                // std::cout << "V(Mutex)" << std::endl;
                // ReleaseSemaphore(Mutex, 1, NULL);

                // std::cout << "V(conMutex)" << std::endl;
                ReleaseSemaphore(conMutex, 1, NULL);

                break;
            }else
            {
                std::ifstream InputFile(fileName, std::ios::in);
                if (InputFile.is_open())
                {
                    std::string line;
                    std::stringstream wordsStream;

                    while (std::getline(InputFile, line))
                    {
                        /* code */
                        std::istringstream iss(line);
                        std::string word;
                        while (iss >> word)
                        {
                            /* code */
                            RemoveNonAlphanumeric(word);
                            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                            if (!word.empty())
                            {
                                wordsStream << word << "\n";
                            }
                        }
                    }

                    InputFile.close();

                    std::string words = wordsStream.str();

                    // WaitForSingleObject(hMutex, INFINITE);
                    DWORD bytesWritten;
                    WriteFile(hPipe, words.c_str(), words.length(), &bytesWritten, NULL);
                    std::cout << "words writed" << std::endl;
                }
                else
                {
                    std::string words = "";

                    // WaitForSingleObject(hMutex, INFINITE);
                    DWORD bytesWritten;
                    WriteFile(hPipe, words.c_str(), words.length(), &bytesWritten, NULL);
                    std::cerr << "Failed to open file for reading." << std::endl;
                }
            }

            
        }

        // std::cout << "V(Mutex)" << std::endl;
        // ReleaseSemaphore(Mutex,1,NULL);

        // std::cout << "V(conMutex)" << std::endl;
        ReleaseSemaphore(conMutex,1,NULL);



    }

    CloseHandle(hPipe);
    // CloseHandle(Mutex);
    CloseHandle(proMutex);
    CloseHandle(conMutex);

    return 0;
}