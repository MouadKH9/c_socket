#include "libServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
    User users[32];
    message msgs[128];
    initUsers(users);
    getUsers(users);
    getMsgs(msgs);
    launchServer(users,msgs);
    return 0;
}
