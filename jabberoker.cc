#include "method_thread.h"

#include <unistd.h>
#include <cstdio>

#include <string>
#include <sstream>

#include <gloox/client.h>
#include <gloox/message.h>
#include <gloox/messagesession.h>
#include <gloox/messagehandler.h>

#include <zmq.hpp>

static char *progname;

class Bot : public gloox::MessageHandler
{
public:
    Bot(const std::string &jserver, const std::string &id,
            const std::string &passwd)
        : context(1), notifier(context, ZMQ_PUB), xmpp_service(context, ZMQ_REP)
    {
        std::ostringstream ss1, ss2;
        ss1 << "ipc:///tmp/jabberoker-pub-" << getpid() << ".ipc";
        notifier.bind(ss1.str().c_str()); 
        ss2 << "ipc:///tmp/jabberoker-xmpp-" << getpid() << ".ipc";
        xmpp_service.bind(ss2.str().c_str());

        assert(method_thread(this, false, &Bot::xmpp_thread) != 0);

        gloox::JID jid(id);
        client = new gloox::Client(jid, passwd);
        client->setServer(jserver);
        client->registerMessageHandler(this);
        client->connect();
    }

    ~Bot()
    {
        client->disconnect();
        delete client;
    }

    void handleMessage(const gloox::Message &stanza,
            gloox::MessageSession *session)
    {
        if (stanza.body().size() > 0 && stanza.subtype() ==
                gloox::Message::Chat) {
            const gloox::JID &peer = stanza.from();
            std::string body = stanza.body();
            size_t colon;
            if (body.size() >= 5) {
                std::string interest = body.substr(0, 5);
                std::string content = body.substr(5, body.size()-5).c_str();
                std::ostringstream oss;
                oss << interest << peer.full().size() << peer.full()
                    << content.size() << content;
                std::string marshall = oss.str();
                int msg_len = marshall.size();
                zmq::message_t msg(msg_len);
                memcpy((char *)msg.data(), marshall.c_str(), msg_len);
                notifier.send(msg);
            } else {
                printf("unknown msg\n");
            }
        }
    }

    void xmpp_thread()
    {
        while (true) {
            zmq::message_t msg;
            xmpp_service.recv(&msg);
            char *data = (char *)msg.data();            
            std::istringstream iss(std::string(data, msg.size()));
            unsigned sz;
            iss >> sz;
            std::string recipient(data + iss.tellg(), sz);
            iss.seekg(sz, std::ios::cur);
            iss >> sz;
            std::string content(data + iss.tellg(), sz);
            gloox::Message response(gloox::Message::Chat, gloox::JID(recipient),
                    content);
            client->send(response);
            zmq::message_t ack(2);
            memcpy((char *)ack.data(), "OK", 2);
            xmpp_service.send(ack);
        }
    }

private:
    gloox::Client *client;

    zmq::context_t context;
    zmq::socket_t notifier;
    zmq::socket_t xmpp_service;
};

static void usage()
{
    printf("Usage: %s -s <jabber_server> -i <jabber_id> -p <password>\n",
            progname);
}

int main(int argc, char *argv[])
{
    extern char *optarg;

    std::string jabber_server = "";
    std::string jid = "";
    std::string passwd = "";
    progname = argv[0];

    if (argc == 1 || strcmp("-h", argv[1]) == 0) {
        usage();
        return 0;
    }

    char c;
    while ((c = getopt(argc, argv, "s:i:p:")) != -1) {
        switch (c) {
            case 's':
                jabber_server = optarg;
                break;
            case 'i':
                jid = optarg;
                break;
            case 'p':
                passwd = optarg;
                break;
            default:
                usage();
                return 1;
        }
    }

    if (jabber_server == "" || jid == "") {
        usage();
        return 1;
    }
    printf("Jabberoker started (PID %u)\n", (unsigned) getpid());
    Bot bot(jabber_server, jid, passwd);

    return 0;
}

