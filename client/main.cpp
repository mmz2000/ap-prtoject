#include <sstream>
#include <SFML/Network.hpp>

const unsigned short PORT = 5000;
const std::string IPADDRESS("127.0.0.1");

sf::TcpSocket socket;

bool Client(void)
{
    if(socket.connect(IPADDRESS, PORT) == sf::Socket::Done)
    {
        return true;
    }
    return false;
}

int main(){
    Client();
    return 0;
}