#pragma once
#include <string>

#define IS_DEBUG true
#define IP_ADDRESS "127.0.0.1"
#define DEFAULT_PORT "27015"
#define MAX_MESSAGE_LENGTH 64

enum ROOMS
{
    BOSS_ROOM,
    SPAWN_ROOM,
    FISH_SPOT,
};

namespace utilities
{
    void print(const std::string& message);
    void print_line(const std::string& message);
    
    void print_debug_line(const std::string& message);
    
    std::string string_to_lower_case(const std::string& string);
}
