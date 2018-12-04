#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include "libClient.h"

void test(int sClient){
  write(sClient,"test",4);
}

void user_pw(int sclient)
{
    char login[10],password[10];
   char msg[10];
    int i=0;

do{
        if(i==3){
            printf ("Vous avez depassez le nombre d'essai");
            exit(1);
        }
        i++;
  memset(&login,0,sizeof(login));

    printf("\t\t\t Nom d'utilisateur: ");
    gets(login);
    write(sclient,login,strlen(login));

  memset(&password,0,sizeof(password));


    printf("\t\t\t Mot de passe: ");
    gets(password);
    write(sclient,password,strlen(password));
    memset(&msg,0,sizeof(msg));
    read(sclient,msg,sizeof(msg));
    if(strcmp(msg,"ok")==0){
        printf("\n\t\t\t BIENVENUE AU SERVICE CLIENT!\n\n");
        break;
    }
    else
        close(sclient);

}while(1);


}
void supp(int sclient){
    char name[32], res[16];
    bzero(res, 16);
    bzero(name, 32);
    printf("Entrez nom de fichier: \n");
    printf("> ");scanf("%s",name);
    write(sclient, name, 32);
    read(sclient,res,16);
    if(strcmp(res,"ok") == 0)
        printf("Le fichier est supprime.\n");
    else
        printf("Fichier inrouvable!\n");
    bzero(res,16);
    bzero(name,32);
}

int part(int sclient){
    char filename[16],shareWith[16],res[16];

    bzero(filename,16);
    bzero(res,16);
    bzero(shareWith,16);

    printf("Entrez le nom du fichier: \n> ");
    scanf("%s",filename);

    write(sclient,filename,16);
    read(sclient,res,16);
    printf("Response: %s\n",res);
    if(strcmp(res,"ok")){
        printf("Fichier introuvable!\n");
        return -1;
    }

    printf("Entrez le nom d'utilisateur:\n> ");
    scanf("%s",shareWith);

    write(sclient, shareWith, 16);
    read(sclient, res, 16);

    if (strcmp(res, "ok"))
    {
        printf("Utilisateur introuvable!\n");
        return -1;
    }

    printf("Fichier %s est partage avec %s\n",filename,shareWith);
}




void email(int sclient){
    char login[30],msg[BUFSIZ*2],res[4];
    bzero(res,4);
    memset(&login,0,sizeof(login));
    printf("Entrez le nom de destinataire: \n");
    scanf("%s",login);
    getchar();
    write(sclient,login,strlen(login));
    memset(&msg,0,sizeof(msg));
    printf("Entrez votre message: \n> ");
    gets(msg);
    write(sclient,msg,strlen(msg));
    printf("Msg: %s\n",msg);
    read(sclient,res,4);
    if(strcmp(res,"ok") == 0)
        printf("Message envoye!\n");
    else
        printf("Utilisateur introuvable!\n");
}

void lister_fichier(int sclient)
{
    char msg[1024];
    bzero(msg,1024);
    read(sclient,msg,1024);
    printf("%s\n",msg);
    bzero(msg,1024);
}


int telecharger(int sclient){
    char filename[32],path[64];
    char owner[32];
    char res[16];
    FILE *fd;
    char buff[1024 * 5];

    bzero(filename,32);
    bzero(path,64);
    bzero(owner,32);
    bzero(buff, 1024 * 5);
    bzero(res,16);
    printf("\nEntrez le nom du fichier a telecharger :\n");
    printf("> ");
    scanf("%s",filename);

    write(sclient,filename,32);
    read(sclient,res,16);

    printf("\nEntrez le nom du proprietaire du fichier (Entrez 'moi' si c'est votre fichier):\n");
    printf("> ");
    scanf("%s",owner);

    write(sclient,owner,32);
    read(sclient,res,16);
    if(!strcmp(res,"permission")){
        printf("T'as pas la permission d'acceder ce fichier!\n");
        return -1;
    }else if (!strcmp(res, "notfound"))
    {
        printf("Fichier Introuvable!\n");
        return -1;
    }

   h sprintf(path,"download\\%s",filename);
    fd=fopen(path,"wb");

    if (fd == NULL){
        perror("Erreur lors de la lecture du ficher");
        exit (3);
    }
    read(sclient, buff, 1024 * 5);
    while(strcmp(buff,"d_done")){
        fputs(buff,fd);
        send(sclient,res,16,0);
        read(sclient, buff, 1024 * 5);
    }
    printf("\n\nFichier telecharger!\n");
    fclose(fd);
}

int upload(int sclient){
    FILE *sa;
    char path_fichier[128], nom_fichier[32], file_size[50], buff[2048 * 5], res[16];
    int i,N,R,size;

    bzero(nom_fichier,strlen(nom_fichier));
    bzero(path_fichier,strlen(path_fichier));
    bzero(file_size,strlen(file_size));
    bzero(buff,strlen(buff));
    bzero(res,strlen(res));

    printf("\nDonnez le nom de votre fichier: \n> ");
    scanf("%s", nom_fichier);
    write(sclient, nom_fichier, strlen(nom_fichier));
    read(sclient, res, 16);

    sprintf(path_fichier,"upload\\%s",nom_fichier);
    sa = fopen(path_fichier, "r");
    if (sa == NULL)
    {
        printf("ERREUR D'OUVERTURE DU FICHIER\n");
        return -1;
    }

    fseek(sa, 0L, SEEK_END);
    size = ftell(sa);
    fseek(sa,0L,SEEK_SET);

    sprintf(file_size, "%d", size);
    sleep(1);
    write(sclient, file_size, strlen(file_size));
    bzero(res,16);
    read(sclient, res, 16);

    if(!strcmp(res,"no_place")){
        printf("Vous avez atteint votre quota de taille!\n");
        return -1;
    }
    printf("Le fichier est en cours de chargement..\n");
    while (fgets(buff, 2048 * 5, sa))
    {
        send(sclient, buff, 2048 * 5, 0);
        read(sclient,res,16);
    }
    strcpy(buff,"u_done");
    send(sclient, buff, 2048 * 5, 0);
    printf("\nFichier uploade\n");

    bzero(nom_fichier, strlen(nom_fichier));
    bzero(path_fichier, strlen(path_fichier));
    bzero(file_size, strlen(file_size));
    bzero(buff, strlen(buff));
    bzero(res, strlen(res));

    return 0;
}

void showMenu(){
    char ch;
    FILE *fp;
    fp = fopen("menu.txt", "r");

    if (fp == NULL){
       printf("Erreur lors d'affichage du menu !!%s\n");
       exit(EXIT_FAILURE);
    }
    while((ch = fgetc(fp)) != EOF)
       printf("%c", ch);
    printf("\n");
    fclose(fp);
}
int showMsgs(int sclient){
    char result[BUFSIZ*2];
    bzero(result,BUFSIZ*2);
    recv(sclient,result,BUFSIZ*2,0);
    if(strcmp(result,"none")){
        printf("\t Mes messages:\n");
        printf("\t ------------ \n");
        printf("%s\n", result);
    }else
        printf("T'as pas des messages..\n\n");
}
