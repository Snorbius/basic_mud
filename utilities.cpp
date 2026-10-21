#include "utilities.h"

#include <iostream>

namespace utilities
{
    void print(const std::string& message)
    {
        std::cout << message; 
    }

    void print_line(const std::string& message)
    {
        std::cout << message << "\n";
    }

    void print_debug_line(const std::string& message)
    {
#if IS_DEBUG
        print("DEBUG: ");
        print_line(message);
#endif
    }

    std::string string_to_lower_case(const std::string& string)
    {
        std::string result = string;
        for (auto& c : result)
            c = tolower(c);
        
        return result;
    }
}
