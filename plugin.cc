#include "plugin.h"
#include "method_thread.h"

#include <sstream>
#include <sys/types.h>
#include <dirent.h>

plugin::plugin(int broker_id)
    : context(1), subscriber(context, ZMQ_SUB), xmpp_sender(context, ZMQ_REQ),
      callback(NULL)
{
    std::ostringstream oss1, oss2;
    oss1 << "ipc:///tmp/" << "jabberoker-pub-" << broker_id << ".ipc";
    std::string pub_sock = oss1.str();
    subscriber.connect(pub_sock.c_str());
    oss2 << "ipc:///tmp/" << "jabberoker-xmpp-" << broker_id << ".ipc";
    std::string xmpp_sock = oss2.str();
    printf("connecting to %s and %s\n", pub_sock.c_str(), xmpp_sock.c_str());
    xmpp_sender.connect(xmpp_sock.c_str());

//    DIR *tmpdir = opendir("/tmp");
//    if (tmpdir) {
//        // connect to the first found unix domain socket with valid name
//        // pattern 
//        struct dirent *cur;
//        while ((cur = readdir(tmpdir)) != NULL) {
//            std::string entry(cur->d_name);
//            if (entry.find("jabberoker-") == 0) {
//                oss << entry;
//                entry = oss.str();
//                subscriber.connect(entry.c_str());
//            }
//        }
//    }
    assert((receiver_th = method_thread(this, false, &plugin::recv_loop)) != 0);
}

void plugin::set_msg_callback(msg_callback *cb)
{
    callback = cb;
}

void plugin::reg_interest(const std::string &interest)
{
    subscriber.setsockopt(ZMQ_SUBSCRIBE, interest.c_str(), interest.size());
}

void plugin::recv_loop()
{
    while (true) {
        zmq::message_t msg;
        subscriber.recv(&msg);        
        char *data = (char *)msg.data();
        assert(msg.size() >= 5);
        std::istringstream iss(std::string(data, msg.size()));
        std::string interest(data, 5);
        iss.seekg(5, std::ios::cur);
        unsigned sz;
        iss >> sz;
        std::string from(data + iss.tellg(), sz);
        iss.seekg(sz, std::ios::cur);
        iss >> sz;
        std::string content(data + iss.tellg(), sz);

        if (callback) {
            std::ostringstream output, response;
            callback->on_msg(interest, from, content, output);
            std::string output_str = output.str();
            response << from.size() << from << output_str.size() << output_str;
            std::string response_str = response.str();
            zmq::message_t zresp(response_str.size());
            memcpy((char *)zresp.data(), response_str.c_str(),
                    response_str.size());
            xmpp_sender.send(zresp);
            zmq::message_t ack;
            xmpp_sender.recv(&ack);
            assert(std::string((char *)ack.data(), ack.size()) == "OK");
        }
    }
}

