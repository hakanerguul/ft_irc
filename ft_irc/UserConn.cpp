#include "IrcServer.hpp"

UserConn::UserConn(int fd, std::string address, int port)
{
    this->fd = fd;
    this->address = address;
    this->port = port;
    this->checkLogin = 0;
    username = "";
    nickname = "";
}

UserConn::~UserConn() {
    close(fd);
}

int             UserConn::check() {
    return this->checkLogin;
}

int             UserConn::getFD() const {
    return fd;
}

std::string     UserConn::getAddress() const {
    return address;
}

std::string     UserConn::getNickname() const {
    return nickname;
}

std::string     UserConn::getRealName() const {
    return realname;
}

std::string     UserConn::getFullname() const {
    return fullname;
}

std::string     UserConn::getUserName() const {
    return username;
}

int             UserConn::getPort() const {
    return port;
}

void UserConn::updateLogin(int param) { 
    this->checkLogin = param;
}

void UserConn::setUserName(std::string username) { 
    this->username = username;
    this->fullname = this->getNickname() + "!" + this->getUserName() + "@" + this->getAddress();
}

void UserConn::setHostName(std::string address) { 
    this->address = address; 
    this->fullname = this->getNickname() + "!" + this->getUserName() + "@" + this->getAddress();
}

void UserConn::setNickName(std::string nickname) { 
    this->nickname = nickname;
    this->fullname = this->getNickname() + "!" + this->getUserName() + "@" + this->getAddress();

}

void UserConn::setRealName(std::string realname) { 
    this->realname = realname;
    this->fullname = this->getNickname() + "!" + this->getUserName() + "@" + this->getAddress();
}
