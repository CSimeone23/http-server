#include <winsock2.h>
#include <stdio.h>
#include <string.h>

#define RECV_ERROR 1
#define SEND_ERROR 2
#define DEFAULT_BUFFER_SIZE 1028

struct HttpRequest{
    char *method;
    char *url;
};

int WSAStartupHelper(){ //DONE
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
        return 0;
    }
}

SOCKET socketHelper(){ //DONE
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
    }
    return sock;
}

int bindHelper(SOCKET sock){ //DONE
    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8080);
    int binder = bind(sock, (SOCKADDR *) &addr, sizeof(addr));
    if(binder == SOCKET_ERROR){
        printf("ERROR BINDING SOCKET TO ADDRESS\nERROR CODE: %d", WSAGetLastError());
        return 1;
    } else {
        return 0;
    }
}

int listenHelper(SOCKET sock){ //DONE
    if(listen(sock, SOMAXCONN) != 0){
        printf("ERROR LISTENING\nERROR CODE: %d", WSAGetLastError());
        return 1;
    } else {
        return 0;
    }
}


int handleGet(SOCKET *clientSocket, char *recvbuf, struct HttpRequest *newRequest){
    char *response = malloc(1000), body[300];
    int contentLength = 0;
    if(strcmp(newRequest->url, "/") == 0){
        FILE *fp = fopen("index.html", "r"); //fopen opens a file... fp points to the file being opened
        char c;
        int index = 0;
        while( (c = getc(fp)) != EOF ){
            *(body + index) = c;
            index++;
        }
        fclose(fp);
        sprintf(response, "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s\n", strlen(body), body);
        printf("response = %s", response);
    } else {
        response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
    }
    if( send(*clientSocket, response, strlen(response), 0) == SOCKET_ERROR){
        printf("Send failed: %d\n", WSAGetLastError());
        closesocket(*clientSocket);
        return SEND_ERROR;
    }
    return 0;
}

int handlePost(SOCKET *clientSocket){
    int iSendResult;
    char const *response = "HTTP/1.1 200 OK\nConnection: keep-alive\nContent-Type: text/html\nContent-Length: 0\r\n\r\n";
    iSendResult = send(*clientSocket, response, strlen(response), 0);
    if(iSendResult == SOCKET_ERROR){
        printf("Send failed: %d\n", WSAGetLastError());
        closesocket(*clientSocket);
        return SEND_ERROR;
    }
    printf("Sent response: %s\n", response);
    return 0;
}

int handleUnkownMethod(SOCKET *clientSocket){
    int iSendResult;
    char const *response = "HTTP/1.1 405 Method Not Allowed\nConnection: close\r\n\r\n";
    iSendResult = send(*clientSocket, response, strlen(response), 0);
    if(iSendResult == SOCKET_ERROR){
        printf("Send failed: %d\n", WSAGetLastError());
        closesocket(*clientSocket);
        return SEND_ERROR;
    }
    printf("Sent response: %s\n", response);
    return 0;
}

int handleMethod(struct HttpRequest *newRequest, SOCKET *clientSocket, char *recvbuf){
    // only going to handle GET and POSTs
    // the other methods are just going to get a crisp 403 sent back to them
    if(strcmp(newRequest->method, "GET") == 0){
        printf("Handling get request\n");
        if(handleGet(clientSocket, recvbuf, newRequest) == SEND_ERROR){
            return SEND_ERROR;
        }
    } else if(strcmp(newRequest->method, "POST") == 0){
        printf("Handling post requet\n");
        if(handlePost(clientSocket) == SEND_ERROR){
            return SEND_ERROR;
        }
    } else {
        if(handleUnkownMethod(clientSocket) == SEND_ERROR){
            return SEND_ERROR;
        }
    }
    return 0;
}

void parseHttpRequest(struct HttpRequest *newRequest, char *recvbuf){
    int mStringLength = 0;
    while(*(recvbuf + mStringLength*sizeof(char)) != 32){ // recvbuf == [ADDRESS].... recvbuf + mStringLength*1 = next address location... use * to get value @ address
        mStringLength++;
    }
    newRequest->method = calloc(mStringLength, sizeof(char)); //calloc initializes array with 0's. Malloc initializes array with garbage values
    for(int i=0; i<mStringLength; i++){
        *(newRequest->method+i*sizeof(char)) = *(recvbuf + i*sizeof(char)); //Kinda gross looking but basically method[i] = recvbuf[i];
    }
    //Done getting method
    //Now going to get URL being requested
    int stoppingPoint = mStringLength+1;
    while(*(recvbuf + stoppingPoint*sizeof(char)) != 32){
        stoppingPoint++;
    }
    int urlLength = stoppingPoint - mStringLength - 1;
    newRequest->url = calloc(urlLength, sizeof(char));
    for(int i=0; i<urlLength; i++){
        *(newRequest->url+i*sizeof(char)) = *(recvbuf+(i+mStringLength+1)); //This is so nasty looking but url[i] = recvuf[i+mStringLength+1]
    }
    //Done getting url
    printf("CLIENT MESSAGE:\r\n%s\n", recvbuf);
}

int communicationHelper(SOCKET clientSocket){
    //Handles the communication between the client and server
    int iResult, iSendResult, recvbuflen = DEFAULT_BUFFER_SIZE;
    char recvbuf[recvbuflen];
    do{
        iResult = recv(clientSocket, recvbuf, recvbuflen, 0);
        if(iResult > 0){
            struct HttpRequest newRequest;
            parseHttpRequest(&newRequest, recvbuf);
            if(handleMethod(&newRequest, &clientSocket, recvbuf) == SEND_ERROR){
                printf("Response Send Error!\n");
                closesocket(clientSocket);
                return SEND_ERROR;
            }
        } else if(iResult == 0){
            printf("Connection closing...\n");
            return 0;
        } else {
            printf("Recv failed: %d\n", WSAGetLastError());
            closesocket(clientSocket);
            return RECV_ERROR;
        }
    } while(iResult > 0);
}

int main(){
    //start WSA up
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

    //creating a socket for the client to connect
    SOCKET clientSocket;
    printf("Server Start-up Successful!\nWaiting for the client to connect...\n");
    //accept
    clientSocket = accept(sock, NULL, NULL);
    if(clientSocket == INVALID_SOCKET){
        printf("Accept Failed with Error; %ld\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    printf("Client socket [%d] connected!\n", clientSocket);
    // 3 way handshake
    // send and recv loop
    switch(communicationHelper(clientSocket)){
        case (RECV_ERROR || SEND_ERROR):
                closesocket(sock);
                WSACleanup();
                return 1;
        default:
                break;
    }
    

    //close socket
    closesocket(sock);
    WSACleanup();
    printf("Connection closed\n");
    return 0;
}