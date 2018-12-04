
#include <windows.h>
#include "libServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <netdb.h>
#include <fcntl.h>

#define ERR 	(-1)
#define TAILLEMESSAGE 1025
#define NBRECON 10
#define PORT 5555
#define NCLIENTSMAX 32

int MAXUSERS = 32,MAXMSGS = 256;

void initUsers(User users[]){
    int i;
    int j;
    for(i = 0;i<MAXUSERS;i++){
        bzero(users[i].username,16);
        bzero(users[i].password,16);
        users[i].etat = 2;
        users[i].files_size = 0;
        for(j=0;j<64;j++){
            bzero(users[i].files[j].name,16);
            users[i].files[j].deleted = 0;
        }
    }
}

int quit(User users[],message msgs[]){
    saveUsers(users);
    saveMsgs(msgs);
}

// Afficher le menu d'admin
void showAdminMenu(){
    printf("\nBienvenue a le panneau d'administration!\n");
    printf("Pour afficher l'aide entrer help\n");
    printf("Entrer votre commande:\n");
}
void handleAdminCmd(char* cmd,User users[],message msgs[]){
    char ** res  = NULL;
    char *  p    = strtok (cmd, " ");
    int n_spaces = 0, i;
    char username[16],password[16];

    /* split string and append tokens to 'res' */

    while (p) {
      res = realloc (res, sizeof (char*) * ++n_spaces);

      if (res == NULL)
        exit (-1);

      res[n_spaces-1] = p;

      p = strtok (NULL, " ");
    }

    res = realloc (res, sizeof (char*) * (n_spaces+1));
    res[n_spaces] = 0;

    if(strcmp(res[0],"help") == 0){
        showAdminHelp();
    }else if(strcmp(res[0],"showusers") == 0){
        printUsers(users);
    }else if(strcmp(res[0],"showfiles") == 0){
        if (n_spaces == 2)
        {
            strcpy(username, res[1]);
            showFiles(username, users);
        }
        else
            printf("%d Erreur du syntaxe.\n", n_spaces);
    }
    else if(strcmp(res[0],"adduser") == 0){
        if (n_spaces == 3 && strlen(res[1]) <= 16 && strlen(res[2]) <= 16){
            strcpy(username,res[1]);
            strcpy(password,res[2]);
            addUser(username,password,users);
        }
        else if (strlen(res[1]) > 16 || strlen(res[2]) > 16)
            printf("Le nom d'utilisateur et le mot de passe ne doit pas depasser 16 characteres!\n");
        else
            printf("%d Erreur du syntaxe.\n", n_spaces);
    }
    else if(strcmp(res[0],"deleteuser") == 0){
        if(n_spaces == 2){
            strcpy(username,res[1]);
            deleteUser(username,users);
        }
        else printf("%d Erreur du syntaxe.\n",n_spaces);
    }else if(strcmp(res[0],"blockuser") == 0){
        strcpy(username,res[1]);
        blockUser(username,users);
    }else{
        printf("Votre commande est invalide.\n");
    }
    showAdminMenu();
}

void showAdminHelp(){
    showFileContent("adminHelp.txt");
}
// Login
char * checkLogin(char username[32], char password[32],User users[]){
   int i;
   for(i = 0; i < MAXUSERS ; i++){
        if (strcmp(users[i].username,username) == 0 && strcmp(users[i].password,password) == 0)
             return "ok";
   }
   return "er";
}
// Ajouter un utilisateur (par l'admin)
int addUser(char username[], char password[], User users[]){
    int i;
    printf("adding user %s with password %s \n",username,password);
    if (getUserIndex(username, users) >= 0)
        return -2;
    for(i = 0; i < MAXUSERS; i++){
        if(users[i].etat == 2){
            strcpy(users[i].username,username);
            strcpy(users[i].password,password);
            users[i].etat = 0;
            createFolder(username);
            return i;
        }
    }
    return -1;
}
// Supprimer un utilisateur (par l'admin)
int deleteUser(char username[32], User users[]){
    int i;
    for(i = 0;i<MAXUSERS;i++){
        if (strcmp(users[i].username,username) == 0){
             char cmd[128];
             char path[64];
             users[i].etat = 2;
             sprintf(path,"users/%s",username);
             sprintf(cmd,"exec rm -r %s/*",path);
             system(cmd);
             RemoveDirectory(path);
        }
    }
    return 1;
}
// Blocker un utilisateur (par l'admin)
int blockUser(char* username, User users[]){
    int i;
    if((i=getUserIndex(username,users)) > 0 )
        if (strcmp(users[i].username,username) == 0) users[i].etat = 1;
    return 1;
}
// Afficher la liste des utilisateurs
void printUsers(User users[]){
        int i,foundOne = 0;
        for (i = 0; i < MAXUSERS; i++){
            if (strcmp(users[i].username,"") && users[i].username[0] != '\0' && users[i].etat != 2){
                printf("%d\t %s\t %s\t %d\n", i + 1, users[i].username, etat(users[i].etat), users[i].files_size);
                foundOne = 1;
            }
        }
        if(foundOne == 0)
            printf("Aucun utilisateur est trouve\n");
}
char* etat(int etat){
    return  etat == 0 ? "O.K" : "Blocke";
}
void saveUsers(User users[]){
    int i;
    char result[BUFSIZ*3];
    bzero(result,BUFSIZ*3);
    for (i = 0; i < MAXUSERS; i++)
        if (users[i].username[0] != '\0' && users[i].etat != 2)
            sprintf(result,"%s\n%s %s %d %d",&result,users[i].username,users[i].password,users[i].etat,users[i].files_size) ;
    saveFile("users.txt",result);
}
void getUsers(User users[]){
    FILE *fp;
    DIR *dir;
    struct dirent *ent;
    size_t len;
    int i = 0;
    char line[1024],path[256];
    struct stat path_stat;

    bzero(line,strlen(line));
    bzero(path,strlen(path));
    fp = fopen("users.txt", "r");

    if (fp == NULL){
       printf("Error while opening the file users.txt \n");
       exit(-1);
    }
    while(fgets(line,1024,fp)!=NULL){
        sscanf(line, "%s %s %d %d", users[i].username, users[i].password, &users[i].etat, &users[i].files_size);
        if(!strcmp(users[i].username,"") || users[i].username[0] =='\0')
            continue;
        sprintf(path,"users\\%s\\",users[i].username);
        if ((dir = opendir(path)) != NULL)
        {
            while ((ent = readdir(dir)) != NULL){
                if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..") && strcmp(ent->d_name,"sharedFiles.txt"))
                    saveFileStruct(users[i].username,ent->d_name,users);
            }
            closedir(dir);
        }
        i++;
    }

}
int saveFileStruct(char username[],char filename[],User users[]){
    int i,j;
    if((i=getUserIndex(username,users)) >= 0)
        for(j=0;j<256;j++)
            if(users[i].files[j].name[0] == '\0' || users[i].files[j].deleted == 1){
                strcpy(users[i].files[j].name,filename);
                users[i].files[j].deleted = 0;
                return 1;
            }
    return -1;
}
int deleteFileStruct(char username[],char filename[],User users[]){
    int i,j,size;
    for (i = 0; i < MAXUSERS; i++)
        if (strcmp(users[i].username, username) == 0){
            for (j = 0; j < 64; j++)
            {
                if (strcmp(users[i].files[j].name, filename) == 0)
                {
                    size = deleteFile(filename, username);
                    users[i].files_size -= size;
                    users[i].files[j].deleted = 1;
                    return 1;
                }
            }
        }
    return -1;
}

void showFiles(char username[],User users[]){
    int i, j;
    for (i = 0; i < MAXUSERS; i++)
        if (strcmp(users[i].username, username) == 0)
            for (j = 0; j < 64; j++)
                if (users[i].files[j].name[0] != '\0' && users[i].files[j].deleted == 0){
                    printf("%s\t%d\n", users[i].files[j].name, users[i].files[j].deleted);
                }
}
int deleteFile(char *filename,char *username){
    char path[256];
    int size;
    FILE* fd;
    sprintf(path,"users/%s/%s",username,filename);
    fd = fopen(path,"r");
    if(fd == NULL)
        return -2;
    fseek(fd,0L,SEEK_END);
    size = ftell(fd);
    fclose(fd);
    if(remove(path) == 0) return size;
    return -1;
}
int createFile(char *FILENAME,char *cont,char *username){
    char path[256];
    sprintf(path,"%s\\%s",username,FILENAME);
    saveFile(path,cont);
    return 0 ;
 }
int saveFile(char *path, char* cont){
    FILE *newfile = NULL;
    newfile = fopen(path, "w");
    if (newfile != NULL){
        fprintf(newfile,cont);
        fclose(newfile);
    }
}
void sendMsg(char src[],char dest[],char MSG[],message whp[]){
    int i;
    for( i = 0; i < MAXMSGS; i++){
        if (whp[i].msg[0] == '\0') {
            strcpy(whp[i].msg,MSG);
            strcpy(whp[i].source,src);
            strcpy(whp[i].destination,dest);
            time(&whp[i].time);
            break;
        }
    }
}
void showMsgs(char username[],message whp[MAXMSGS]){
    int i,c=0;
    time_t hm;
    char result[BUFSIZ*3];
    bzero(result,BUFSIZ*2);
    for( i = 0; i < MAXMSGS; i++){
        if (strcmp(whp[i].destination,username) == 0){
            hm = whp[i].time;
            sprintf(result,"%s\n\t%s: \n%s\n%s\n\n",result,whp[i].source,whp[i].msg,ctime(&hm));
            c++;
        }
    }
    if(c==0){
        bzero(result,BUFSIZ*2);
        strcpy(result,"none");
    }
    printf("%s\n",result);
}
void getMsgs(message msgs[]){
    FILE *fp;
    size_t len;
    int i = 0;
    char line[1024];

    fp = fopen("msgs.txt", "r");

    if (fp == NULL){
       printf("Error while opening the file msgs.txt \n");
       exit(-1);
    }
    while(fgets(line,1024,fp)!=NULL){
        sscanf(line,"%[^|]|%s %s %u",msgs[i].msg,msgs[i].source,msgs[i].destination,&msgs[i].time);
        i++;
    }
}
void saveMsgs(message msgs[]){
    int i;
    FILE* fd;
    char line[1048];
    bzero(line,1048);
    fd = fopen("msgs.txt","w");
    for (i = 0; i < MAXMSGS; i++){
        if(msgs[i].msg[0] != '\0'){
            sprintf(line,"%s | %s %s %lu\n",msgs[i].msg,msgs[i].source,msgs[i].destination,msgs[i].time);
            fputs(line,fd);
        }
    }
    fclose(fd);
}

void aff_addr(struct sockaddr_in addr){
   printf("\nIP : %s, port : %d",(char *)inet_ntoa(addr.sin_addr.s_addr),ntohs(addr.sin_port));
}

void afficheErrorSortie(char *nomFct) {
     printf("\n %s -> %s ", nomFct, strerror(errno));
     exit(1);
}

void launchServer(User users[],message msgs[]){
    struct sockaddr_in addrServeur,addrSocket;
    connexion clients[NCLIENTSMAX];
    int sServeur, maxDescr;
    int longAddr,ret_fct,longMessage=TAILLEMESSAGE;
    char msg[TAILLEMESSAGE + 1],cmd[256];
    int i, nbreCon=0;
    pid_t  pid;
    struct timeval tmp={0, 500000};
    fd_set ensemble;
    int flags;
    int test = 0;

    sServeur = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(sServeur==ERR){ afficheErrorSortie("socket()");  exit(1);}

    addrServeur.sin_family = AF_INET;
    addrServeur.sin_port = htons(PORT);
    addrServeur.sin_addr.s_addr=0;


    ret_fct  = bind(sServeur,(struct sockaddr*)&addrServeur,sizeof(struct sockaddr_in));
    if(ret_fct==ERR) afficheErrorSortie("bind()");

    ret_fct=listen(sServeur,NBRECON);
    if(ret_fct==ERR) afficheErrorSortie("listen()");


    for(i=0;i<NCLIENTSMAX;i++)
        clients[i].valide = 0;
    showAdminMenu();
    while (1)
    {
        /* Ici select surveille en lecture le socket de dialogue uniquement */
        FD_ZERO(&ensemble);

        FD_SET(sServeur, &ensemble);
        FD_SET(fileno(stdin), &ensemble);
        maxDescr = sServeur;
        for (i = 0; i < NCLIENTSMAX; i++)
            if (clients[i].valide)
            {
                FD_SET(clients[i].descr, &ensemble);
                if (clients[i].descr > maxDescr)
                    maxDescr = clients[i].descr;
            }

        ret_fct = select(maxDescr + 1, &ensemble, NULL, NULL, &tmp);
        if (ret_fct == ERR)
            afficheErrorSortie("select()");
        else
        {
            if (FD_ISSET(sServeur, &ensemble))
            {
                /* Une nouvelle connxexion*/

                if (nbreCon < NCLIENTSMAX)
                {
                    for (i = 0; i < NCLIENTSMAX; i++)
                        if (clients[i].valide == 0)
                            break;
                    nbreCon++;
                    longAddr = sizeof(struct sockaddr_in);
                    clients[i].descr = accept(sServeur, (struct sockaddr *)&addrSocket, &longAddr);
                    if (clients[i].descr == ERR)
                    {
                        printf("accept()");
                        exit(1);
                    }
                    strcpy(clients[i].ip, (char *)inet_ntoa(addrSocket.sin_addr.s_addr));
                    clients[i].port = ntohs(addrSocket.sin_port);
                    clients[i].valide = 1;
                    clients[i].msg_count = 0;
                }
            }

            for (i = 0; i < NCLIENTSMAX; i++)
                if (clients[i].valide)
                {
                    if (FD_ISSET(clients[i].descr, &ensemble))
                    {
                        /* récupérrer les données*/
                        memset(msg, 0, sizeof(msg));
                        if ((longMessage = read(clients[i].descr, msg, TAILLEMESSAGE + 1)) > 0)
                        {
                            if (!strcmp(msg, "q") || !strcmp(msg, "Q"))
                            {
                                close(clients[i].descr);
                                clients[i].valide = 0;
                                nbreCon--;
                                continue;
                            }
                            writeLog(msg,clients[i].username);
                            switch (clients[i].msg_count)
                            {
                            case 0:
                                clients[i].msg_count++;
                                strcpy(clients[i].username, msg);
                                break;
                            case 1:
                                clients[i].msg_count++;
                                writeLog((char *)checkLogin(clients[i].username, msg, users),clients[i].username);
                                write(clients[i].descr, checkLogin(clients[i].username, msg, users), 2);
                                break;
                            default:
                                handleClientCmd(msg, users, msgs, clients[i]);
                                break;
                            }
                        }
                    }
                }

            if (FD_ISSET(fileno(stdin), &ensemble)) // handle admin cmd
            {
                gets(cmd);
                if (strcmp(cmd, "quit") == 0){
                    quit(users, msgs);
                    exit(1);
                }else
                    handleAdminCmd(cmd, users,msgs);
            }
        }
    }
    close(sServeur);
    for (i = 0; i < NCLIENTSMAX; i++)
        if (clients[i].valide)
            close(clients[i].descr);
    exit(1);
}
void handleClientCmd(char* msg,User users[],message msgs[],connexion client) {
    if (strcmp(msg, "upload") == 0)
        handleUpload(users,client);
    else if (strcmp(msg, "list") == 0)
        handleList(users,client);
    else if (strcmp(msg, "download") == 0)
        handleDownload(client);
    else if (strcmp(msg, "share") == 0)
        handleShare(users,client);
    else if (strcmp(msg, "delete") == 0)
        handleDelete(users,client);
    else if (strcmp(msg, "message") == 0)
        handleMsg(users,client,msgs);
    else if (strcmp(msg, "my_messages") == 0)
        handleShowMsgs(client.username,msgs,client);
}

int handleShare(User users[],connexion client){
    char shareWith[16],filename[16],path[64],res[16];
    FILE* fd;
    bzero(res,16);
    bzero(shareWith,16);
    bzero(path,64);
    bzero(filename,16);

    read(client.descr,filename,16);

    if(fileExists(client.username,filename,users) > 0){
        strcpy(res,"ok");
        write(client.descr, res, 16);
    }else{
        strcpy(res,"!ok");
        write(client.descr, res, 16);
        return -1;
    }
    read(client.descr, shareWith, 16);
    sprintf(path, "users\\%s\\sharedFiles.txt", shareWith);
    fd = fopen(path, "a");
    if (getUserIndex(shareWith,users) >= 0 && fd != NULL){
        strcpy(res, "ok");
        write(client.descr, res, 16);
    }else{
        strcpy(res, "!ok");
        write(client.descr, res, 16);
        return -1;
    }
    fprintf(fd, "users\\%s\\%s\n", client.username, filename);
    fclose(fd);
}

int fileExists(char username[],char filename[],User users[]){
    int i,j;
    i = getUserIndex(username,users);
    if(i<0)
        return -1;
    for(j=0;j<64;j++)
        if (!strcmp(users[i].files[j].name,filename))
            return 1;
    return -1;
}

int handleDelete(User users[], connexion client)
{
    char filename[32],res[16];
    bzero(res,16);
    bzero(filename,32);

    read(client.descr,filename,32);
    if (deleteFileStruct(client.username, filename, users) > 0)
        strcpy(res,"ok");
    else
        strcpy(res,"!ok");
    write(client.descr,res,16);

    return 0;

}

int handleDownload(connexion client){
    char filename[32], owner[16], res[16], path[256], buff[1024 * 5];
    FILE* fd;
    strcpy(res,"ok");
    bzero(filename,32);
    bzero(path,256);
    bzero(buff, 1024 * 5);
    bzero(owner,16);

    read(client.descr,filename,32);
    send(client.descr,res,16,0);

    read(client.descr,owner,16);

    if (!strcmp(owner, "moi")){
        sprintf(path,"users\\%s\\%s",client.username,filename);
    }else{
        sprintf(path, "users\\%s\\%s", owner, filename);
        if(checkPermission(client.username,path) == 0){
            strcpy(res,"permission");
            write(client.descr,res,16);
            return -1;
        }
        sprintf(path, "users/%s/%s", owner, filename);
    }

    fd = fopen(path, "rb");

    if(fd == NULL){
        printf("File not found\n");
        strcpy(res, "notfound");
        write(client.descr, res, 16);
        return -1;
    }

    write(client.descr,res,16);

    while(fgets(buff,1024*5,fd)){
        send(client.descr, buff, 1024 * 5, 0);
        read(client.descr,res,16);
    }
    strcpy(res,"d_done");
    write(client.descr,res,16);
}

int checkPermission(char username[],char path[]){
    char filePath[256],buff[256];
    FILE* fd;

    sprintf(filePath,"users\\%s\\sharedFiles.txt",username);
    fd = fopen(filePath,"r");
    if(fd == NULL)
        return -1;
    strcat(path,"\n");
    while(fgets(buff,256,fd)){
        if(strcmp(buff,path)==0)
            return 1;
    }
    return 0;
}

int handleList(User users[], connexion client){
    char msg[1024],path[256],line[128];
    int i, j,c=0,d=0;
    FILE* fd;
    bzero(msg,1024);
    for (i = 0; i < MAXUSERS; i++)
        if (strcmp(users[i].username, client.username) == 0){
            strcpy(msg,"Mes Fichiers:\n");
            for (j = 0; j < 64; j++)
                if (users[i].files[j].name[0] != '\0' && users[i].files[j].deleted == 0){
                    sprintf(msg, "%s\n+ %s", msg, users[i].files[j].name);
                    c++;
                }
            if(c==0)
                strcat(msg, "\nVous n'avez aucun fichier\n");
            sprintf(path,"users/%s/sharedFiles.txt",users[i].username);
            fd = fopen(path,"r");
            if(fd != NULL){
                strcat(msg, "\n\nFichiers partages avec vous:\n\n");
                while(fgets(line,128,fd)){
                    d++;
                    strcat(msg,line);
                }
            }
            if (d == 0)
                strcat(msg, "\nVous n'avez pas de fichiers partages\n");
        }
    send(client.descr,msg,1024,0);

}
void scanMsgs(char username[], message whp[MAXMSGS], char result[])
{
    int i, c = 0;
    time_t hm;
    bzero(result, BUFSIZ * 2);
    for (i = 0; i < MAXMSGS; i++)
    {
        if (strcmp(whp[i].destination, username) == 0)
        {
            hm = whp[i].time;
            sprintf(result, "%s\n\t%s: \n%s\n%s\n\n", result, whp[i].source, whp[i].msg, ctime(&hm));
            c++;
        }
    }
    if (c == 0)
    {
        bzero(result, BUFSIZ * 2);
        strcpy(result, "none");
    }
}
int handleShowMsgs(char username[],message msgs[],connexion client){
    char result[BUFSIZ*2];
    bzero(result,BUFSIZ*2);
    scanMsgs(username, msgs, result);
    send(client.descr,result,BUFSIZ*2,0);
}
int handleMsg(User users[], connexion client, message msgs[]){
    char dest[32], msg[BUFSIZ*2],res[4];
    bzero(dest,32);
    bzero(msg,BUFSIZ*2);
    bzero(res,4);
    read(client.descr,dest,32);
    read(client.descr,msg,BUFSIZ*2);
    if (getUserIndex(dest, users) >= 0)
    {
        sendMsg(client.username,dest,msg,msgs);
        strcpy(res, "ok");
        send(client.descr, res, 4,0);
        bzero(msg,BUFSIZ*2);
        printf("%s\n",msg);
    }else{
        strcpy(res,"err");
        send(client.descr,res,4,0);
    }
    return 1;
}
int getUserIndex(char username[],User users[]){
    int i;
    for (i = 0; i < MAXUSERS; i++){
        if (strcmp(username, users[i].username) == 0)
            return i;
    }
    return -1;
}
int handleUpload(User users[],connexion client){
    FILE* fd;
    int size;
    char ok[] = "ok", path[64] = "", filename[32] = "",size_str[16],buff[2048 * 5], res[16] = "";

    bzero(buff,strlen(buff));
    bzero(res,strlen(res));
    bzero(filename, strlen(filename));

    read(client.descr,filename,32);
    send(client.descr,ok,strlen(ok),0);
    read(client.descr,size_str,16);

    size = atoi(size_str);
    if(size + users[getUserIndex(client.username,users)].files_size > 2000000){
        strcpy(res,"no_place");
        send(client.descr, res, strlen(res), 0);
        return -1;
    }

    send(client.descr,ok,strlen(ok),0);
    users[getUserIndex(client.username, users)].files_size += size;
    bzero(path, strlen(path));
    sprintf(path,"users\\%s\\%s",client.username,filename);
    fd = fopen(path, "w");
    if(fd == NULL){
        printf("Error on making the file %s\n",path);
        return 0;
    }

    strcpy(res,"ok\n");
    read(client.descr, buff, 2048 * 5);
    send(client.descr, res, 16, 0);

    while(strcmp(buff,"u_done")){
        fputs(buff, fd);
        read(client.descr, buff, 2048 * 5);
        send(client.descr, res, 16, 0);
    }
    saveFileStruct(client.username,filename,users);
    bzero(buff, strlen(buff));
    bzero(res, strlen(res));
    bzero(filename, strlen(filename));

    fclose(fd);
}

void writeLog(char *cont,char username[]){
    FILE *newfile = NULL;
    time_t rawtime;
    struct tm * timeinfo;
    newfile = fopen("log.txt", "a");
    if (newfile != NULL){
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );
        fprintf(newfile,"%s %s: %s\n",asctime(timeinfo),username,cont);
        fclose(newfile);
        newfile = NULL;
    }else{
        printf("Error writing into log.txt\n");
    }
}

int createFolder(char *name)
{
    char path[256];
    FILE *fd;
    sprintf(path, "users/%s", name);
    CreateDirectory("users", NULL);
    if (CreateDirectory(path, NULL))
    {
        sprintf(path, "users/%s/sharedFiles.txt", name);
        fd = fopen(path, "w");
        fclose(fd);
        return 1;
    }
    return -1;
}
int deleteFolder(char *name)
{
    if (RemoveDirectory(name))
        return 1;
    return -1;
}
void showFileContent(char *fileName)
{
    char ch;
    FILE *fp;
    fp = fopen(fileName, "r"); // read mode

    if (fp == NULL)
    {
        printf("Error while opening the file %s\n", fileName);
        exit(EXIT_FAILURE);
    }
    while ((ch = fgetc(fp)) != EOF)
        printf("%c", ch);
    printf("\n");
    fclose(fp);
}
