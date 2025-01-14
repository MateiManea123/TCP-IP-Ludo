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
vector<vector<int>> winning_position_pawns(4,vector<int>(5,-1));
bool game_started = false;
bool handle_command(int fd, char* buffer);
int notified[MAX_CLIENTS] = {0};
int moved_pawn = 0;
int repeat = 0;
int move_to_next = 0;
int ejected_pawn_error = 0 ;
int moving_pawn_error = 0;
int ate_pawn  = -1;
int victory = -1;
bool unknown_command = false;
bool quitting = false;
bool game_won = false;
int check_victory()
{
    for(int i = 1; i<=4;i++)
    {
        if(winning_position_pawns[current_client][i]==-1)
            return 0;
    }
    return 1;
}
int no_valid_moves(int dice_roll)
{
    for (int i = 0; i < 4; i++) 
    {
        int pawn_pos = player_pawns[current_client][i];
        
        if (pawn_pos == -1 && dice_roll == 6)
        {
            return 0;
        }


        if (pawn_pos >= 0 && pawn_pos < 100) 
        {
            int new_pos = pawn_pos + dice_roll;

            
            if (new_pos > last_positions[current_client]) 
            {
                int winning_pos = new_pos - last_positions[current_client];
                if (winning_pos <= 4 && winning_position_pawns[current_client][winning_pos] == -1) {
                    return 0; 
                }
            }
            else
                return 0;
            
        }
    }
    return 1; 
}
void sendClient(int fd, const char *message) 
{
    
    if (write(fd, message, strlen(message)) < 0) {
        perror("[server] Error sending message to client.");
    }
    
}
bool readClient(int fd)
{
    char buffer[BUFFER_SIZE] = {0};
    memset(buffer,0,sizeof(buffer));
    if (read(fd, buffer, sizeof(buffer)) < 0)
    {
        perror("[server] Error reading message from client.");
        return false;
    }

    else
    {
        std::cout<<buffer<<"\n";
        return handle_command(fd,buffer);
    }
}
bool handle_command(int fd, char* buffer)
{
    if (buffer[0] == 0)
    {
        std::cout<<"handle_command: empty buffer\n";
        return false;
    }
    unknown_command = true;
    if(!strcmp(buffer,"OK"))
    {
        unknown_command = false;
    }
    if(!strcmp(buffer,"quit"))
    {
        quitting = true;
        unknown_command = false;
    }
    std::cout<<"handle_command:"<<buffer<<std::endl;
    if(!game_started && !strcmp(buffer,"start"))
    {
        game_started = true;
        for(int i = 0 ; i< 4;i++)
            for(int j = 0;j<4;j++)
            {
                player_pawns[i][j] = -1;
                winning_position_pawns[i][j+1] = -1;
            }
                
        // std::cout<<"Game Started"<<std::endl;
        // player_pawns[0][0] = 101;
        // player_pawns[0][1] = 102;
        // player_pawns[0][2] = 103;
        // player_pawns[0][3] = 40;
        // winning_position_pawns[0][1] = 0;
        // winning_position_pawns[0][2] = 1;
        // winning_position_pawns[0][3] = 2;
        // cout<<"winning poses : \n";
        //     cout<<winning_position_pawns[current_client][4]<<"\n";
        unknown_command = false;
    }
    if(!strcmp(buffer,"roll") and fd == client_fds[current_client])
    {
        unknown_command = false;
        srand(time(0));
        dice_roll = (rand() % 6) + 1;
        if(dice_roll==6)
            repeat = 1;
        
        if(no_valid_moves(dice_roll) == 1)
        {
            move_to_next = 1;
        }
       

        current_dice_roll = dice_roll;
        std::cout<<"Rolled"<<std::endl;
        
    }
    if((!strcmp(buffer,"1")||!strcmp(buffer,"2")||!strcmp(buffer,"3")||!strcmp(buffer,"4")) and fd == client_fds[current_client])
    {
        unknown_command = false;
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
                int win_pos = player_pawns[current_client][moved_pawn-1] - last_positions[current_client];
                if(win_pos > 4 or winning_position_pawns[current_client][win_pos]!=-1)
                {
                    moving_pawn_error = 1;
                }
                else 
                {
                    winning_position_pawns[current_client][win_pos] = moved_pawn-1;
                    player_pawns[current_client][moved_pawn-1] = 100*(current_client+1)+win_pos;
                    if(check_victory() == 1)
                        victory = 1;
                    
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
    return true;
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
                output << "| Pawn " << j + 1 << ": "<< "On WINNING Position " << setw(12) << player_pawns[i][j]%10 << " |\n";
            }
            else
                output<< "| Pawn " << j + 1 << ": "<< "On Position " << setw(12) << player_pawns[i][j]%41 << " |\n";
        }

        output << "+------------------------------------+\n";
    }

    output << separator << "\n";

    
    string result = output.str();

    
    char* buffer = (char*)malloc(result.size() + 1);
    if (!buffer) {
        cerr << "Error at memory allocation!\n";
        return nullptr;
    }

    
    strcpy(buffer, result.c_str());

    return buffer; 
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    fd_set readfds;


    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[server] Error at creating socket.");
        exit(1);
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("[server] Error at setsockopt.");
        close(server_fd);
        exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[server] Error at bind.");
        close(server_fd);
        exit(1);
    }


    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("[server] Error at listen.");
        close(server_fd);
        exit(1);
    }

    std::cout << "[server] Server started. Listening to port : " << PORT << "...\n";
    int old_current_client = 0;
    while (true) {
        bool work_message_received = false;
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
            perror("[server] Error at select.");
            continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            client_len = sizeof(client_addr);
            if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
                perror("[server] Error at accept.");
                continue;
            }

            if (client_count < MAX_CLIENTS) {
                client_fds[client_count++] = client_fd;
                std::cout << "[server] Client connected. IP: " << inet_ntoa(client_addr.sin_addr) << "\n";

                const char *welcome_message = "Welcome to Nu te supara frate! Wait for messages from the server...\n";
                sendClient(client_fd, welcome_message);
                if(client_count == 1)
                {
                    sendClient(client_fds[0], "Wait for at least one more person to join!");
                }
                if (client_count == 2)
                {
                    sendClient(client_fds[0],"start");
                }
            } else {
                const char *full_message = "The server is full. Try again later.\n";
                sendClient(client_fd, full_message);
                close(client_fd);
            }
        }
        else{
            unknown_command = false;
            for (int i = 0; i < client_count; i++)
            {
                if (FD_ISSET(client_fds[i], &readfds))
                {
                    if (i == current_client)
                    {
                        work_message_received = true;
                    }
                    if (readClient(client_fds[i]) == false)
                    {
                        work_message_received = true;
                        quitting = true;
                        break;
                    }

                }
            }
            if(old_current_client!=current_client)
                work_message_received = true;
            if(game_won)
            {
                if (client_count == 2)
                {
                    sendClient(client_fds[0],"start");
                    game_won =false;
                }
            }
        }
        
        
            
        if (work_message_received && game_started) 
        {
            old_current_client = current_client;
            if (unknown_command)
            {
                sendClient(client_fds[current_client], "unknown");

                continue;
            }
            else if (quitting)
            {
                for (int i = 0; i < client_count; i++)
                {
                    sendClient(client_fds[i], "quit");
                }
                break;
            }
            for (int i = 0; i < client_count; i++) 
            {
                if(notified[i]==2 and i==current_client)
                { 
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
                        char temp[200] = {0};
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
                    char temp[200] = {0};
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
                        if(victory == 1)
                        {
                            if(i == current_client)
                            {
                                strcat(temp, game_state);
                                sprintf(temp2, "You Won!!!");
                                strcat(temp,temp2);
                            }
                            else
                            {
                                strcat(temp, game_state);
                                sprintf(temp2, "Player %d Won!!!",current_client);
                                strcat(temp,temp2);
                            }
                            sendClient(client_fds[i], temp);
                            game_started = false;
                            game_won = true;
                        }
                        else
                        {
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
                        
                    }
                    victory = 0;
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
            sleep(1);
        } //else
        
    } //while
    close(server_fd);
    return 0;
}