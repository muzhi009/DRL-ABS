/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3socket.h"

namespace ns3
{
NS3Client::NS3Client(){
    sock_client = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    connect(sock_client, (sockaddr*)&server_addr, sizeof(sockaddr));
}
NS3Client::NS3Client(int port){
    sock_client = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    connect(sock_client, (sockaddr*)&server_addr, sizeof(sockaddr));
}

NS3Client::NS3Client(const char* ipaddress,int port){
    sock_client = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = inet_addr(ipaddress);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    connect(sock_client, (sockaddr*)&server_addr, sizeof(sockaddr));
}
void 
NS3Client::SendData(char* sendData){
    send(sock_client, sendData, strlen(sendData) + 1, 0);
}
void 
NS3Client::SendData(DRLstate* sendData){
    nlohmann::json json_data = {    //Convert data to JSON format and send it
        {"a", sendData->a},
        {"b", sendData->b},
        {"c", sendData->c},
        {"d", sendData->d},
        {"reward", sendData->reward},
        {"done", sendData->done}
    };
    std::string serialized_data = json_data.dump();
    send(sock_client, serialized_data.c_str(), serialized_data.length(), 0);
}

float 
NS3Client::RecvData(){
    char recv_info[50];
    recv(sock_client, recv_info, sizeof(recv_info), 0);
    float received_action = std::stof(recv_info);
    return received_action;
}

void 
NS3Client::CloseClient(){
    close(sock_client);
}

}


