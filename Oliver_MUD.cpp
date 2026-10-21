
#include <iostream>
#include <winsock2.h>

#include "mud_client.h"
#include "mud_server.h"
#include "utilities.h"

using namespace utilities;

void command_help();
void print_commands();
void command_server();
void command_client();

int main()
{
    WSAData wsa_data;
    print_debug_line("Setting up WSA...");
    int res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (res != 0)
    {
        print_line("Could not start WSA, error code: " + std::to_string(res));
        print_line("Exiting...");
        return -1;
    }
    print_debug_line("Successfully set up WSA");
    
    bool running = true;
    std::string input;
    print_line("Hello and welcome to MUD.");
    print_commands();
    // ---------- Main Loop -----------
    while (running)
    {
        print("> ");
        std::getline(std::cin, input);
        input = string_to_lower_case(input);
        
        if (input == "exit")    {running = false;   continue;}
        if (input == "help")    {command_help();    continue;}
        if (input == "server")  {command_server();  continue;}
        if (input == "client")  {command_client();  continue;}
        
        print_line("I couldn't recognize that command. Type HELP if you want to know what I DO recognize.");
    }
    // --------------------------------
    
    print_debug_line("Cleaning up WSA...");
    if (WSACleanup() == SOCKET_ERROR)
    {
        print_line("Could not properly clean up WSA, error code: " + std::to_string(WSAGetLastError()));
        print_line("Exiting...");
        return -1;
    }
    print_debug_line("Successfully cleaned up WSA");
    
    print_line("Goodbye!");
    
    return 0;
}

void command_help()
{
    print_line("You really shouldn't need help when the program is this simple but whatever.");
    print_commands();
}

void print_commands()
{
    print_line("");
    print_line("type HELP to know how to use the program.");
    print_line("type SERVER to launch a server that people can connect to.");
    print_line("type CLIENT to connect to a server that someone else has launched.");
    print_line("type EXIT to exit the program.");
    print_line("");
}

void command_server()
{
    print_debug_line("Starting up server...");
    mud_server server;
    server.run_server();
    print_debug_line("Server has finished running.");
}

void command_client()
{
    print_debug_line("Starting up client...");
    mud_client client;
    client.run_client();
    print_debug_line("Client has finished running.");
}
