//WALID MOUDDEN -client.c
/* =============================================================================== */
/*                                                                                 */
/* client.c : Exercice synthese                                        */
/*                                                                                 */
/* =============================================================================== */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>

#define CHECK(sts,msg) if ((sts) == -1 )  { perror(msg);exit(-1);}
#define TAILLE_MESSAGE 256


/* ====================================================================== */
/*                                                                        */
/* Fonction executée pour traiter les signaux                             */
/*                                                                        */
/* ====================================================================== */
void deroute(int sig_num){

	switch(sig_num){
		case SIGUSR1:
			printf("Connexion réussi au serveur \n");
			sleep(1);
			break;
		default: 	
			break;
	}
}

/* ====================================================================== */
/*                                                                        */
/* Fonction du thread pour récuperer les msg envoyé par le serveur        */
/*                                                                        */
/* ====================================================================== */
void * myThreadFunction(void *arg)
{
	char messagerecu[TAILLE_MESSAGE];
	while(1)
	{
		read((int)arg,  messagerecu, TAILLE_MESSAGE);
		if(strlen(messagerecu)>0)
		{
			printf("Message recu du serveur : %s \n", messagerecu);
		}
	}

    pthread_exit(0);
}

/* ====================================================================== */
/*                                                                        */
/* main						                          */
/*                                                                        */
/* ====================================================================== */
int main(int argc, char **argv)
{
	/* déclaration des variables */
	key_t cleSegment; /* Clef du segment de mémoire partagée */
	int shmId; /* Identifiant de la zone de mémoire partagée */
	int erreur = 0;
	char *pidServeur;
	int ret;
	struct sigaction newact;
	char nomTube[] = "walid.fifo"; // tube ecriture des clients
	int entreeTube;
	char messageClient[TAILLE_MESSAGE];
	char nomTube2[TAILLE_MESSAGE]; // tube en lecture pour le serveur
	int sortieTube;
	pthread_t th;

	/* Partie concernant le traitement du signal envoyé par le serveur */
	newact.sa_handler = deroute;
	newact.sa_flags = 0;
	CHECK(sigemptyset(&newact.sa_mask),"erreur init mask \n");
	CHECK(sigaction(SIGUSR1, &newact, NULL),"prb sigaction \n");

	/* Partie concernant la mémoire partagée */
	cleSegment = ftok(argv[1], 1);	
	if (cleSegment == - 1)
	{
		erreur = errno;
		printf("Erreur de création de clef %s\n",strerror(erreur));
		exit(-1);
	}

	shmId = shmget(cleSegment, 100 * sizeof(char),0666);
	if (shmId == -1)
	{
		erreur = errno;
		printf("Erreur de création du segment de mémoire partagée %s\n",strerror(erreur));
		exit(-1);		
	}
	pidServeur = (char *)shmat(shmId, NULL, 0);
	if (pidServeur ==(char *) -1)
	{
		printf("CLIENT - Impossible de s'attacher au segment de mémoire %d \n",shmId);
		exit(-1);
	}
	printf("demande de connexion en cours .... \n");
	CHECK(kill(atoi(pidServeur), SIGUSR1),"erreur envoie signal");
	
	// création du tube pour ecrire des messages au serveur
	if(mkfifo(nomTube, 0666) != 0)
	{

		if (errno == EEXIST)
		{   
        		printf("CLIENT - Tube déjà existant \n");
    		}
	}

	if ((entreeTube = open(nomTube,O_RDWR)) == -1)
	{
		printf("CLIENT - Impossible d'ouvrir l'entrée du FIFO \n");
		exit(EXIT_FAILURE);
	}
	else
	{
		printf("CLIENT - Ouverture du tube écriture OK  \n");
	}
	sleep(1);
	sprintf(nomTube2, "%d", getpid());
	// ouvrir le fichier du tube 2 pour lire les messages du serveur
	if ((sortieTube = open(nomTube2, O_RDONLY)) == -1) {
		printf("Impossible d'ouvrir le tube en lecture \n");
		exit(EXIT_FAILURE);
	}
	else
	{
		printf("CLIENT  - Ouverture du tube lecture OK \n");
	}
	// on passe le descripteur du tube 2 en argument du Thread 
	 pthread_create(&th, NULL, myThreadFunction, (void *)sortieTube);

	// tant que le client n'a pas saisi AU REVOIR, le programme client ne va pas s'arreter
	while(strncmp(messageClient,"AU REVOIR",9) != 0)
	{	
		printf("Votre message ? \n");
		fgets(messageClient,256,stdin);
		write(entreeTube,messageClient,TAILLE_MESSAGE);
		printf("Message envoye au serveur \n");
	}
	CHECK(kill(atoi(pidServeur), SIGUSR2),"erreur envoie signal");
	ret = shmdt(pidServeur);
	if(ret == -1)
	{
		erreur = errno;
		printf("Erreur lors du détachement de la mémoire %s\n",strerror(erreur));
		exit(-1);
	}
	close(entreeTube);
	close(sortieTube);
	printf("CLIENT - Fin de programme \n");
}
