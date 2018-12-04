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
#include <fcntl.h>
#include <sys/uio.h>
#include "libClient.h"

#define ERR (-1)
#define TAILLEMESSAGE 1025
#define PORT 5555
#define IPADRESSE "127.0.0.1"

void aff_addr(struct sockaddr_in addr){
   printf("\nIP : %s, port : %d\n",(char *)inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
}


void afficheErrorSortie(char *nomFct) {
     printf("\n %s -> %s ", nomFct, strerror(errno));
     //exit(1);
}


int main(int argc, char const *argv[]){
    int sClient,ret_fct;
    char msg[256];
    char msgwlc[256];
    struct sockaddr_in  addrServeur;
    /* Initialisation du timeval � 0,5 sec */
    struct timeval tmp={0, 500000};
    char ops[9][16] = {"upload","download","delete","list","share","message","my_messages","quit","none"};
    /* Pour g�rer la liste des descripteurs � surveiller */
    fd_set ensemble;
    sClient=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (sClient==ERR){ afficheErrorSortie("socket()");  exit(1);}
    addrServeur.sin_family = AF_INET;
    addrServeur.sin_port   = htons(PORT);
    addrServeur.sin_addr.s_addr=inet_addr(IPADRESSE);

    ret_fct=connect(sClient,(struct sockaddr*)&addrServeur,sizeof(struct sockaddr));
    if(ret_fct==ERR){
        afficheErrorSortie("connect()");
        exit(1);
    }

    printf("\nConnexion ...");
    aff_addr(addrServeur);
    while(1){
        user_pw(sClient);
        break;
    }
    while(1){
        FD_ZERO(&ensemble);
        FD_SET(sClient,&ensemble);

        ret_fct = select(sClient+1,&ensemble,NULL,NULL,&tmp);
        if(ret_fct==ERR)afficheErrorSortie("select()");
        else{
            int n;
            showMenu();
            printf("Entrez votre choix: \n");
            printf("> ");scanf("%d",&n);

            if (n < 9)
            {
                write(sClient, ops[n - 1], 16);
            }
            else
                write(sClient, ops[9], 16);
            switch (n){
                case 1 :
                    upload(sClient);
                    break;
                case 2 :
                    telecharger(sClient);
                    break;
                case 3 :
                    supp(sClient);
                    break;
                case 4 :
                    lister_fichier(sClient);
                    break;
                case 5 :
                    part(sClient) ;
                    break;
                case 6 :
                    email(sClient);
                    break;
                case 7 :
                    showMsgs(sClient);
                    break;
                case 8 :
                    exit(1);
            }
            n = 0;
        }
    }
    close(sClient);
    return 0;
}
