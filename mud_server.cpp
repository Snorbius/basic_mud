#include "mud_server.h"

#include <format>
#include <iostream>
#include <thread>

#include "utilities.h"

#include <winsock2.h>
#include <ws2tcpip.h>

using namespace utilities;

void mud_server::run_server()
{
    // This is likely where any necessary setup will be conducted
    server_loop();

    for (client_connection* client : clients)
        delete client;
}

void mud_server::server_loop()
{
    addrinfo settings;
    ZeroMemory(&settings, sizeof(settings));
    settings.ai_family = AF_INET; // ipv4
    settings.ai_socktype = SOCK_STREAM; // TCP
    settings.ai_protocol = IPPROTO_TCP; // TCP again??
    settings.ai_flags = AI_PASSIVE; // Use socket address when binding 
    
    addrinfo* result;
    int success = getaddrinfo(nullptr, port, &settings, &result);
    if (success != 0)
    {
        print_line("Could not successfully get address info, error code: " + std::to_string(success));
        print_line("Exiting...");
        return;
    }
    
    listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    
    u_long mode = 1; // non-blocking, 0 = blocking
    ioctlsocket(listen_socket, FIONBIO, &mode);
    
    success = bind(listen_socket, result->ai_addr, static_cast<int>(result->ai_addrlen));
    if (success == SOCKET_ERROR)
    {
        print_line("Could not successfully bind socket, error code: " + std::to_string(WSAGetLastError()));
        print_line("Exiting...");
        return;
    }
    
    listen(listen_socket, 16);
    
    print_line("Server set up!");
    print_line("This is a dedicated server, you will have to launch an extra terminal to join yourself.");
    print_line("Type 'exit' to stop the server.");
    
    bool running = true;
    
    std::thread server_thread([this, &running]
    {
        constexpr int sleep_time = static_cast<int>((1.0f / SERVER_TICK_RATE) * 1000.0f);
        while (running)
        {
            handle_connecting_clients();
            
            handle_dead_clients();
            
            handle_receiving_client_messages();
            
            handle_tasks();
            
            handle_sending_messages();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }
    });

    std::string input;
    while (running)
    {
        std::getline(std::cin, input);
        input = string_to_lower_case(input);
        
        if (input == "exit") { running = false; continue; }
        
        print_line(std::to_string(input[0]));
    }
    
    print_debug_line("joining thread...");
    server_thread.join();

    print_debug_line("telling clients to suicide...");
    for (client_connection* client : clients)
    {
        client->add_to_send_queue("dead");
        client->send_to_client();
    }
    
    print_debug_line("sleeping for a tiny bit...");
    // Sleep for a bit to make sure all messages are sent before clearing memory
    std::this_thread::sleep_for(std::chrono::seconds(3));

    print_debug_line("freeing address info...");
    freeaddrinfo(result);
    
    print_debug_line("Done! See you again later!");
}

void mud_server::handle_connecting_clients()
{
    SOCKET client_socket = accept(listen_socket, nullptr, nullptr);
    if (client_socket == INVALID_SOCKET)
    {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) // WSAEWOULDBLOCK just means there was no data, fine to keep going
            print_line("Could not accept client connection, error code: " + std::to_string(error));
        return;
    }
    
    client_connection* new_client = new client_connection(next_client_id++, "placeholder", client_socket);

    // Tell everyone else that a new kid has joined plus have the new kid catch up on who's here
    for (client_connection* client : clients)
    {
        client->add_to_send_queue("new " + std::to_string(new_client->client_id) + " " + new_client->name);
        new_client->add_to_send_queue("new " + std::to_string(client->client_id) + " " + client->name);
    }
    
    clients.push_back(new_client);

    print_line("New client connected!!");
}

void mud_server::handle_dead_clients()
{
    std::vector<int> clients_to_kill;

    for (size_t i = 0; i < clients.size(); ++i)
        if (clients[i]->queued_for_deletion)
            clients_to_kill.emplace(clients_to_kill.begin(), i);

    for (int index : clients_to_kill)
    {
        client_connection* client_to_kill = clients[index];
        u_int client_id = client_to_kill->client_id;
        print_debug_line(std::format("Client with id {} getting killed", client_id));
        client_to_kill->add_to_send_queue("dead");
        client_to_kill->send_to_client();
        clients.erase(clients.begin() + index);
        for (client_connection* client : clients)
            client->add_to_send_queue(std::format("kill {}", client_id));
        
        delete client_to_kill;
    }
}

void mud_server::handle_receiving_client_messages()
{
    for (client_connection* client : clients)
        client->receive_message();
}

void mud_server::handle_tasks()
{
    for (client_connection* client : clients)
    {
        while (!client->task_queue.empty())
        {
            std::string message = client->task_queue.front();
            client->task_queue.pop();
            
            size_t space = message.find(' ');
            std::string command = space == std::string::npos ? message : message.substr(0, space);
            
            if (command == "msg")       { handle_command_msg(client, message); continue; }
            if (command == "move")      { handle_command_move(client, message); continue; }
            if (command == "fish")      { handle_command_fish(client); continue; }
            if (command == "attack")    { handle_command_attack(client); continue; }
        }
    }
}

void mud_server::handle_command_msg(client_connection* from, std::string command)
{
    std::string message = std::format("msg {}({}): {}", 
        from->name, 
        from->client_id, 
        command.substr(command.find(' ') + 1));
    for (client_connection* client : clients)
        client->add_to_send_queue(message);
}

void mud_server::handle_command_move(client_connection* from, std::string command)
{
    int current_room_id = from->current_room;
    int destination = std::stoi(command.substr(command.find(' ')));
    if (abs(destination - current_room_id) != 1) // not a valid move
        return;
    
    from->current_room = static_cast<ROOMS>(destination);
    from->add_to_send_queue(std::format("move {}", destination));
    print_debug_line(std::format("Client with id {} moving to room {}", from->client_id, static_cast<int>(from->current_room)));
}

void mud_server::handle_command_fish(client_connection* from)
{
    if (from->current_room != FISH_SPOT) // Disallow fishing outside the fishing spot
        return;

    float r = static_cast<float>(rand()) / RAND_MAX;
    
    int heal_amount = 0;
    if (r > 0.5f) // 50% chance to catch a fish
        heal_amount += 10;

    from->heal(heal_amount);
}

void mud_server::handle_command_attack(client_connection* from)
{
    if (from->current_room != BOSS_ROOM) // Validate that the player is in the boss room
        return;
    
    int damage = 10;
    current_boss_health = max(current_boss_health - damage, 0);

    for (client_connection* client : clients)
        client->add_to_send_queue(std::format("attack c{}d{}b{}", from->client_id, damage, current_boss_health));
    
    int client_damage = static_cast<int>((static_cast<float>(rand()) / RAND_MAX) * 25.0f) + 1;
    from->heal(-client_damage);
    
    if (current_boss_health == 0)
    {
        num_defeated++;
        current_boss_health = static_cast<int>(starting_boss_health * pow(1.2, num_defeated));
        std::string new_boss_message = std::format("boss {}", current_boss_health);
        for (client_connection* client : clients)
            client->add_to_send_queue(new_boss_message);
    }
}

void mud_server::handle_sending_messages()
{
    for (client_connection* client : clients)
        client->send_to_client();
}
