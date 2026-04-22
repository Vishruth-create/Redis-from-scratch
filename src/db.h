#include <unordered_map>
#include <deque>
#include <list>
#include <chrono>
#include <mutex>
#include <condition_variable>

std::mutex mtx;

struct Value
{
    bool type{}; //type true for string false for list
    std::string value{};
    std::deque<std::string> dq{};
};

struct WaitingClient
{
    int client_fd{};
    std::condition_variable cv;
    bool ready = false;
    std::string value{};
};

std::unordered_map<std::string, Value> mp{};
std::unordered_map<std::string, std::chrono::steady_clock::time_point> expiry{};
std::unordered_map<std::string, std::list<std::shared_ptr<WaitingClient>>> waiters{};

//parsed[1] is key and parsed[2] is value

void set(const int &client_fd, const std::vector<std::string> &parsed)
{
    mp[parsed[1]].value = parsed[2];
    mp[parsed[1]].type = true;

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

    if (mp.find(parsed[1]) == mp.end() || mp[parsed[1]].type==false)
    {
        std::string response = "$-1\r\n";
        std::cout << "Key not found!!" << std::endl;
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }

    std::cout << "Found : " << mp[parsed[1]].value << std::endl;
    std::string response = "$" + std::to_string(mp[parsed[1]].value.size()) + "\r\n" + mp[parsed[1]].value + "\r\n";
    send(client_fd, response.c_str(), response.size(), 0);
    return;
}

void rpush(const int &client_fd, const std::vector<std::string> &parsed)
{
    std::unique_lock<std::mutex> lock(mtx);
    std::string key = parsed[1];
    if(mp.find(parsed[1]) == mp.end()) mp[parsed[1]].type = false;

    if(mp[key].type == true)
    {
        std::string response = "-ERR wrong type\r\n";
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }
    size_t len = mp[key].dq.size();
    size_t elements = parsed.size();
    for(size_t e = 2; e < elements; e++)
    {
        std::string val = parsed[e];
        if(waiters[key].empty() == true) {mp[key].dq.push_back(val);}
        else
        {
            auto wc = waiters[key].front();
            waiters[key].pop_front();
            wc->value = val;
            wc->ready = true;
            wc->cv.notify_one();
        }
        len++;
    }

    std::string response = ":" + std::to_string(len) + "\r\n";
    lock.unlock();

    send(client_fd, response.c_str(), response.size(), 0);
}

void lpush(const int &client_fd, const std::vector<std::string> &parsed)
{
    std::unique_lock<std::mutex> lock(mtx);
    std::string key = parsed[1];
    if(mp.find(parsed[1]) == mp.end()) mp[parsed[1]].type = false;

    if(mp[key].type == true)
    {
        std::string response = "-ERR wrong type\r\n";
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }
    size_t len = mp[key].dq.size();
    size_t elements = parsed.size();
    for(size_t e = 2; e < elements; e++)
    {
        std::string val = parsed[e];
        if(waiters[key].empty() == true) {mp[key].dq.push_front(val);}
        else
        {
            auto wc = waiters[key].front();
            waiters[key].pop_front();
            wc->value = val;
            wc->ready = true;
            wc->cv.notify_one();
        }
        len++;
    }

    std::string response = ":" + std::to_string(len) + "\r\n";
    lock.unlock();

    send(client_fd, response.c_str(), response.size(), 0);
}

void lrange(const int &client_fd, const std::vector<std::string> &parsed)
{
    if(mp.find(parsed[1]) != mp.end() && mp[parsed[1]].type == false)
    {
        int starting = std::stoi(parsed[2]);
        int end = std::stoi(parsed[3]);
        
        if(starting < 0) starting += mp[parsed[1]].dq.size();
        if(end < 0) end += mp[parsed[1]].dq.size();
        if(starting<0) starting = 0;
        if(starting >= mp[parsed[1]].dq.size() || end<starting)
        {
            std::cout << "Inavalid Indices!!" << std::endl;
            std::string response = "*0\r\n";
            send(client_fd, response.c_str(), response.size(), 0);
            return;
        }
        if (end >= mp[parsed[1]].dq.size()) end = mp[parsed[1]].dq.size()-1;
        int elements = end-starting+1;
        std::string response = "*" + std::to_string(elements) + "\r\n";
        for(int e = 0; e < elements; e++)
        {
            response+=("$" + std::to_string(mp[parsed[1]].dq[starting+e].length()));
            response+="\r\n";
            response+=mp[parsed[1]].dq[starting+e];
            response+="\r\n";
        }
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }
    std::string response = "*0\r\n";
    send(client_fd, response.c_str(), response.size(), 0);
}

void llen(const int& client_fd, const std::vector<std::string> &parsed)
{
    std::string response = ":";
    if(mp.find(parsed[1]) != mp.end() && mp[parsed[1]].type == false)
    {
        response+=std::to_string(mp[parsed[1]].dq.size());
        response+="\r\n";
    }
    else response = ":0\r\n";
    send(client_fd, response.c_str(), response.size(), 0);
}

void lpop(const int& client_fd, const std::vector<std::string> &parsed)
{
    std::string response{};
    if(mp.find(parsed[1]) == mp.end() || (mp[parsed[1]].type == true || mp[parsed[1]].dq.empty()==true))
    {
        response = "$-1\r\n";
    }
    else
    {
        response = "$" + std::to_string(mp[parsed[1]].dq[0].length()) + "\r\n" + mp[parsed[1]].dq[0] + "\r\n";
        mp[parsed[1]].dq.pop_front();
    }
    send(client_fd, response.c_str(), response.size(), 0);
}

void lpop_(const int& client_fd, const std::vector<std::string> &parsed)
{
    int elements = std::stoi(parsed[2]);
    std::string response{};
    if(mp.find(parsed[1]) == mp.end() || (mp[parsed[1]].type == true || mp[parsed[1]].dq.empty()==true))
    {
        response = "$-1\r\n";
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }
    elements = (elements > mp[parsed[1]].dq.size() ? mp[parsed[1]].dq.size() : elements);
    response = "*" + std::to_string(elements) + "\r\n";
    for(int e = 0; e < elements; e++)
    {
        response+= "$" + std::to_string(mp[parsed[1]].dq[0].length()) + "\r\n" + mp[parsed[1]].dq[0] + "\r\n";
        mp[parsed[1]].dq.pop_front();
    }
    send(client_fd, response.c_str(), response.size(), 0);
}

void blpop(const int&client_fd, const std::vector<std::string> &parsed)
{
    std::string key = parsed[1];
    std::unique_lock<std::mutex> lock(mtx);

    if(mp.find(key) != mp.end() && mp[key].type == false && !mp[key].dq.empty())
    {
        std::string val = mp[key].dq[0];
        mp[key].dq.pop_front();
        lock.unlock();

        std::string response = "*2\r\n$" + std::to_string(key.size()) + "\r\n" + key + "\r\n" + "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
        send(client_fd, response.c_str(), response.length(), 0);
        return;
    }

    auto wc = std::make_shared<WaitingClient>();
    wc->client_fd = client_fd;

    waiters[key].push_back(wc);

    while(wc->ready == false)
    {
        wc->cv.wait(lock); //makes the program wait
    }
    std::string response = "*2\r\n$" + std::to_string(key.length()) + "\r\n" + key + "\r\n" + "$" + std::to_string(wc->value.length()) + "\r\n" + wc->value + "\r\n";
    lock.unlock();
    send(client_fd, response.c_str(), response.size(), 0);
}