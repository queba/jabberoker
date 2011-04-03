#ifndef _PLUGIN_H
#define _PLUGIN_H

#include <string>
#include <sstream>
#include <map>
#include <pthread.h>
#include <zmq.hpp>

class msg_callback {
public:
    msg_callback() {}
    virtual ~msg_callback() {}

    virtual void on_msg(const std::string &from, const std::string &interest,
            const std::string &msg, std::ostringstream &response) = 0;
};

class plugin {
public:
    plugin(int broker_id=-1);

    void run();

    void set_msg_callback(msg_callback *cb);

    void reg_interest(const std::string &interest);

    void recv_loop();

private:
    pthread_attr_t receiver_attr;
    pthread_t receiver_th;

    zmq::context_t context;
    zmq::socket_t subscriber;
    zmq::socket_t xmpp_sender;

    msg_callback *callback;
};

#endif
