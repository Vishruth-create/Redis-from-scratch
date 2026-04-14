#include "libraries.h"
#include "utilities.h"
#include "parser.h"
#include "db.h"

void handle_request(const std::vector<std::string> &parsed, int client_fd)
{
  for(std::string str : parsed) std::cout << str;

  if (parsed.size()==2 && to_upper(parsed[0]) == "ECHO")
  {
    std::cout << "Got ECHO" << std::endl;
    std::string response = "$" + std::to_string(parsed[1].size()) + "\r\n" + parsed[1] + "\r\n";
    std:: cout << "Sending: " << response << std::endl;
    send(client_fd, response.c_str(), response.size(), 0);
  }

  else if(to_upper(parsed[0]) == "SET" && parsed.size()>=3)
  {
    std::cout << "Got SET" << std::endl;
    set(client_fd, parsed);
  }

  else if(to_upper(parsed[0]) == "GET" && parsed.size() == 2)
  {
    std::cout << "Got GET" << std::endl;
    get(client_fd, parsed);
  }

  else if(to_upper(parsed[0]) == "RPUSH" && parsed.size()>=3)
  {
    std::cout << "Got RPUSH" << std::endl;
    rpush(client_fd, parsed);
  }

  else if(to_upper(parsed[0]) == "LRANGE" && parsed.size()==4)
  {
    std::cout << "Got LRANGE" << std::endl;
    lrange(client_fd, parsed);
  }

  else if(to_upper(parsed[0]) == "PING" && parsed.size()==1)
  {
    const char* response = "+PONG\r\n";
    std:: cout << "Sending: " << response << std::endl;
    send(client_fd, response, strlen(response), 0);
  }

  else
  {
    std::string response = "-ERR something went wrong\r\n";
    send(client_fd, response.c_str(), response.size(), 0);
  }
}


void handle_clients(int client_fd)
{
  char buffer[1024]{};
  while(true)
  {
    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer)-1, 0);
    if(bytes_received==0){std::cout << "Client Disconected" << std::endl; break;}
    else if(bytes_received<0){std::cout << "ERROR!! <0 Bytes Received" << std::endl; break;}
    std::string data = get_full_message(client_fd, buffer, bytes_received);
    std::cout << "Receieved " << data << '\n';

    handle_request(parse(data), client_fd);
  }
  close(client_fd);
}

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  std::cout << "Waiting for a client to connect...\n";

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment the code below to pass the first stage
  while(true){
  int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  std::cout << "Client connected\n";
  std::thread(handle_clients, client_fd).detach();
  }

  std::cout << "Client disconnected!!" << '\n';

  close(server_fd);

  return 0;
}
