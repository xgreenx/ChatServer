# ChatServer
Before usage you need to install 'libevent'.

Ubuntu:
sudo apt-get install libevent-dev

Run:
make && ./make <port>

default port is 2283


Server supports commands:

/list - return list of online users

/nickname <nickname> - change nickname to <nickname> (example "/nickname Green" will change your nickname to Green)

/registr <password> - lock your current nickname by <password>

/exit - disconnect you
