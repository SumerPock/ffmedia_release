#include "tty_process.hpp"
#include <unistd.h>
#include <signal.h>


static volatile sig_atomic_t sig_received_flag = 0;
void sig_handler(int signo, siginfo_t* info, void* ctx)
{
    sig_received_flag = 1;
}

int main(int argc, char** argv)
{
    int opt;
    int line_number = 0, max_lines = 0, time_diff = -1;
    ModuleTtyProcess::ttyPara para;
    std::string device, datafile, gpsfile, net_dev;

    while ((opt = getopt(argc, argv, "d:g:f:m:s:B:b:p:l:n:t:")) != -1) {
        switch (opt) {
            case 'd':
                device = optarg;
                break;
            case 'g':
                gpsfile = optarg;
                break;
            case 'l':
                line_number = atoi(optarg);
                break;
            case 'f':
                datafile = optarg;
                break;
            case 'm':
                max_lines = atoi(optarg);
                break;
            case 's':
                para.speed = atoi(optarg);
                break;
            case 'B':
                para.databits = atoi(optarg);
                break;
            case 'b':
                para.stopbits = atoi(optarg);
                break;
            case 'p':
                para.parity = atoi(optarg);
                break;
            case 'n':
                net_dev = optarg;
                break;
            case 't':
                time_diff = atoi(optarg);
                break;
            default:
                printf("Usage: %s [Options]\n\n"
                       "Options: \n"
                       "-d                 Set the serial port device name\n"
                       "-s                 Set the serial port device rate\n"
                       "-B                 Set the serial port device data bits\n"
                       "-b                 Set the serial port device stop bits\n"
                       "-p                 Set the serial port device parity check\n"
                       "-g                 Set the filename to receive gps information\n"
                       "-l                 Set the gps information output specified line\n"
                       "-f                 Set the receiving information output file\n"
                       "-m                 Set the maximum number of received data output lines to the file\n"
                       "-n                 Set the network device that obtains the ip address\n"
                       "-t                 Set the verification time difference\n"
                       "\n",
                       argv[0]);
                return 0;
        }
    }

    printf("tty device %s speed %d, data bits %d, stop bits %d, parity %d\n",
           device.c_str(), para.speed, para.databits, para.stopbits, para.parity);

    if (device.empty())
        return -1;

    ModuleTtyProcess tty(device, para);
    tty.setDataOutputFile(datafile, max_lines);
    tty.setGpsDataOutputFile(gpsfile, line_number);
    tty.setInterface(net_dev);
    tty.setTimeDiff(time_diff);
    int ret = tty.init();
    if (ret < 0) {
        printf("Failed to init tty %d\n", ret);
        return -1;
    }

    tty.start();
    int err, sig;
    sigset_t st;
    sigemptyset(&st);
    sigaddset(&st, SIGINT);
    sigaddset(&st, SIGTERM);
    sigprocmask(SIG_SETMASK, &st, NULL);
    while (true) {
        err = sigwait(&st, &sig);
        if (err == 0) {
            printf("receive sig: %d\n", sig);
            break;
        }
    }
    tty.stop();
    return 0;
}
