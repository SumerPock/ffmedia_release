#include "osd.hpp"
#include <unistd.h>
#include <signal.h>


int main(int argc, char** argv)
{
    if (argc < 2) {
        ff_info("Usage: %s osdConfFile\n", argv[0]);
        return -1;
    }

    ModuleOsd* osd;
    osd = new ModuleOsd(argv[1]);
    int ret = osd->init();
    if (ret < 0) {
        delete osd;
        ff_error("Failed to init osd %d\n", ret);
        return -1;
    }

    osd->start();
    int err, sig;
    sigset_t st;
    sigemptyset(&st);
    sigaddset(&st, SIGINT);
    sigaddset(&st, SIGTERM);
    sigprocmask(SIG_SETMASK, &st, NULL);
    while (true) {
        err = sigwait(&st, &sig);
        if (err == 0) {
            ff_info("receive sig: %d\n", sig);
            break;
        }
    }
    osd->stop();
    delete osd;
    ff_info("%s exit\n", argv[0]);
    return 0;
}
