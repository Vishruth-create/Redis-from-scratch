#include <unordered_map>
std::unordered_map<std::string, std::string> mp{};


void set(const int &client_fd, const std::vector<std::string> &parsed)
{
    mp[parsed[1]] = parsed[2];
    std::cout << "Stored : " << parsed[2] << " in " << parsed[1] << std::endl;
    std::string response = "+OK\r\n";
    std::cout << "Sending : " << response << std::endl;
    send(client_fd, response.c_str(), response.size(), 0);
}

void get(const int &client_fd, const std::vector<std::string> &parsed)
{
    if(mp.find(parsed[1]) == mp.end())
    {
        std::string response = "$-1\r\n";
        std::cout << "Key not found!!" << std::endl;
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }
    std::cout << "Found : " << mp[parsed[1]] << std::endl;
    std::string response = "$" + std::to_string(mp[parsed[1]].size()) + "\r\n" + mp[parsed[1]] + "\r\n";
    send(client_fd, response.c_str(), response.size(), 0);
}