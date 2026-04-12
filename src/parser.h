#include <string>
#include <vector>
#include <cassert>

bool is_complete(std::string str)
{
  int elements = -1;
  int i = 0;
  for(; i < str.size(); i++)
  {
    if(str[i]=='\r')
    {
      elements = std::stoi(str.substr(1, i-1));
      i+=2;
      break;
    }
  }
  if(elements < 0) return false;
  for(int e = 0; e < elements; e++)
  {
    if(i+1 >= str.size() || str[i]!='$') return false;
    i++;
    int length_start = i;
    int length = 0;
    for(; i < str.size(); i++)
    {
      if(str[i]=='\r'){length= std::stoi(str.substr(length_start, i-length_start)); break;}
    }
    i+=2; // skipping \r and \n
    i+=length;
    i+=2;
    if(i>str.size()) return false;
  }
  return true;
}

std::string get_full_message(const int &client_fd, char *buffer, ssize_t &bytes_received)
{
  std::string data;
  data.append(buffer, bytes_received);
  while(!is_complete(data))
  {
    ssize_t n = recv(client_fd, buffer, 1023, 0);
    if(n<=0) break;
    bytes_received+=n;
    data.append(buffer, n);
  }
  return data;
}

void parse_bulkstring(const std::string &buffer, const int& elements, std::vector<std::string> &vec, int &i)
{
    int length{};
    i++;
    int start_len = i;
    for(; i < buffer.size(); i++)
    {
        if (buffer[i] == '\r')
            {length = std::stoi(buffer.substr(start_len, i-start_len));i+=2; break;}
    }
    vec.push_back(buffer.substr(i, length));
    i+=length;
    i+=2; //skipping \r and \n
}

std::vector<std::string> parse(std::string buffer)
{
    std::vector<std::string> vec{};
    int bytes_received = buffer.size();
    assert(bytes_received > 0);
    assert(buffer[0] == '*');
    int elements{};
    int i =0;
    for (; i < bytes_received; i++)
    {
        if (buffer[i] == '\r')
            {elements = std::stoi(buffer.substr(1, i - 1));i+=2; break;}
    }

    for(int e = 0; e < elements; e++)
    {
        if(buffer[i]=='$') parse_bulkstring(buffer, elements, vec, i);
    }
    return vec;
}