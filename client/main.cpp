#include <sstream>
#include <iostream>
#include <SFML/Network.hpp>
#include "request.pb.h"
#include "communication.pb.h"

const unsigned short PORT = 5000;
const std::string IPADDRESS("127.0.0.1");

int sessionid=0;
int Queue_id=-1;
std::string name;
int score;

sf::TcpSocket socket;
sf::Mutex globalMutex;

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
        
        std::cout<<"enter password\n";
        std::cin>>password;
        
        globalMutex.lock();
        temp->set_password(password);
        temp->set_username(username);
        
        log.set_allocated_login(temp);
        
        log.SerializeToOstream(&stream);
        packetSend << stream.str();
        globalMutex.unlock();

        socket.send(packetSend);
        socket.receive(packetReceive);
        if (packetReceive >> resStr)
        {
            Response rep;
            rep.ParseFromString(resStr);
            if(rep.has_login())
            {
                if(rep.login().session_id()!=0)
                {
                    sessionid=rep.login().session_id();
                    done=true;
                }
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
        
        
        std::cout<<"enter username\n"<<std::flush;
        std::cin>>username;
        
        
        std::cout<<"enter password\n"<<std::flush;
        std::cin>>password;
        
        std::cout<<"re-enter password\n"<<std::flush;
        std::cin>>confirm;
        globalMutex.lock();
        temp->set_name(name);
        temp->set_username(username);
        temp->set_password(password);
        temp->set_confirm_password(confirm);
        reg.set_allocated_register_(temp);
    
        reg.SerializeToOstream(&stream);
        packetSend << stream.str();
        globalMutex.unlock();

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
    std::cout<<"now try loging in";
}

void login_register()
{
    Client();
    std::cout << "(L) for login (R) for register";
    char action;
    std::cin >> action;
    if( action == 'L' )
        login();
    else if ( action == 'R' )
        reg();
    else
    {
        std::cout << "wrong input \n goodbye";
        exit(1);
    }
    socket.disconnect();    
}

void user_info()
{
    Client();
    Request info;
    std::string RecStr;
    std::stringstream stream;
    sf::Packet packetSend;
    sf::Packet packetRecieve;
    request::UserInfo * uinfo_ptr(new request::UserInfo);
    uinfo_ptr->set_session_id(sessionid);
    info.set_allocated_user_info(uinfo_ptr);
    info.SerializeToOstream(&stream);
    globalMutex.lock();
    packetSend<<stream.str();
    globalMutex.unlock();
    socket.send(packetSend);
    socket.receive(packetRecieve);
    if(packetRecieve>>RecStr)
    {
        response::UserInfo RUI;
        RUI.ParseFromString(RecStr);
        globalMutex.lock();
        name = RUI.name();
        score = RUI.score();
        globalMutex.unlock();
    }
    socket.disconnect(); 

}

void Q_list()
{
    Client();
    std::stringstream stream;
    sf::Packet packetSend;
    sf::Packet packetReceive;
    std::string resSrt;
    Request reg;
    request::QueueList *QL(new request::QueueList);
    QL->set_session_id(sessionid);
    reg.set_allocated_queue_list(QL);
    reg.SerializeToOstream(&stream);
    packetSend << stream.str();

    socket.send(packetSend);
    socket.receive(packetReceive);
    if (packetReceive >> resSrt)
    {
        Response rep;
        rep.ParseFromString(resSrt);
        if(rep.has_queue_list())
        {
            for(types::QueueItem QI:rep.queue_list().queue_items())
            {
                std::cout<<"Queue ID: "<<QI.id()<<" Queue occupied/size: "<<QI.occupied()<<"/"<<QI.size()<<std::endl;
            }
        }
    }
    socket.disconnect();
}
bool Q_join(int Q_id)
{
    Client();
    std::stringstream stream;
    sf::Packet packetSend;
    sf::Packet packetReceive;
    std::string resSrt;
    Request reg;
    request::QueueJoin *QJ(new request::QueueJoin);
    QJ->set_session_id(sessionid);
    QJ->set_queue_id(Q_id);
    reg.set_allocated_queue_join(QJ);
    reg.SerializeToOstream(&stream);
    packetSend << stream.str();

    socket.send(packetSend);
    socket.receive(packetReceive);
    if (packetReceive >> resSrt)
    {
        Response rep;
        rep.ParseFromString(resSrt);
        if(rep.has_queue_join())
        {
            if(rep.queue_join().success())
                Queue_id=Q_id;
        }
        socket.disconnect();
        return rep.queue_join().success();
    }
    return false;
}
void Q_create(void) 
{

    Client();
    std::stringstream stream;
    sf::Packet packetSend;
    sf::Packet packetReceive;
    std::string resSrt;
    Request reg;
    request::QueueCreate *QC(new request::QueueCreate);
    QC->set_session_id(sessionid);
    int a;
    std::cout<<"input size of the new queue";
    std::cin>>a;
    QC->set_queue_size(a);
    reg.set_allocated_queue_create(QC);
    reg.SerializeToOstream(&stream);
    packetSend << stream.str();

    socket.send(packetSend);
    socket.receive(packetReceive);
    if (packetReceive >> resSrt)
    {
        Response rep;
        rep.ParseFromString(resSrt);
        if(rep.has_queue_create())
        {
            socket.disconnect();
            Q_join(rep.queue_create().queue_id());
        }
    }

}

void Qmanager()
{
    
    while(Queue_id==-1)
    {
        Q_list();
        std::cout<<"(J) to join a queue (C) to create\n";
        char a;
        std::cin>>a;
        if(a=='J')
        {
            std::cout<<"enter queueid";
            int b;
            std::cin>>b;
            if(!Q_join)
            {
                std::cout<<"try again";
            }
        } else
        {
            Q_create();
        }
        
    }
}
int main(){
    while(sessionid==0)
        login_register();
    Qmanager();
    return 0;
}