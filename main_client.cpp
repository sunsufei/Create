// ! 客户端主进程


#include <iostream>
#include <Windows.h>
#include <string>
#include <sstream>

wchar_t *ConvertToWideString(const char *narrowString)
{
    int length = strlen(narrowString) + 1;
    int wideLength = MultiByteToWideChar(CP_UTF8, 0, narrowString, length, NULL, 0);

    wchar_t *wideString = new wchar_t[wideLength];
    MultiByteToWideChar(CP_UTF8, 0, narrowString, length, wideString, wideLength);

    return wideString;
}

int main(int argc, char *argv[])
{
    std::cout<<"Main process running..."<<std::endl;
    DWORD MainID = GetCurrentProcessId();
    std::cout<<"Main PID:"<<MainID<<std::endl;

    // 将进程ID转换为字符串
    std::ostringstream oss;
    oss << MainID;
    std::string processIdStr = oss.str();

    const char *producer = "producer.exe";
    const char *consumer = "consumer.exe";

    std::string producerCommadLine = std::string(producer) + " "+processIdStr;
    std::string consumerCommadLine = std::string(consumer) + " "+processIdStr;
    // 编译器不同
    /**
     * 对于mingw64，直接使用producer和consumer
     * 对于msvc，使用proPath和conPath
    */
    wchar_t* proPath=ConvertToWideString(producer);
    wchar_t* conPath=ConvertToWideString(consumer);

    STARTUPINFO producerStartupInfo={sizeof(STARTUPINFO)};
    PROCESS_INFORMATION producerProcessInfo;
    if (!CreateProcess(NULL, const_cast<char*>(producerCommadLine.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &producerStartupInfo, &producerProcessInfo))
    {
        std::cerr<<"Create producer process failed. Error code: "<< GetLastError()<<std::endl;
        return 1;
    }

    STARTUPINFO consumerStartupInfo={sizeof(STARTUPINFO)};
    PROCESS_INFORMATION consumerProcessInfo;
    if(!CreateProcess(NULL, const_cast<char*>(consumerCommadLine.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &consumerStartupInfo,&consumerProcessInfo))
    {
        std::cerr<<"Create consumer process failed. Error code: "<< GetLastError()<<std::endl;
        return 1;
    }

    WaitForSingleObject(consumerProcessInfo.hProcess, INFINITE);
    // WaitForSingleObject(producerProcessInfo.hProcess, INFINITE);
    TerminateProcess(producerProcessInfo.hProcess, 0);
    

    CloseHandle(producerProcessInfo.hProcess);
    CloseHandle(producerProcessInfo.hThread);
    CloseHandle(consumerProcessInfo.hProcess);
    CloseHandle(consumerProcessInfo.hThread);

    system("pause");

    return 0;
}
