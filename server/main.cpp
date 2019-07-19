#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>

#include <map>
#include <utility>

#include <SFML/Network.hpp>

#include "communication.pb.h"
#include "database.pb.h"

const unsigned short PORT = 5000;
std::map<std::string,database::User> users;

sf::TcpListener listener;

int random_id ()
{
    boost::uuids::random_generator generator;
    boost::uuids::uuid uuid1= generator();
    int a = uuid1.data[0]+uuid1.data[1]+uuid1.data[2]+uuid1.data[3];
    return a;
}


void openfile ()
{
    std::ifstream db("database.db",std::ios::in);
    std::string str,username;
    while(db>>str)
    {
        database::User user;
        user.ParseFromString(str);
        username=user.username();
        users.insert(std::pair<std::string, database::User>(username,user));
    }
    db.close();
}

response::Login * log(const request::Login Log)
{
    response::Login *res(new response::Login);
    
    if(users.find(Log.username()) != users.end() && Log.password() == users[Log.username()].password())
    {
        res->set_session_id(random_id());
    } else {
        res->set_session_id(0);
    }
    return res;
}

response::Register * reg(const request::Register Reg)
{
    std::stringstream stream;
    std::ofstream db("database.db", std::ios::out | std::ios::app);
    response::Register *res(new response::Register);
    database::User user;
    if(users.find(Reg.username()) == users.end() && Reg.password() == Reg.confirm_password())
    {
        user.set_id(random_id());
        user.set_name(Reg.name());
        user.set_password(Reg.password());
        user.set_username(Reg.username());
        users[user.username()]=user;

        user.SerializeToOstream(&stream);        
        db<<stream.str()<<std::endl;
        
        res->set_success(true);
    } else {
        res->set_success(false);
    }
    db.close();
    return res;
}

void Server(void)
{
    sf::TcpSocket socket;
    listener.accept(socket);
    std::cout<<"connected\n";
    std::string reqStr;
    sf::Packet packetReceive;
    Response res;
    sf::Packet packetSend;
    socket.receive(packetReceive);
    if (packetReceive >> reqStr)
    {
        Request req;
        req.ParseFromString(reqStr);
        if (req.has_login())
        {
            res.set_allocated_login(log(req.login()));
        } else if(req.has_register_())
        {
            res.set_allocated_register_(reg(req.register_()));
        }
    }
    std::stringstream stream;
    res.SerializeToOstream(&stream);
    packetSend<<stream.str();
    socket.send(packetSend);
}

int main(){
    openfile();

    listener.listen(PORT);
    Server();
    return 0;
}