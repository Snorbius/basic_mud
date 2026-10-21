#pragma once
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <winsock2.h>

#include "utilities.h"

class mud_client
{
public:
    void run_client();
    void client_loop();
    void thread_func();
    
private:
    SOCKET connect_socket = -1;
    std::queue<std::string> task_queue;
    bool running = true;
    std::mutex thread_mutex;
    u_int client_id = -1;
    std::map<u_int, std::string> connected_players;
    ROOMS current_room = SPAWN_ROOM;
    int current_hp = 100;
    const int MAX_HP = 100;
    
    
    // client side commands
    static void command_help();
    void command_msg(std::string input);
    void command_players();
    void command_move(std::string input);
    void command_fish();
    void command_attack();

    // handle tasks sent from server
    void handle_tasks();
    void handle_init_command(std::string input);
    void handle_new_command(std::string input);
    void handle_msg_command(std::string input);
    void handle_kill_command(std::string input);
    void handle_move_command(std::string input);
    void handle_heal_command(std::string input);
    void handle_receive_messages();
};
