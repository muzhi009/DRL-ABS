/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef NS3SOCKET_H
#define NS3SOCKET_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <nlohmann/json.hpp>
// Add a doxygen group for this module.
// If you have more than one file, this should be in only one of them.
/**
 * \defgroup NS3Socket Description of the NS3Socket
 */

namespace ns3
{
//Transmission data template
struct DRLstate {
    float a;
    float b;
    float c;
    float d;
    float reward;
    bool done;
};

class NS3Client{
public:
    NS3Client();    //Initialize classes with different parameters
    NS3Client(int port);
    NS3Client(const char* ipaddress, int port);
    void SendData(char* sendData); //Send data
    void SendData(DRLstate* sendData);
    float RecvData();   //Receive data
    void CloseClient();
private:
    int sock_client;
};

// Each class should be documented using Doxygen,
// and have an \ingroup NS3Socket directive

}

#endif /* NS3SOCKET_H */
