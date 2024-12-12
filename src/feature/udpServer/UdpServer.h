#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <string>
#include <lwip/sockets.h>
#include <esp_log.h>

class UdpServer {
public:
    UdpServer(int port);
    ~UdpServer();
    void Init();
    void ReceiveTask();
    void SendMessage(const std::string& message, const std::string& ip);

private:
    int port;
    int sock;
    std::string droneIp;
    static const char *Tag;
};

#endif // UDP_SERVER_H
