#include <unordered_map>
#include <chrono>

std::unordered_map<std::string, std::string> mp{};
std::unordered_map<std::string, std::chrono::steady_clock::time_point> expiry{};

//parsed[1] is key and parsed[2] is value

void set(const int &client_fd, const std::vector<std::string> &parsed)
{
    mp[parsed[1]] = parsed[2];

    if(parsed.size()>=5 && to_upper(parsed[3])=="EX")
    {
        expiry[parsed[1]] = std::chrono::steady_clock::now() + std::chrono::seconds(std::stoi(parsed[4]));
    }
    else if(parsed.size()>=5 && to_upper(parsed[3])=="PX")
    {
        expiry[parsed[1]] = std::chrono::steady_clock::now() + std::chrono::milliseconds(std::stoi(parsed[4]));
    }


    std::cout << "Stored : " << parsed[2] << " in " << parsed[1] << std::endl;
    std::string response = "+OK\r\n";
    std::cout << "Sending : " << response << std::endl;
    send(client_fd, response.c_str(), response.size(), 0);
}

void get(const int &client_fd, const std::vector<std::string> &parsed)
{
    if(expiry.find(parsed[1]) != expiry.end() && expiry[parsed[1]] < std::chrono::steady_clock::now())
    {
        mp.erase(parsed[1]);
    }

    if (mp.find(parsed[1]) == mp.end())
    {
        std::string response = "$-1\r\n";
        std::cout << "Key not found!!" << std::endl;
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }

    std::cout << "Found : " << mp[parsed[1]] << std::endl;
    std::string response = "$" + std::to_string(mp[parsed[1]].size()) + "\r\n" + mp[parsed[1]] + "\r\n";
    send(client_fd, response.c_str(), response.size(), 0);
    return;
}