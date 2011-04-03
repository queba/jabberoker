#include "plugin.h"
#include <cstdio>
#include <cstdlib>

class uptime_callback : public msg_callback {
public:
    void on_msg(const std::string &interest, const std::string &from,
            const std::string &content, std::ostringstream &response)
    {
        char buf[2048];
        FILE *read_fp;
        if ((read_fp = popen("uptime", "r")) != NULL) {
            int nread = fread(buf, sizeof(char), 2048, read_fp);
            response << buf;
        } else {
            response << "Error while retrieving uptime";
        }
    }
};

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <broker_pid>\n", argv[0]);
        exit(1);
    }
    plugin up(atoi(argv[1]));
    up.reg_interest("UPTIM");
    msg_callback *cb = new uptime_callback;
    up.set_msg_callback(cb);
    while (true)
        sleep(1);
}

