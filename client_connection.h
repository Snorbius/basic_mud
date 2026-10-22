#pragma once
#include <queue>
#include <string>
#include <winsock2.h>

#include "utilities.h"

class client_connection
{
public:
    client_connection(u_int client_id, const std::string& name, SOCKET client_socket)
        : client_id(client_id),
          name(name),
          client_socket(client_socket)
    {
        add_to_send_queue("init " + std::to_string(client_id) + " " + name);
    }

    u_int client_id;
    std::string name;
    SOCKET client_socket;
    std::queue<std::string> task_queue;
    std::queue<std::string> send_queue;
    bool queued_for_deletion = false;
    ROOMS current_room = SPAWN_ROOM;
    int current_hp = 100;
    const int MAX_HP = 100;
    
    void receive_message();
    void add_to_send_queue(std::string message);
    void heal(int amount);
    void send_to_client();
};
