#include <IrcServer.hpp>
#include <macro.hpp>

bool nickControl(std::string nickname) {
    if (nickname.length() < 1)
        return (false);
    for (int i = 0; i < static_cast<int>(nickname.length()); i++)
    {
        if ((nickname[i] >= 0 && nickname[i] <= 47))
            return (false);
    }
    return (true);
}

bool checkNick(std::vector<UserConn *> userConns, std::string input) {
    for( std::vector<UserConn *>::iterator it = userConns.begin(); it != userConns.end() ; ++it) {
        if((*it)->getNickname() == input) {
            return(false);
        }
    }
    return(true);
}

IrcServer::IrcServer(int port, std::string password) : port(port), serverFD(-1), password(password) {
}

IrcServer::~IrcServer() {
    if (serverFD >= 0)
        close(serverFD);
    std::vector<UserConn *>::iterator it;
    for (it = userConns.begin(); it != userConns.end(); ++it) {
        delete *it;
    }
}

void IrcServer::start() {
    int opt = 1;
    this->serverFD = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(this->serverFD, F_SETFL, O_NONBLOCK);
    if (this->serverFD < 0)
        throw std::runtime_error("failed to create server socket");
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    if (setsockopt(this->serverFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        throw std::runtime_error("failed to set socket options");
    if (bind(this->serverFD, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0)
        throw std::runtime_error("failed to bind server socket to port");
    else 
        std::cout << "success"<< std::endl;
    if (listen(this->serverFD, 5) < 0) 
        throw std::runtime_error("listening error");
    while (true)
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(this->serverFD, &read_fds);
        int max_fd = this->serverFD;

        std::vector<UserConn *>::iterator it;
        for ( it = userConns.begin(); it != userConns.end(); ++it)
        {
            UserConn *uc = *it;
            int fd = uc->getFD();
            if (fd >= 0) {
                FD_SET(fd, &read_fds);
                max_fd = std::max(max_fd, fd);
            }
        }
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            std::perror("I/O : ");
            std::cerr << "ERROR: Unable to wait for I/O." << std::endl;
            return;
        }
        if (FD_ISSET(this->serverFD, &read_fds))
            newConnection();
        for (it = userConns.begin(); it < userConns.end(); ++it)
        {
            UserConn *uc = *it;
            int fd = uc->getFD();
            std::cout << "      fd: " << fd << std::endl;
            if (fd >= 0 && FD_ISSET(fd, &read_fds))
                processInput(uc);
        }
    }
}


void IrcServer::logIn(UserConn *uc, t_command command) {
    if (command.name == "PASS")
    {
        if(uc->check() == e_begin) {
            if(this->password == "")
            {
                uc->updateLogin(e_pass);
                this->sendUser(uc->getFD(), "Server : Correct Password");
                return;
            }
            if (command.params.size() < 1 && command.args == "")
            {
                std::string message = "461 " + command.name + " : Incorrect Password";
                this->sendUser(uc->getFD(), message);
                return;
            }
            if(command.params.size() < 1)
                command.params.push_back(" ");
            if(command.args == this->password || command.params[0] == this->password) {
                uc->updateLogin(e_pass);
                this->sendUser(uc->getFD(), "Server : Correct Password");
                return;
            }
            else if (command.args != this->password || command.params[0] != this->password)
            {
                std::string message = "464 : Incorrect Password";
                this->sendUser(uc->getFD(), message);
            }
        }
        else
        {
            std::string message = "462 : Unauthorized command, already registered.";
            this->sendUser(uc->getFD(), message);
            return;
        }
    }

    if (command.name == "NICK")
    {
        if (uc->check() < e_pass)
        {
            std::string message = "484 :Your connection is restricted!";
            this->sendUser(uc->getFD(), message);
        }
        else if (command.params.size() < 1)
        {
            std::string message = "432 : Erroneous nickname";
            this->sendUser(uc->getFD(), message); 
        }        
        else if(!checkNick(this->getUserConns(), command.params[0]))
        {
            std::string message = "433 " + command.params[0] + " : Nickname is already in use";
            this->sendUser(uc->getFD(), message);
        }
        else
        {
            uc->setNickName(command.params[0]);
            std::string successMessage = "Your nickname has been successfully set, " + uc->getNickname();
            this->sendUser(uc->getFD(),"Server :" + successMessage);
            uc->updateLogin(e_nickname);
        }

    }
    if (command.name == "USER")
    {
        if(uc->check() == e_begin)
            this->sendUser(uc->getFD(), "484 : Your connection is restricted!");
        else if(uc->check() == e_pass)
            this->sendUser(uc->getFD(), "Server : Your nickname is not set!");
        else if(uc->check() != e_nickname)
            this->sendUser(uc->getFD(), "462 : You're already in the server");
        else
        {
            if((command.params.size() == 3 && command.args != "") || (command.params.size() == 4 && command.args == "")) {
                uc->setUserName(command.params[0]);
                uc->setHostName("localhost");
                uc->setRealName(command.params.size() == 3 ? command.args : command.params[3]);
                uc->updateLogin(e_user);
            }
            else
                this->sendUser(uc->getFD(), "461 : Wrong Usage, try \"USER <username> * * <realname>\"");
        }


    }
    if (uc->check() == e_user)
    {
        uc->updateLogin(e_success);
        std::string message = "001 " + uc->getNickname() + " : Welcome to the world of IRC " + uc->getNickname() + "!" + uc->getUserName() + "@" + uc->getAddress();
        this->sendUser(uc->getFD(), message);
    }
    if (command.name == "QUIT") {
        endUserConn(uc->getFD());
        return;
    }
}

void IrcServer::commieHandle(UserConn *uc, t_command command) {
    if (uc->check() != (e_success) && (command.name != "PASS" && command.name != "NICK" && command.name != "USER" && command.name != "PING" && command.name != "CAP")) {
        this->sendUser(uc->getFD(), "ERROR : You are not logged in.");
        return ;
    }
    if (command.name == "CAP")
    {
        this->sendUser(uc->getFD(), "Server : Server command list :");
        this->sendUser(uc->getFD(), "Server : PASS    = To enter server password for server connection, must be sent before the NICK/USER registration combination.");
        this->sendUser(uc->getFD(), "Server : NICK    = To create/change IRC nickname.");
        this->sendUser(uc->getFD(), "Server : USER    = To specify the username, hostname and real name.");
        this->sendUser(uc->getFD(), "Server : JOIN    = To join/create a channel.");
        this->sendUser(uc->getFD(), "Server : PING    = To test the latency between two users.");
        this->sendUser(uc->getFD(), "Server : KICK    = To kick someone from a channel, can only be used by admins.");
        this->sendUser(uc->getFD(), "Server : PRIVMSG = To send private messages to other users.");
        this->sendUser(uc->getFD(), "Server : NOTICE  = To send a notice messages to other users that cannot be answered.");
        this->sendUser(uc->getFD(), "Server : LIST    = To list the channels in the server.");
        this->sendUser(uc->getFD(), "Server : QUIT    = To disconnect from the server.");
        this->sendUser(uc->getFD(), "Server : NAMES   = To list the users in a channel.");
        this->sendUser(uc->getFD(), "Server : MSG     = To send private messages to other users.");
    }
    if (command.name == "JOIN")
    {
        if (command.params[0][0] != '#')
        {
            this->sendUser(uc->getFD(), "Server : Channel name must start with '#'");
            return ;
        }
        std::cout << "JOIN USER FD => " << uc->getFD() << std::endl;
        if (command.params.size() < 1) {
            std::string message = "461 : not enough parameters";
            this->sendUser(uc->getFD(), message);
        }
        if (checkChannel(channelList, command.params[0]))
        {
            this->addNewChannel(uc, command);
            std::string message = ": you created "+ command.params[0];
            this->sendUser(uc->getFD(), message);
            this->sendUser(uc->getFD(),"353 = " + command.params[0] + " : " + uc->getNickname());
            this->sendUser(uc->getFD(), ":" + command.params[0] + " " + uc->getFullname() + " :Welcome to Channel " + command.params[0]);
        }
        else
        {
            std::vector<UserConn *> users = channelList.find(command.params[0])->second.getUserVector();
            for (std::vector<UserConn *>::iterator it = users.begin(); it != users.end(); ++it)
            {
                if ((*it)->getFD() == uc->getFD())
                {
                    this->sendUser(uc->getFD(), "443 : You already joined this channel " + command.params[0]);
                    return;
                }
            }
            if (channelList.find(command.params[0])->second.getChannelPass() != "")
            {
                if(command.params[1] == channelList.find(command.params[0])->second.getChannelPass())
                {
                    channelList.find(command.params[0])->second.addUser(uc);
                    this->sendUser(uc->getFD(), "353 = " + command.params[0] + " : " + uc->getNickname());
                    this->sendUser(uc->getFD(), ":" + command.params[0] + " " + uc->getFullname() + " :Welcome to Channel " + command.params[0]);
                    for(std::vector<UserConn *>::iterator it = users.begin(); it != users.end(); ++it)
                    {
                        std::string message = uc->getNickname() + " joined channel " + command.params[0];
                        this->sendUser((*it)->getFD(), message);
                    }
                }
                else
                    this->sendUser(uc->getFD(), "475 :" + command.params[0] + ": Cannot join to channel");
            }
            else
            {
                channelList.find(command.params[0])->second.addUser(uc);
                this->sendUser(uc->getFD(), "332 " + command.params[0]+ " : " + channelList.find(command.params[0])->second.getTopic());
                this->sendUser(uc->getFD(), "353 = " + command.params[0] + " : " + uc->getNickname());
                this->sendUser(uc->getFD(), ":" + command.params[0] + " " + uc->getFullname() + " : Welcome to Channel " + command.params[0]);
                for(std::vector<UserConn *>::iterator it = users.begin(); it != users.end(); ++it)
                {
                    std::string message = uc->getNickname() + " joined to channel " + command.params[0];
                    this->sendUser((*it)->getFD(), message);
                }
            }
        }
    }
    if (command.name == "TOPIC")
    {
        bool sender_in_channel = 0;
        if (command.params.size() < 1) {
            std::string message = "461 : not enough parameters";
            this->sendUser(uc->getFD(), message);
            return ;
        }
         if (command.params[0][0] != '#')
        {
            this->sendUser(uc->getFD(), "Server : Channel name must start with '#'");
            return ;
        }
        std::map<std::string , Channel>::iterator channelit = channelList.find(command.params[0]);
        std::vector<UserConn *> users =  (*channelit).second.getUserVector();
        for (std::vector<UserConn *>::iterator it = users.begin(); it != users.end(); ++it) {
            if (uc->getFD() == (*it)->getFD()) {
                sender_in_channel = 1;
                break;
            }
        }
        if (!sender_in_channel)
        {
            std::string message = "442 " + command.params[0] + ": You are not connected to the channel  ";
            this->sendUser(uc->getFD(), message);
            return ;
        }
        else
        {
            if(command.params[1] != "")
                channelit->second.setTopic(command.params[1]);
            if (command.params[1] != "")
                this->sendUser(uc->getFD(), "332 :" + channelit->second.getChannelName() + " :" + command.params[1]);
            else
                this->sendUser(uc->getFD(), "331 :" + channelit->second.getChannelName() + " : The topic is not set");
        }
    }
    if (command.name == "PRIVMSG" || command.name == "MSG") {
        if (command.params.size() < 1)
        {
            std::string message = "461 " + command.name + " : not enough parameters";
            this->sendUser(uc->getFD(), message);
            return;
        }
        if (command.params[0][0] == '#')
        {
            this->privMSG(uc, command, command.params[0]);
            return;
        }
        this->privMSG(uc, command);
    }
    if (command.name == "KICK") {
        this->kick(uc, command);
    }
    if (command.name == "NOTICE") {
        this->noticeMSG(uc, command);
    }
    if (command.name == "PING") {
        this->sendUser(uc->getFD(), "SERVER :" + uc->getFullname() + " PONG : " + uc->getNickname());
    }
    if (command.name == "LIST")
    {
        std::map<std::string , Channel>::iterator it = channelList.begin();
        if(it == channelList.end())
        {
            this->sendUser(uc->getFD(), "323 : There is no channel in server");
            return ;
        }
        for (it = channelList.begin(); it != channelList.end(); ++it)
        {
            std::string lst = it->second.getChannelName().substr(1) + " " + std::to_string(it->second.getUserCount()) + " :" + it->second.getTopic(); 
            this->sendUser(uc->getFD(), "322 server "+ lst);
            std::cout<< "322 server " + lst<<std::endl;
        }
        std::string channels;
        for (it = channelList.begin(); it != channelList.end(); ++it)
        {
            channels = channels + it->second.getChannelName() + " "; 
        }
        this->sendUser(uc->getFD(), "Server: "+ channels);
        this->sendUser(uc->getFD(), "323 ");
    }
    if (command.name == "NAMES")
    {
        if (command.params.size() < 1)
        {
            std::string message = "461 : not enough parameters";
            this->sendUser(uc->getFD(), message);
            return;
        }        
        if (command.params[0][0] != '#' && command.params[0]!="")
        {
            this->sendUser(uc->getFD(), "Server : Channel name must start with '#'");
            return ;
        }
        if (checkChannel(channelList, command.params[0]))
        {
            std::string message = "403 " + command.params[0] + " : No such channel";
            this->sendUser(uc->getFD(), message);
        }
        else
        {
            std::vector<UserConn *> users = channelList.find(command.params[0])->second.getUserVector();
            std::vector<UserConn *>::iterator it = users.begin();
            if(it == users.end())
                this->sendUser(uc->getFD(), "Server : No users in channel");
            for (it = users.begin(); it != users.end(); ++it)
            {
                this->sendUser(uc->getFD(), "Server : " + (*it)->getNickname());
            }
        }
    }
}


void IrcServer::newConnection() {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(serverFD, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    if (client_fd < 0)
    {
        std::cerr << "ERROR: Unable to accept new connection." << std::endl;
        return;
    }
    char address_buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), address_buffer, INET_ADDRSTRLEN);
    int port = ntohs(client_addr.sin_port);
    addUserConn(client_fd, address_buffer, port);
}

void IrcServer::processInput(UserConn *uc) {
    char buff[512];
    int fd = uc->getFD();
    int n = read(fd, buff, 511);
    if (n < 0)
    {
        endUserConn(fd);
        return;
    }
    if (n == 0)
    {
        endUserConn(fd);
        return;
    }
    buff[n] = '\0';
    uc->buff += buff;
    std::string ret;
    while (uc->buff.find('\n') != std::string::npos)
    {
        ret = uc->buff.substr(0, uc->buff.find('\n') + 1);
        uc->buff.erase(0, uc->buff.find('\n') + 1);
        ret.pop_back();
        if (ret.back() == '\r')
            ret.pop_back();
        t_command command = parseCommand(ret);
        std::cout << "command : " << ret << std::endl;
        std::cout << "name : " << command.name << std::endl;
        std::cout << "params :" << std::endl;
        for (std::vector<std::string>::iterator it = command.params.begin(); it != command.params.end(); ++it)
            std::cout << "<" << *it << ">" << std::endl;
        std::cout << "args : " << command.args << std::endl;

        commieHandle(uc, command);
        logIn(uc, command);
    }
}

void IrcServer::addUserConn(int fd, std::string address, int port) {
    UserConn *uc = new UserConn(fd, address, port);
    userConns.push_back(uc);
    std::cout << "New connection from " << address << ":" << port << std::endl;
}

void IrcServer::endUserConn(int fd) {
    for (std::map<std::string, Channel>::iterator chn = this->channelList.begin(); chn != this->channelList.end(); chn++)
    {
           std::vector<UserConn *>&usr =  (*chn).second.getUserVector();
           for (std::vector<UserConn*>::iterator it = usr.begin(); it < usr.end(); ++it)
           {
                if ((*it)->getFD() == fd)
                {
                    usr.erase(it);
                    break;
                }
           }
    }

    std::vector<UserConn *>::iterator it = userConns.begin();
    while (it != userConns.end()) {
        UserConn *uc = *it;
        if (uc->getFD() == fd) {
            userConns.erase(it);
            delete uc;
            std::cout << "Connection ended." << std::endl;
            break;
        }
        ++it;
    }
}

void IrcServer::sendAll(std::string message) {
    for (std::vector<UserConn *>::iterator it = userConns.begin(); it != userConns.end(); ++it) {
        int fd = (*it)->getFD();
        if (fd >= 0) {
            sendUser(fd, message);
        }
    }
}

void IrcServer::privMSG(UserConn *uc, t_command comm, std::string channelName) {
    bool recived_channel = 0;
    bool sender_in_channel = 0;
    int i = comm.params.size() - 1;
    for (std::map<std::string , Channel>::iterator it = channelList.begin(); it != channelList.end(); ++it)
    {
        if (it->first == channelName)
            recived_channel = 1;
    }

    if (recived_channel)
    {
        std::map<std::string , Channel>::iterator channelit = channelList.find(channelName);
        std::vector<UserConn *> users =  (*channelit).second.getUserVector();
        for (std::vector<UserConn *>::iterator it = users.begin(); it != users.end(); ++it) {
            if (uc->getFD() == (*it)->getFD()) {
                sender_in_channel = 1;
                break;
            }
        }
        if (!sender_in_channel)
        {
            std::string message = "442 " + comm.params[0] + ": You are not connected to the channel  ";
            this->sendUser(uc->getFD(), message);
            return ;
        }
        
        while(comm.params[i] != "" && i > 0)
            comm.args.insert(0,comm.params[i--] + " ");

        std::string channel_message = ":" + comm.params[0] + " | " + uc->getNickname() + " :";
        channel_message += comm.args;
        for (std::vector<UserConn *>::iterator it = users.begin(); it < users.end(); ++it) {
            this->sendUser((*it)->getFD(), channel_message);
        }
    }
    else{
        this->sendUser(uc->getFD(), "ERROR : Channel Not Found");
    }
}

void IrcServer::privMSG(UserConn *uc, t_command comm) {
    int reciver_fd = -1;
    int i = comm.params.size() - 1;
    std::string message = uc->getNickname() + " says: ";
    for (std::vector<UserConn *>::iterator it = this->userConns.begin(); it != this->userConns.end(); ++it) {
        if ((*it)->getNickname() == comm.params[0])
        {
            reciver_fd = (*it)->getFD();
            break;
        }
    }
    if (reciver_fd == -1)
    {
        std::string message = "401 " + comm.params[0] + " : No such nick/channel";
        this->sendUser(uc->getFD(), message);
    }
    while(comm.params[i] != "" && i > 0)
        comm.args.insert(0,comm.params[i--] + " ");
    std::cout<<comm.args;
    if (comm.args != "")
        message = message + " " + comm.args;
    this->sendUser(reciver_fd, message);
}

void IrcServer::kick(UserConn *uc, t_command comm) {
    if (comm.params.size() != 2)
    {
        this->sendUser(uc->getFD(), "461 : KICK usage 'KICK <channel> <user>'");
        return;
    }
    std::map<std::string , Channel>::iterator channelit = this->channelList.find(comm.params[0]);
    if (channelit == channelList.end())
    {
        std::string message = "403 " + comm.params[0] + " : No such channel";
        this->sendUser(uc->getFD(), message);
        return;
    }
    if (uc->getFD() != (*channelit).second.getUserVector()[0]->getFD())
    {
        std::string message = "401 " + comm.params[1] + " : No such nick/channel";
        this->sendUser(uc->getFD(), message);
        return ;
    }
    int user_number = 0;
    std::vector<UserConn *> &channelUserVector = (*channelit).second.getUserVector();
    for (std::vector<UserConn *>::iterator it = channelUserVector.begin(); it != channelUserVector.end(); ++it)
    {
        if ((*it)->getNickname() == comm.params[1])
        {
            sendUser((*it)->getFD(), "kicked " + comm.params[0] + " by operator");
            channelUserVector.erase(std::next(channelUserVector.begin() , user_number));
            break;
        }
        user_number++;
    }
    for (std::vector<UserConn *>::iterator it = channelUserVector.begin(); it != channelUserVector.end(); ++it)
    {
        this->sendUser((*it)->getFD(), comm.params[1] + " kicked from channel by operator");
    }
}

void IrcServer::noticeMSG(UserConn *uc, t_command comm) {
    if (comm.params.empty())
        return;
    int reciver_fd = -1;
    for (std::vector<UserConn *>::iterator it = this->userConns.begin(); it != this->userConns.end(); ++it) {
        if ((*it)->getNickname() == comm.params[0]) {
            reciver_fd = (*it)->getFD();
            break;
        }
    }
    std::string message = "NOTICE " + uc->getFullname() + " :";
    std::vector<std::string>::iterator it = comm.params.begin();
    ++it;
    for (; it != comm.params.end(); ++it) {
        message = message + " " + *it; 
    }
    if (comm.args != "")
        message = message + " " + comm.args;
    if (reciver_fd == -1)
    {
        std::string message = "401 " + comm.params[1] + " : No such nick/channel";
        this->sendUser(uc->getFD(), message);
        return;
    }
    this->sendUser(reciver_fd, message);
}

void IrcServer::sendUser(int fd, std::string message) {
    std::string output = message + "\r\n";
    int n = write(fd, output.c_str(), output.size());
    if (n < 0)
    {
        endUserConn(fd);
        return;
    }
}

void IrcServer::addNewChannel(UserConn *uc, t_command comm) {
    Channel newChannel(comm, *uc);
    if (comm.params[1] != "")
        newChannel.setChannelPass(comm.params[1]);
    this->channelList.insert(std::pair<std::string, Channel>(comm.params[0] , newChannel));
    
}

std::vector<UserConn *>	IrcServer::getUserConns() {
    return (userConns);
}