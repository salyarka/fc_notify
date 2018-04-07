#define BUF_SIZE 512
#define PORT "59599"

/* 
 * TODO
 * reasign BUF_SIZE length
 * */

// types of messages
enum msg_type {
    // registration in server with directory,
    // where the creation of new files is tracked
    REGISTRATION,
    // notification to the server about new file in directory
    NOTIFY,
    // subscription for the notification about new files in clients directories
    SUBSCRIPTION
};

struct request {
    char msg[BUF_SIZE];
    enum msg_type type;
};
