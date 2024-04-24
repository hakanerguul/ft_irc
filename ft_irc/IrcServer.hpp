#ifndef IRC_SERVER_HPP
#define IRC_SERVER_HPP

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <string.h>
#include <stdio.h>
#include "UserConn.hpp" //used to be #include <UserConn.h> why angle brackets?
#include <stdlib.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "parser.hpp"
#include <fcntl.h>
#include <algorithm>
#include "Channel.hpp"

typedef enum checkLogin
{
	e_begin = 0,
	e_pass = 1,
	e_nickname = 2,
	e_user = 3,
	e_success = 4,
	mnl = 16,
	mnll = 32,
	mnnll = 64,
	mmnnll = 128
} e_login;

class IrcServer
{
	public:
								IrcServer(int port, std::string);
								~IrcServer();
		void					start();
		std::vector<UserConn *>	getUserConns();
	private:
		void							addNewChannel(UserConn *, t_command);
		void							commieHandle(UserConn *, t_command);
		void							logIn(UserConn *, t_command);
		void							newConnection();
		void							processInput(UserConn *uc);
		void							addUserConn(int fd, std::string address, int port);
		void							endUserConn(int fd);
		void							sendAll(std::string message);
		void							sendUser(int fd, std::string message);
		void							privMSG(UserConn *uc, t_command comm);
		void							privMSG(UserConn *uc, t_command comm, std::string channelName);
		void							noticeMSG(UserConn *uc, t_command comm);
		void							kick(UserConn *uc, t_command comm);
		int								port;
		int								serverFD;
		std::vector<UserConn *>			userConns;
		std::map<std::string, Channel>	channelList;
		std::string						password;
};

bool checkChannel(std::map<std::string, Channel> channels, std::string input);

#endif
