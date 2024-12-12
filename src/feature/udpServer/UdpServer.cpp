#include "UdpServer.h"

const char* UdpServer::Tag = "UdpServer";

UdpServer::UdpServer(int port,WifiServer wifiServer) : wifiServer(wifiServer), port(port), sock(-1) {}

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
            
        }
    }
}

void UdpServer::SendMessage(const char* message) {
    if (sock < 0 || wifiServer.getDroneIp().empty()) {
        ESP_LOGW(Tag, "Socket ou IP non valide !");
        return;
    }

    struct sockaddr_in destAddr = {};
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);
    inet_pton(AF_INET, wifiServer.getDroneIp().c_str(), &destAddr.sin_addr.s_addr);

    sendto(sock, message, strlen(message), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
    ESP_LOGI(Tag, "Message envoyé : %s", message);
}
