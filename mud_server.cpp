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
    int success = getaddrinfo(nullptr, DEFAULT_PORT, &settings, &result);
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
    
    // This now assumes that the socket is non-blocking. This would likely run on a 
    // separate thread while the main thread just acts as another client
    // Basic server loop:
    // 1. Handle server priority task queue
    // 2. Handle any connecting clients
    // 3. Receive any data (if applicable) from all connected clients and push up to task queue
    // 4. Handle all tasks present in the task queue and build up messages to send (likely a property of each client?)
    // 5. Send relevant messages to all clients
    // 6. Sleep a lil'
    // 7. Repeat
    
    print_line("Server set up!");

    constexpr int sleep_time = static_cast<int>((1.0f / SERVER_TICK_RATE) * 1000.0f);
    while (true)
    {
        handle_connecting_clients();
        
        handle_dead_clients();
        
        handle_receiving_client_messages();
        
        handle_tasks();
        
        handle_sending_messages();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    }
    
    std::string input;
    std::getline(std::cin, input);
    
    freeaddrinfo(result);
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
        client_connection* client = clients[index];
        u_int client_id = client->client_id;
        print_debug_line(std::format("Client with id {} getting killed", client_id));
        clients.erase(clients.begin() + index);
        for (client_connection* client : clients)
            client->add_to_send_queue(std::format("kill {}", client_id));
        
        delete client;
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
            
            if (command == "msg")   { handle_command_msg(client, message); continue; }
            if (command == "move")  { handle_command_move(client, message); continue; }
            if (command == "fish")  { handle_command_fish(client); continue; }
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

    from->current_hp = min(from->current_hp + heal_amount, from->MAX_HP);
    
    from->add_to_send_queue(std::format("heal {}", heal_amount));
}

void mud_server::handle_sending_messages()
{
    for (client_connection* client : clients)
        client->send_to_client();
}
