#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <string>
#include <lwip/sockets.h>
#include <esp_log.h>
#include <feature/wifiServer/WifiServer.h>

class UdpServer {
public:
    UdpServer(int port,WifiServer wifiServer);
    ~UdpServer();
    void Init();
    void ReceiveTask();
    void SendMessage(const std::string& message);

private:
    WifiServer wifiServer;
    int port;
    int sock;
    static const char *Tag;
};

#endif // UDP_SERVER_H
