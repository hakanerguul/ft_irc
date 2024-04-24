#ifndef USERCONN_HPP
# define USERCONN_HPP

# include <iostream>
# include <string>
# include <vector>

class UserConn
{
    public:
                        UserConn(int fd, std::string address, int port);
                        ~UserConn();
        int             check();
        int             getFD() const;
        std::string     getAddress() const;
        std::string     getNickname() const;
        std::string     getRealName() const;
        std::string     getFullname() const;
        std::string     getUserName() const;
        int             getPort() const;
        void            updateLogin(int param);
        void            setUserName(std::string username);
        void            setHostName(std::string address);
        void            setNickName(std::string nickname);
        void            setRealName(std::string realname);
        std::string     buff;
    private:
        int         checkLogin;
        int         fd;
        int         port;
        std::string address;
        std::string username;
        std::string nickname;
        std::string realname;
        std::string fullname;
};

#endif
