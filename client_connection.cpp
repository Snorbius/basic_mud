#include "client_connection.h"

#include <format>

#include "utilities.h"

using namespace utilities;

void client_connection::receive_message()
{
    char buffer[MAX_MESSAGE_LENGTH];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received == SOCKET_ERROR)
    {
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) // This is fine, just means the client has not sent anything
            return;
        if (error == WSAECONNRESET) // This is fine, the client has disconnected and will soon be killed
        {
            queued_for_deletion = true;
            return;
        }
        
        print_line("Failed to receive message from client, error code: " + std::to_string(error));
        return;
    }
    if (bytes_received == 0)
    {
        queued_for_deletion = true;
        return;
    }

    std::string message(buffer);
    message = message.substr(0, bytes_received);
    task_queue.push(message);
    
    print_debug_line("Received message from client: \"" + message + "\"");
}

void client_connection::add_to_send_queue(std::string message)
{
    send_queue.push(message);
}

void client_connection::send_to_client()
{
    while (!send_queue.empty())
    {
        std::string message = send_queue.front();
        send_queue.pop();
        message += ";";
        send(client_socket, message.data(), static_cast<int>(message.length()), 0);
    }
}
