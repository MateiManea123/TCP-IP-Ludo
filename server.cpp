#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <sstream>
#include <iomanip>
#define PORT 8080
#define MAX_CLIENTS 4
#define BUFFER_SIZE 1024
using namespace std;
int client_fds[MAX_CLIENTS]; // Array pentru socket-urile clienților
int client_count = 0;
int dice_roll = -1;
int current_client = 0; //primul jucator e 0
int current_dice_roll  = 0;
vector<vector<int>> player_pawns(4,vector<int>(4,-1)); //primul pion al primului jucator e player_pawns[0][0] = -1;
vector<int> starting_positions = {1, 11, 21, 31 };
vector<int> last_positions = {40, 50, 60, 70 };
vector<vector<int>> winning_position_pawns(4,vector<int>(4,-1));
bool game_started = false;
void handle_command(int fd, char* buffer);
int notified[MAX_CLIENTS] = {0};
int moved_pawn = 0;
int repeat = 0;
int move_to_next = 0;
int ejected_pawn_error = 0 ;
int moving_pawn_error = 0;
int ate_pawn  = -1;

int no_valid_moves()
{
    for (int i =0;i<4;i++)
    {
        if((player_pawns[current_client][i]!=-1 and player_pawns[current_client][i]<100) or winning_position_pawns[current_client][i]!=-1)
            return 0;
    }
    return 1;
}
void sendClient(int fd, const char *message) 
{
    
    if (write(fd, message, strlen(message)) < 0) {
        perror("[server] Eroare la trimiterea mesajului către client.");
    }
    
}
void readClient(int fd)
{
    char buffer[BUFFER_SIZE] = {0};
    memset(buffer,0,sizeof(buffer));
    if (read(fd, buffer, sizeof(buffer)) < 0) 
    {
        perror("[server] Eroare la trimiterea mesajului către client.");
    }
    
    else
    {
        handle_command(fd,buffer);
        std::cout<<buffer<<"\n";
    }
}
void handle_command(int fd, char* buffer)
{
    std::cout<<"handle_command:"<<buffer<<std::endl;
    if(!game_started && !strcmp(buffer,"start"))
    {
        game_started = true;
        std::cout<<"Game Started"<<std::endl;
    }
    if(!strcmp(buffer,"roll") and fd == client_fds[current_client])
    {
        srand(time(0));
        dice_roll = (rand() % 6) + 1;
        if(dice_roll==6)
            repeat = 1;
        cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! NO_VALID MOVES : "<<no_valid_moves<<endl;
        if(no_valid_moves() == 1  and dice_roll!=6)
        {
            move_to_next = 1;
        }
        current_dice_roll = dice_roll;
        cout<<"current_dice_roll : "<<current_dice_roll<<endl;
        std::cout<<"Rolled"<<std::endl;
    }
    if((!strcmp(buffer,"1")||!strcmp(buffer,"2")||!strcmp(buffer,"3")||!strcmp(buffer,"4")) and fd == client_fds[current_client])
    {
        ate_pawn = -1;
        moved_pawn = atoi(buffer);
        if((current_dice_roll!=6 and player_pawns[current_client][moved_pawn-1]==-1))
        {
            ejected_pawn_error = 1;
        }
        else
        {
            
            if(player_pawns[current_client][moved_pawn-1] == -1)
            {
                player_pawns[current_client][moved_pawn-1] = starting_positions[current_client];
            }
            else
                player_pawns[current_client][moved_pawn-1] +=current_dice_roll;
            if(player_pawns[current_client][moved_pawn-1] > last_positions[current_client])
            {
                if(player_pawns[current_client][moved_pawn-1] - last_positions[current_client] > 4 or winning_position_pawns[current_client][moved_pawn-1]!=-1)
                {
                    moving_pawn_error = 1;
                }
                else 
                {
                    winning_position_pawns[current_client][moved_pawn-1] = player_pawns[current_client][moved_pawn-1] - last_positions[current_client];
                    player_pawns[current_client][moved_pawn-1] = 100*(current_client+1)+winning_position_pawns[current_client][moved_pawn-1];
                }
            }
            for(int i = 0; i < client_count;i++)
            {
                for(int j = 0;j<4;j++)
                {
                    if((i!= current_client) and player_pawns[current_client][moved_pawn -1 ] == player_pawns[i][j])
                    {
                        player_pawns[i][j] = -1;
                        ate_pawn = i;
                    }
                        
                }
            }
            
            current_dice_roll = 0;
            std::cout<<"Moved Pawn"<<std::endl;
        }

        
    }
}

char* print_game_state(int turn) {
    ostringstream output;

    const string separator = "\n========================================";

    output << separator << "\n";
    output << "            Don't Worry Brother!          \n";
    output << separator << "\n";

    output << "It's the turn of Player " << turn + 1 << "!\n";
    output << separator << "\n";

    for (int i = 0; i < client_count; i++) {
        output << "+------------------------------------+\n";
        output << "| Player " << i + 1 << " Pawns:" << setw(22) << "|\n";
        output << "+------------------------------------+\n";

        for (int j = 0; j < 4; j++) {
            if (player_pawns[i][j] == -1) {
                output << "| Pawn " << j + 1 << ": " << "In Base" << setw(22) << "|\n";
            } else if(player_pawns[i][j]>100){
                output << "| Pawn " << j + 1 << ": "<< "On WINNING Position " << setw(12) << winning_position_pawns[i][j] << " |\n";
            }
            else
                output<< "| Pawn " << j + 1 << ": "<< "On Position " << setw(12) << player_pawns[i][j] << " |\n";
        }

        output << "+------------------------------------+\n";
    }

    output << separator << "\n";

    // Obținem textul generat ca string
    string result = output.str();

    // Alocăm un buffer pentru a stoca textul
    char* buffer = (char*)malloc(result.size() + 1);
    if (!buffer) {
        cerr << "Eroare la alocarea memoriei!\n";
        return nullptr;
    }

    // Copiem textul în buffer
    strcpy(buffer, result.c_str());

    return buffer; // Returnăm pointerul către buffer
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    fd_set readfds;

    // Creare socket server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[server] Eroare la creare socket.");
        exit(1);
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("[server] Eroare la setsockopt.");
        close(server_fd);
        exit(1);
    }
    // Configurare adresa server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[server] Eroare la bind.");
        close(server_fd);
        exit(1);
    }

    // Ascultare conexiuni
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("[server] Eroare la listen.");
        close(server_fd);
        exit(1);
    }

    std::cout << "[server] Server pornit. Ascultă pe portul " << PORT << "...\n";
    
    while (true) {
        dice_roll  = -1;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_fd = server_fd;

        for (int i = 0; i < client_count; i++) {
            FD_SET(client_fds[i], &readfds);
            if (client_fds[i] > max_fd) {
                max_fd = client_fds[i];
            }
        }

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("[server] Eroare la select.");
            continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            client_len = sizeof(client_addr);
            if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
                perror("[server] Eroare la accept.");
                continue;
            }

            if (client_count < MAX_CLIENTS) {
                client_fds[client_count++] = client_fd;
                std::cout << "[server] Client conectat. IP: " << inet_ntoa(client_addr.sin_addr) << "\n";

                const char *welcome_message = "Bun venit pe server! Așteptați mesaje de la server...\n";
                sendClient(client_fd, welcome_message);
            } else {
                const char *full_message = "Serverul este plin. Reîncercați mai târziu.\n";
                sendClient(client_fd, full_message);
                close(client_fd);
            }
        }
        else{
            for (int i = 0; i < client_count; i++) 
            {
                if (FD_ISSET(client_fds[i], &readfds))
                    readClient(client_fds[i]); 
            }
        }

        // Trimitem mesaje tuturor clienților
        if(!game_started)
        {
            for (int i = 0; i < client_count; i++) 
            {
                if (FD_ISSET(client_fds[i], &readfds))
                    sendClient(client_fds[i],i==0?"start":"Game not started");
            }
            sleep(2); // Pauză pentru demonstrație
        }
            
        else 
        {
            for (int i = 0; i < client_count; i++) 
            {
                std::cout<<"notified[i] "<<notified[i]<<std::endl;
                if(notified[i]==2 and i==current_client)
                { 
                        std::cout<<"dice_roll: "<<dice_roll<<std::endl;    
                        sendClient(client_fds[i],"choose");
                        notified[i]++;
                        
                }
                if (!notified[i]) 
                { 
                    if (i == current_client) 
                    {    
                            sendClient(client_fds[i], "roll"); 
                    } 
                    else 
                    {
                        sendClient(client_fds[i], "wait"); 
                    }
                    notified[i] = 1; 
                }
            }
            char* game_state = print_game_state(current_client);
            cout<<game_state;
            if (dice_roll != -1) 
            {
                if(move_to_next == 0 )
                {
                    for (int i = 0; i < client_count; i++) 
                    {
                        char temp[30000] = {0};
                        if (i == current_client) 
                        {
                            
                                    sprintf(temp, "You rolled value: %d  \n", dice_roll); 
                                    sendClient(client_fds[i], temp);
                                    notified[i] = 2;
                                
                        } 
                        else
                        { 
                                sprintf(temp, "Client %d rolled value %d \n", current_client, dice_roll);
                                sendClient(client_fds[i], temp);
                        }
                    }
                }
                else
                {
                    char temp[30000] = {0};
                    sprintf(temp, "You rolled value: %d! You can't move any piece!",dice_roll);
                    sendClient(client_fds[current_client], temp);
                    current_client = (current_client+1)%client_count;
                    move_to_next = 0;
                    memset(notified,0,sizeof(notified));
                }
                
            }
            
            if(moved_pawn!=0)
            {
                if(ejected_pawn_error == 0 and moving_pawn_error == 0)
                {
                    for (int i = 0; i < client_count; i++) 
                    {
                        char temp[30000] = {0};
                        char temp2[1000] = {0};
                        if (i == current_client) 
                        {
                            if(ate_pawn!=-1)
                            {
                                strcat(temp, game_state);
                                sprintf(temp2, "You moved pawn : %d , and ate %d's pawn\n", moved_pawn, ate_pawn);
                                strcat(temp,temp2);
                                
                            }
                            else
                            {
                                strcat(temp, game_state);
                                sprintf(temp2, "You moved pawn : %d  \n", moved_pawn);
                                strcat(temp,temp2); 
                            }
                            sendClient(client_fds[i], temp);
                            notified[i] = 3;
                        } 
                        else
                        { 
                            if(ate_pawn!=-1)
                            {
                                strcat(temp, game_state);
                                sprintf(temp2, "Client %d moved pawn: %d , and ate %d's pawn\n", current_client, moved_pawn,ate_pawn);
                                strcat(temp, temp2);
                            }
                            else
                            {
                                strcat(temp, game_state);
                                sprintf(temp2, "Client %d moved pawn: %d \n", current_client, moved_pawn);
                                strcat(temp, temp2);
                            }
                                sendClient(client_fds[i], temp);
                        }
                    }
                    if(repeat == 1)
                    {
                        moved_pawn = 0;
                        memset(notified,0,sizeof(notified));
                        repeat =0;
                    }
                    else
                    {
                        current_client = (current_client+1)%client_count;
                        moved_pawn = 0;
                        memset(notified,0,sizeof(notified));
                    }
                }
                else if(moving_pawn_error!=0)
                {
                    sendClient(client_fds[current_client], "You can't move the pawn because the position is occupied, or you rolled to much!\n Move another pawn!");
                    moving_pawn_error = 0;
                    notified[current_client] = 2;
                    moved_pawn = 0; 
                }
                else
                {
                    sendClient(client_fds[current_client], "You tried to take out a pawn without rolling 6! Move another pawn!");
                    ejected_pawn_error = 0;
                    notified[current_client] = 2;
                    moved_pawn = 0;
                }
                
                
            }

        }
    }
    close(server_fd);
    return 0;
}