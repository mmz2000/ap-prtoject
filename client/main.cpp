#include <sstream>
#include <iostream>
#include <SFML/Network.hpp>
#include "request.pb.h"
#include "communication.pb.h"

const unsigned short PORT = 5000;
const std::string IPADDRESS("127.0.0.1");

sf::TcpSocket socket;

bool Client(void)
{
    if(socket.connect(IPADDRESS, PORT) == sf::Socket::Done)
    {
        std::cout << "connected\n";
        return true;
    }
    return false;
}

void login()
{
    
    std::stringstream stream;
    sf::Packet packetSend;
    sf::Packet packetReceive;
    Request log;
    bool done=false;
    request::Login *temp(new request::Login);
    std::string username;
    std::string password;
    std::string resStr;
    while(!done)
    {
        std::cout<<"enter username\n";
        std::cin>>username;
        temp->set_username(username);
        
        std::cout<<"enter password\n";
        std::cin>>password;
        temp->set_password(password);
        
        log.set_allocated_login(temp);
        
        log.SerializeToOstream(&stream);
        packetSend << stream.str();
        socket.send(packetSend);
        socket.receive(packetReceive);
        if (packetReceive >> resStr)
        {
            Response rep;
            rep.ParseFromString(resStr);
            if(rep.has_login())
            {
                if(rep.login().session_id()!=0)
                    done=true;
            }
        }
    }

}

void reg()
{
    std::stringstream stream;
    sf::Packet packetSend;
    
    Request reg;
    request::Register *temp(new request::Register);
    std::string name;
    std::string username;
    std::string password;
    std::string confirm;
    std::string resSrt;
    bool match = false, done = false;
    sf::Packet packetReceive;
    while(!done)
    {
        std::cout<<"enter name\n"<<std::flush;
        std::cin>>name;
        temp->set_name(name);
        
        std::cout<<"enter username\n"<<std::flush;
        std::cin>>username;
        temp->set_username(username);
        
        
        std::cout<<"enter password\n"<<std::flush;
        std::cin>>password;
        temp->set_password(password);
        
        std::cout<<"re-enter password\n"<<std::flush;
        std::cin>>confirm;
        temp->set_confirm_password(confirm);
        reg.set_allocated_register_(temp);
    
        reg.SerializeToOstream(&stream);
        packetSend << stream.str();
        socket.send(packetSend);
        socket.receive(packetReceive);
        if (packetReceive >> resSrt)
        {
            Response rep;
            rep.ParseFromString(resSrt);
            if(rep.has_register_())
            {
                if(rep.register_().success())
                    done=true;
            }
        }

    }
    //std::cout<<"now try loging in";
   // login();
}

void login_register()
{
    std::cout << "(L) for login (R) for register";
    char action;
    std::cin >> action;
    if( action == 'L' )
        login();
    else if ( action == 'R' )
        reg();
    else 
        std::cout << "wrong input \n goodbye";
    exit(1);    
}

int main(){
    Client();
    login_register();
    return 0;
}