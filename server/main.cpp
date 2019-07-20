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
std::map<std::string,bool> logged;
std::map<int,std::string> id2user;
std::map<int,sf::TcpSocket *> sockets;
std::map<int,sf::Thread *> threads;
std::map<int,bool> EOT;
std::map<int,database::Queue> Queues;
std::map<int,int> id2qid;

sf::TcpListener listener;
sf::Mutex globalMutex;


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
        logged[username]=false;
    }
    db.close();
}
void writefile()
{
    std::stringstream stream;
    globalMutex.lock();
    std::ofstream db("database.db",std::ios::out);
    std::map<std::string,database::User>::iterator itr;
    for(itr=users.begin();itr!=users.end();itr++)
    {
        itr->second.SerializeToOstream(&stream);
        db<<stream.str()<<std::endl;
    }
    db.close();
    globalMutex.unlock();
}

int Queue_creat(int size)
{
    database::Queue new_queque;
    int a=random_id();
    new_queque.set_id(a);
    new_queque.set_size(size);
    globalMutex.lock();
    Queues[a]=new_queque;
    globalMutex.unlock();
    return(a);
}

response::Scoreboard * SB(const request::Scoreboard sb)
{
    response::Scoreboard * r_value (new response::Scoreboard);
    std::map<int,std::string> score2user;
    std::map<std::string,database::User>::iterator itr;
    for(itr=users.begin();itr!=users.end();itr++)
    {
        score2user.insert(std::pair<int,std::string>(itr->second.score(),itr->second.name()));
    }
    std::string name =users[id2user[sb.session_id()]].name();
    std::map<int,std::string>::reverse_iterator itr2;
    int i=0;
    for(itr2=score2user.rbegin();itr2!=score2user.rend();itr2++)
    {
        if(itr2->second==name){
            r_value->set_user_rate(i);
            r_value->set_user_score(itr2->first);
        }
        if(i<10)
        {
            types::UserScore * US = r_value->add_top_10();
            US->set_name(itr2->second);
            US->set_score(itr2->first);
        }
    }
    return r_value;
}

response::Login * log(const request::Login Log)
{
    response::Login *res(new response::Login);
    
    if(users.find(Log.username()) != users.end() && !logged[Log.username()] && Log.password() == users[Log.username()].password())
    {
        globalMutex.lock();
        int a=random_id();
        while(a==0)
            a=random_id();
        res->set_session_id(a);
        users[Log.username()].set_session_id(a);
        id2user[a]=Log.username();
        logged[Log.username()]=true;
        globalMutex.unlock();
    } else {
        globalMutex.lock();
        res->set_session_id(0);
        globalMutex.unlock();
    }
    return res;
}

response::Register * reg(const request::Register Reg)
{
    response::Register *res(new response::Register);
    database::User user;
    if(users.find(Reg.username()) == users.end() && Reg.password() == Reg.confirm_password())
    {
        user.set_id(random_id());
        user.set_name(Reg.name());
        user.set_password(Reg.password());
        user.set_username(Reg.username());
        user.set_score(0);
        users[user.username()]=user;

        writefile();
        res->set_success(true);
    } else {
        res->set_success(false);
    }
        return res;
}

response::UserInfo * userI(const request::UserInfo userinfo)
{
    int a = userinfo.session_id();
    std::string b = id2user[a];
    response::UserInfo * r_value(new response::UserInfo);
    r_value->set_name(users[b].name());
    r_value->set_score(users[b].score());
    return r_value;
}

response::QueueCreate * Q_creat(const request::QueueCreate QC)
{
    response::QueueCreate * r_value (new response::QueueCreate);
    r_value->set_queue_id(Queue_creat(QC.queue_size()));
    return r_value;
}

response::QueueList * Q_list(const request::QueueList QL)
{
    response::QueueList * r_value (new response::QueueList);
    std::map<int,database::Queue>::iterator itr;
    globalMutex.lock();
    for(itr=Queues.begin();itr!=Queues.end();itr++)
    {
        types::QueueItem * QI = r_value->add_queue_items();
        QI->set_id(itr->second.id());
        QI->set_occupied(itr->second.user_ids_size());
        QI->set_size(itr->second.size());
    }
    globalMutex.unlock();
    return r_value;
}

response::QueueJoin * Q_join(const request::QueueJoin QJ)
{
    globalMutex.lock();
    response::QueueJoin * r_value(new response::QueueJoin);
    if(Queues.find(QJ.queue_id())!=Queues.end()&&Queues[QJ.queue_id()].size()>Queues[QJ.queue_id()].user_ids_size())
    {
        Queues[QJ.queue_id()].add_user_ids(QJ.session_id());
        r_value->set_success(true);
        id2qid[QJ.session_id()]=QJ.queue_id();
    } else
    {
        r_value->set_success(false);
    }
    globalMutex.unlock();
    return r_value;
}

response::QueueStatus * Q_status(const request::QueueStatus QS)
{
    response::QueueStatus * r_value(new response::QueueStatus);
    int Q_id=id2qid[QS.session_id()];
    for(int a : Queues[Q_id].user_ids())
    {
        r_value->add_player_names(users[id2user[a]].name());
    }
    r_value->set_size(Queues[Q_id].size());
    if(Queues[Q_id].user_ids_size()==Queues[Q_id].size())
        r_value->set_initialized(true);
    else
        r_value->set_initialized(false);
    return r_value;
    
}
void Server(int id)
{
    std::cout<<"connected"<<std::flush;
    std::string reqStr;
    sf::Packet packetReceive;
    Response res;
    sf::Packet packetSend;
    sockets[id]->receive(packetReceive);
    if (packetReceive >> reqStr)
    {
        Request req;
        globalMutex.lock();
        req.ParseFromString(reqStr);
        globalMutex.unlock();
        if (req.has_login())
        {
            res.set_allocated_login(log(req.login()));
        } else if(req.has_register_())
        {
            res.set_allocated_register_(reg(req.register_()));
        } else if(req.has_user_info())
        {
            res.set_allocated_user_info(userI(req.user_info()));
        } else if(req.has_queue_create())
        {
            res.set_allocated_queue_create(Q_creat(req.queue_create()));
        } else if(req.has_queue_join())
        {
            res.set_allocated_queue_join(Q_join(req.queue_join()));
        } else if(req.has_queue_list())
        {
            res.set_allocated_queue_list(Q_list(req.queue_list()));
        } else if(req.has_scoreboard())
        {
            res.set_allocated_scoreboard(SB(req.scoreboard()));
        } else if(req.has_queue_status())
        {
            res.set_allocated_queue_status(Q_status(req.queue_status()));
        }

    }
    std::stringstream stream;
    res.SerializeToOstream(&stream);
    packetSend<<stream.str();
    sockets[id]->send(packetSend);
    EOT[id]=true;
}

int main(){
    openfile();
    Queue_creat(5);
    int id=0;
    listener.listen(PORT);
    bool quit=false;
    
    while(!quit)
    {
        sf::TcpSocket *socket_ptr(new sf::TcpSocket);
        sockets[id]=socket_ptr;
        if(listener.accept(*socket_ptr)==sf::Socket::Done)
        {
            sf::Thread *thread_ptr(new sf::Thread(Server,id));
            threads[id] = thread_ptr;
            threads[id]->launch();
            EOT[id]=false;
            id++;
        }
        for(std::map<int,bool>::iterator itr=EOT.begin();itr!=EOT.end();itr++)
        {
            if(itr->second)
            {
                delete threads[itr->first];
                delete sockets[itr->first];
                threads.erase(itr->first);
                sockets.erase(itr->first);
                EOT.erase(itr->first);
            }
        }
        
    }
    
    return 0;
}