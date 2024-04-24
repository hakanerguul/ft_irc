#include "IrcServer.hpp"

Channel::Channel() {}

Channel::Channel(t_command comm, UserConn &user) {
    this->channelName = comm.params[0];
    if (comm.params.size() == 2) {
        this->channelPass = comm.params[1];
    }
    else{
        this->channelPass = "";
    }
    this->channelUsers.push_back(&user);
}

Channel::~Channel() {}

bool checkChannel(std::map<std::string , Channel> channels, std::string input) {
    for (std::map<std::string , Channel>::iterator it = channels.begin(); it != channels.end(); ++it) {
        if(it->second.getChannelName() == input) {
            return(false);
        }
    }
    return(true);
}

int         Channel::getUserCount() {
    return channelUsers.size();
}

void     Channel::setChannelName(std::string name) {
    this->channelName = name; 
}

void     Channel::setTopic(std::string topicName) { 
    this->topic = topicName; 
}

void    	Channel::setChannelPass(std::string pass) {
    this->channelPass = pass;
}

void      Channel::addUser(UserConn *uc) {
    this->channelUsers.push_back(uc);
}

void      Channel::updateMember(std::vector<UserConn *> var) {
    this->channelUsers = var;
}

std::string Channel::getChannelName() {
    return channelName;
}

std::string Channel::getTopic() {
    return topic;
}

std::string Channel::getChannelPass() {
    return channelPass;
}

std::vector<UserConn *> &Channel::getUserVector() {
    return (this->channelUsers);
}
