#include <winsock2.h>
#include <stdio.h>


int WSAStartupHelper(){
    // need to do this to setup socket
    /*
        MAKEWORD(2, 2) = 2.2???? 2.2 is the windows version that we're using
        wsaData is a struct... not sure what it does tbh
    */
    WSADATA wsaData;
    int errCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(errCode != 0){
        printf("WSAStartup failed with the error %d\n", errCode);
        return 1;
    } else {
        printf("WSAStartup Successful!\n");
        return 0;
    }
}

SOCKET socketHelper(){
    // create a socket
    /*
        AF_INET means we're connecting to an IPv4 address
        SOCK_STREAM means we're using TCP
        IPPROTO_TCP means we're using TCP w/ AF_INET or AF_INET6 (IPv6)
    */
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock == INVALID_SOCKET){
        printf("ERROR CREATING SOCKET\n\tERROR CODE: %d", WSAGetLastError());
    } else {
        printf("Socket Creation Successful!\n");
    }
    return sock;
}

int bindHelper(SOCKET sock){
    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8080);
    int binder = bind(sock, (SOCKADDR *) &addr, sizeof(addr));
    if(binder == SOCKET_ERROR){
        printf("ERROR BINDING SOCKET TO ADDRESS\nERROR CODE: %d", WSAGetLastError());
        return 1;
    } else {
        printf("Address Binding Successful!\n");
        return 0;
    }
}

int listenHelper(SOCKET sock){
    if(listen(sock, SOMAXCONN) != 0){
        printf("ERROR LISTENING\nERROR CODE: %d", WSAGetLastError());
        return 1;
    } else {
        printf("Listening Successful!");
        return 0;
    }
}

int main(){
    if(WSAStartupHelper() == 1){
        return 1;
    }
    //create a new socket
    SOCKET sock = socketHelper();
    if(sock == INVALID_SOCKET){
        WSACleanup();
        return 1;
    }
    //bind
    if(bindHelper(sock) == 1){
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    //listen
    if(listenHelper(sock) == 1){
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    //accept
        // 3 way handshake
        // send and recv loop
    //close socket
    closesocket(sock);
    WSACleanup();
    return 0;
}