#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include "IrcServer.hpp"

class Channel
{
	public:
								Channel();
								Channel(t_command comm, UserConn &user);
								~Channel();
		int						getUserCount();
		void					setChannelName(std::string name);
		void					setTopic(std::string topicName);
		void					setChannelPass(std::string pass);
		void					addUser(UserConn *);
		void					updateMember(std::vector<UserConn *> var);
		std::string				getChannelName();
		std::string				getTopic();
		std::string				getChannelPass();
		std::vector<UserConn *> &getUserVector();
	private:
		std::vector<UserConn *>	channelUsers;
		std::string				channelName;
		std::string				topic;
		std::string				channelPass;
};

#endif