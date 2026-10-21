#include "mud_client.h"

#include <format>
#include <iostream>
#include <ppltasks.h>
#include <thread>

#include "utilities.h"
#include <winsock2.h>
#include <ws2tcpip.h>

using namespace utilities;

void mud_client::run_client()
{
    // Imaginary setup...
    client_loop();
}

void mud_client::client_loop()
{
    addrinfo settings;
    ZeroMemory(&settings, sizeof(settings));
    settings.ai_family = AF_INET; // Ipv4
    settings.ai_socktype = SOCK_STREAM; // TCP
    settings.ai_protocol = IPPROTO_TCP; // TCP
    
    addrinfo* result;
    int success = getaddrinfo(IP_ADDRESS, DEFAULT_PORT, &settings, &result);
    if (success != 0)
    {
        print_line("Could not successfully get address info, error code: " + std::to_string(success));
        print_line("Exiting...");
        return;
    }
    
    connect_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    
    success = connect(connect_socket, result->ai_addr, static_cast<int>(result->ai_addrlen));
    if (success != 0)
    {
        print_line("Could not connect to socket, error code: " + std::to_string(WSAGetLastError()));
        print_line("Exiting...");
        return;
    }
    
    u_long mode = 1;
    ioctlsocket(connect_socket, FIONBIO, &mode);
    
    print_line("Successfully connected to server!");
    
    std::thread receiving_thread(&mud_client::thread_func, this);
    
    command_help();
    std::string input;
    while (running)
    {
        std::getline(std::cin, input);
        input = string_to_lower_case(input);
        
        size_t space = input.find(' ');
        std::string command = space == std::string::npos ? input : input.substr(0, space);
        
        if (command == "exit")      { running = false; continue; }
        if (command == "help")      { command_help(); continue; }
        if (command == "msg")       { command_msg(input); continue; }
        if (command == "players")   { command_players(); continue; }
        if (command == "move")      { command_move(input); continue; }
        if (command == "fish")      { command_fish(); continue; }
        
        print_line("That doesn't seem like a proper command, type 'help' to show what you can do");
    }
    
    receiving_thread.join();
    
    closesocket(connect_socket);
}

void mud_client::command_help()
{
    print_line("");
    print_line("Available commands:");
    print_line("[HELP] - display commands");
    print_line("[PLAYERS] - list all players currently connected to the server");
    print_line("[MSG message] - send a message to everyone on the server");
    print_line("[MOVE up/down] - move in a certain direction");
    print_line("[FISH] - lets you fish to recover some health, assuming you are at the fishing spot");
    print_line("[EXIT] - exit the game");
    print_line("");
}

void mud_client::command_msg(std::string input)
{
    if (input.size() > MAX_MESSAGE_LENGTH)
    {
        print_line("I'm sorry but that message was too long.\n"
                   "Please make it shorter next time :) (max 64 characters)");
        return;
    }
    
    thread_mutex.lock();
    send(connect_socket, input.data(), static_cast<int>(input.size()), 0);
    thread_mutex.unlock();
    
    print_debug_line("Sent message with contents: \"" + input + "\"");
}

void mud_client::command_players()
{
    print_line("These are the currently connected players:");
    for (const std::pair<const unsigned, std::string>& player : connected_players)
    {
        print_line("\t" + player.second + 
            ", id:" + std::to_string(player.first) + 
            (player.first == client_id ? " (YOU)" : ""));
    }
}

void mud_client::command_move(std::string input)
{
    int current_room_id = current_room;
    if (input.find("up") != std::string::npos)
        current_room_id--;
    if (input.find("down") != std::string::npos)
        current_room_id++;
    
    if (current_room_id < 0 || current_room_id > 2 || current_room_id == current_room)
    {
        print_line("That is not a valid direction to move in.");
        return;
    }
    
    std::string message = std::format("move {}", current_room_id);
    thread_mutex.lock();
    send(connect_socket, message.data(), static_cast<int>(message.size()), 0);
    thread_mutex.unlock();
}

void mud_client::command_fish()
{
    if (current_room != FISH_SPOT)
    {
        print_line("You can only fish in the fishing spot.");
        return;
    }
    
    print_line("You attempt to fish...");
    std::string message = std::format("fish");

    thread_mutex.lock();
    send(connect_socket, message.data(), static_cast<int>(message.length()), 0);
    thread_mutex.unlock();
}

void mud_client::handle_tasks()
{
    thread_mutex.lock();

    while (!task_queue.empty())
    {
        std::string task = task_queue.front();
        task_queue.pop();
        
        size_t space = task.find(' ');
        std::string command = space == std::string::npos ? task : task.substr(0, space);
        
        std::string input = task.substr(space + 1);
        
        if (command == "init")  { handle_init_command(input); continue; }
        if (command == "new")   { handle_new_command(input); continue; }
        if (command == "msg")   { handle_msg_command(input); continue; }
        if (command == "kill")  { handle_kill_command(input); continue; }
        if (command == "move")  { handle_move_command(input); continue; }
        if (command == "heal")  { handle_heal_command(input); continue; }
        
        print_debug_line(std::format("{}; unrecognizable task", task));
    }
    
    thread_mutex.unlock();
}

void mud_client::handle_init_command(std::string input)
{
    size_t space = input.find(' ');
    std::string string_id = input.substr(0, space);
    std::string name = input.substr(space + 1);
    u_int id = std::stoi(string_id);
    client_id = id;
    connected_players[id] = name;
    
    print_line("The server has given you the privilege to have a name.\n"
               "You shall henceforth be called \"" + name + "\" with an id of " + std::to_string(id) + ".");
}

void mud_client::handle_new_command(std::string input)
{
    size_t space = input.find(' ');
    std::string string_id = input.substr(0, space);
    std::string name = input.substr(space + 1);
    u_int id = std::stoi(string_id);
    connected_players[id] = name;
    
    print_line("A new player by the name of " + name + " just connected! Remember to say hi!");
}

void mud_client::handle_msg_command(std::string input)
{
    if (!input.empty())
        print_line(input);
}

void mud_client::handle_kill_command(std::string input)
{
    u_int id_to_kill = std::stoi(input);
    
    if (!connected_players.contains(id_to_kill)) // Do nothing if this player doesn't exist
        return;
    
    print_line(std::format("Player {}({}) has disconnected!", connected_players[id_to_kill], id_to_kill));
    connected_players.erase(id_to_kill);
}

void mud_client::handle_move_command(std::string input)
{
    current_room = static_cast<ROOMS>(stoi(input));
    print_debug_line(std::format("You have moved to room {}", static_cast<int>(current_room)));

    switch (current_room)
    {
    case BOSS_ROOM:
        print_line("You have entered the boss room. Type 'attack' to attack the boss!");
        print_line("The spawn room is below you.");
        break;
    case SPAWN_ROOM:
        print_line("You have entered the spawn room, there is nothing of note here.");
        print_line("Above you is the boss room and below you is the fishing spot.");
        break;
    case FISH_SPOT:
        print_line("You have entered the fishing spot, type 'fish' to reel in a fish and recover some health.");
        print_line("Above you is the spawn room.");
        break;
    }
}

void mud_client::handle_heal_command(std::string input)
{
    int amount = std::stoi(input);
    current_hp = min(current_hp + amount, MAX_HP);
    if (amount == 0)
        print_line("Drat! You fish up some garbage.");
    if (amount > 0)
    {
        print_line(std::format("You get a succulent little fish, you are healed for {}", amount));
        print_line(std::format("Your new health sits at {}", current_hp));
    }
    if (amount < 0)
    {
        print_line(std::format("Youch!! You are hit for {} damage!", abs(amount)));
        print_line(std::format("Your new health sits at {}", current_hp));
    }
}

void mud_client::handle_receive_messages()
{
    char buffer[1024];
    int bytes_received = recv(connect_socket, buffer, sizeof(buffer), 0);
        
    if (bytes_received == SOCKET_ERROR)
    {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) // This is fine, just means the server has not sent anything
            print_line("Failed to receive message from server, error code: " + std::to_string(error));
    }
    else
    {
        std::string message(buffer);
        message = message.substr(0, bytes_received);
            
        print_debug_line("Received message from server: \"" + message + "\"");
            
        thread_mutex.lock();
        while (!message.empty())
        {
            std::string task = message.substr(0, message.find(';'));
            task_queue.push(task);
            message = message.substr(message.find(';') + 1);
        }
        thread_mutex.unlock();
    }
}

void mud_client::thread_func()
{
    while (running)
    {
        handle_receive_messages();
        
        handle_tasks();
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
