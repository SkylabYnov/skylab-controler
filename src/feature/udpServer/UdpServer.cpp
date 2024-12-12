#include "UdpServer.h"

const char* UdpServer::Tag = "UdpServer";

UdpServer::UdpServer(int port) : port(port), sock(-1), droneIp("") {}

UdpServer::~UdpServer() {
    if (sock >= 0) {
        close(sock);
    }
}

void UdpServer::Init() {
    struct sockaddr_in serverAddr;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(Tag, "Erreur de création du socket : errno %d", errno);
        return;
    }

    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        ESP_LOGE(Tag, "Erreur lors du bind : errno %d", errno);
        close(sock);
        sock = -1;
        return;
    }

    ESP_LOGI(Tag, "Serveur UDP prêt sur le port %d", port);
}

void UdpServer::ReceiveTask() {
    char rxBuffer[128];
    struct sockaddr_in sourceAddr;
    socklen_t sockLen = sizeof(sourceAddr);

    while (true) {
        int len = recvfrom(sock, rxBuffer, sizeof(rxBuffer) - 1, 0, (struct sockaddr *)&sourceAddr, &sockLen);
        if (len < 0) {
            ESP_LOGE(Tag, "Erreur de réception : errno %d", errno);
            break;
        } else {
            rxBuffer[len] = '\0';
            ESP_LOGI(Tag, "Message reçu : %s", rxBuffer);

            if (strcmp(rxBuffer, "hello world !") == 0) {
                char addrStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &sourceAddr.sin_addr, addrStr, INET_ADDRSTRLEN);
                droneIp = std::string(addrStr);
                ESP_LOGI(Tag, "IP Drone enregistrée : %s", droneIp.c_str());
            }
        }
    }
}

void UdpServer::SendMessage(const std::string& message, const std::string& ip) {
    if (sock < 0 || ip.empty()) {
        ESP_LOGW(Tag, "Socket ou IP non valide !");
        return;
    }

    struct sockaddr_in destAddr = {};
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &destAddr.sin_addr.s_addr);

    sendto(sock, message.c_str(), message.length(), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
    ESP_LOGI(Tag, "Message envoyé : %s", message.c_str());
}
