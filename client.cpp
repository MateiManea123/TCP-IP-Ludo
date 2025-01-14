#include <sys/types.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cstdlib>
using namespace std;
#define BUFFER_SIZE 2048

int main(int argc, char *argv[]) {
    int sd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if (argc != 3) {
        std::cerr << "Syntax: " << argv[0] << " <server_adress> <port>\n";
        exit(1);
    }

    
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[client] Error when creating socket.");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[client] Error when trying to connect.");
        close(sd);
        exit(1);
    }

    std::cout << "[client] Connected to the server. Waiting for messages...\n";

    while (true) {
        
        memset(buffer, 0, sizeof(buffer));

        int bytes = read(sd, buffer, sizeof(buffer) - 1);

        if (bytes <= 0) {
            std::cerr << "[client] The connexion to the server has been lost.\n";
            break;
        }
        std::cout << "[client] Message from server: " << buffer << "\n";
        if (!strcmp(buffer, "quit")) {
            printf("[client] Disconnecting...\n");
            break;
        }
        if(!strcmp(buffer, "start") or !strcmp(buffer, "roll") or !strcmp(buffer,"choose")or !strcmp(buffer,"unknown"))
        {
            if(!strcmp(buffer, "start"))
                cout<<"[client] You can start the game when you want! [start]"<<endl;
            if(!strcmp(buffer,"choose"))
                cout<<"[client] Choose a pawn to move! [1,2,3,4]"<<endl;
            if(!strcmp(buffer, "roll"))
                cout<<"[client] Roll the dice! [roll]"<<endl;
            if(!strcmp(buffer, "unknown"))
                cout<<"[client] unknown command"<<endl;

            char buffer_send[BUFFER_SIZE] = {0};

            read(0, buffer_send, sizeof(buffer));

            cout<<"[client] Message sent to server: "<< buffer_send<<"\n";

            buffer_send[strcspn(buffer_send, "\n")] = 0;

            write(sd, buffer_send, strlen(buffer_send));

        }
        else
        {
            char buffer_send[bytes] = "OK";

            write(sd, buffer_send, sizeof(buffer_send));
        }

    }

    close(sd);
    return 0;
}
