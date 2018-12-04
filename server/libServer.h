
#include <time.h>

#ifndef SERVER_LIBRARY
#define SERVER_LIBRARY

typedef struct
{
    char name[16];
    int deleted;
} file;

typedef struct{
    char username[16];
    char password[16];
    int etat; // 0 ok 1 blocked 2 deleted
    file files[64];
    unsigned int files_size;
} User;
typedef struct {
    int descr;
    char ip[16];
    unsigned int port;
    char valide;
    int msg_count;
    char username[16];
}connexion;
typedef struct {
  char msg[128];
  char source[16];
  char destination[16];
  unsigned long time;
}message;

void initUsers(User users[]);
int shareFile(char username[], char filename[], char shareWith[], User users[]);
int quit(User users[], message msgs[]);
void handleAdminCmd(char *cmd, User users[], message msgs[]);
void showAdminHelp();
int handleMsg(User users[], connexion client, message msgs[]);
char *checkLogin(char *username, char *password, User users[]);
int addUser(char username[], char password[], User users[]);
int blockUser(char* username, User users[]);
int deleteUser(char* username, User users[]);
void printUsers(User users[]);
void printFiles(int index, User users[]);
char* etat(int etat);
void saveUsers(User users[]);
void getUsers(User users[]);
int saveFileStruct(char username[],char filename[],User users[]);
int deleteFileStruct(char username[],char filename[],User users[]);
int deleteFile(char *filename,char *username);
int createFile(char *FILENAME,char *cont,char *username);
int saveFile(char *path, char* cont);
void sendMsg(char *src,char *dest,char *MSG,message whp[148]);
void scanMsgs(char username[], message whp[], char result[]);
void showMsgs(char username[], message whp[]);
void saveMsgs(message msgs[]);
void launchServer(User users[], message msgs[]);
void handleClientCmd(char *msg, User users[], message msgs[], connexion client);
void writeLog(char* cont,char username[]);
void showFiles(char username[], User users[]);

int createFolder(char *name);
int deleteFolder(char *name);
void showFileContent();
#endif
