#pragma once
#include <vector>
#include <winsock2.h>

#include "client_connection.h"

#define SERVER_TICK_RATE 5

class mud_server
{
public:
    void run_server();

    explicit mud_server(PCSTR port)
        : port(port)
    {
    }

private:
    PCSTR port;
    
    SOCKET listen_socket = -1;
    std::vector<client_connection*> clients;
    u_int next_client_id = 10;
    int current_boss_health = 100;
    const int starting_boss_health = 100;
    int num_defeated = 0;
    
    void server_loop();
    void handle_connecting_clients();
    void handle_dead_clients();
    void handle_receiving_client_messages();
    void handle_tasks();
    void handle_command_msg(client_connection* from, std::string command);
    void handle_command_move(client_connection* from, std::string command);
    void handle_command_fish(client_connection* from);
    void handle_command_attack(client_connection* from);
    
    void handle_sending_messages();
};
