//WALID MOUDDEN -serveur.c
/* =============================================================================== */
/*                                                                                 */
/* serveur.c : Exercice synthese                                        */
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
#include <sys/shm.h>

#define CHECK(sts,msg) if ((sts) == -1 )  { perror(msg);exit(-1);}
#define TAILLE_MESSAGE 256
#define NB_MAX_CLIENTS 100

/* variables globales */
int utilisateurs = 0;
int nbUsers = 0; //pour la fin du serveur
int entreeTubeClients[NB_MAX_CLIENTS];

/* ====================================================================== */
/*                                                                        */
/* Fonction executée pour traiter les signaux                             */
/*                                                                        */
/* ====================================================================== */
void deroute(int sig_num, siginfo_t *info, void * context){
	int pidClient;
	switch(sig_num){
		case SIGUSR1:
			pidClient = info->si_pid;
			char tubeClient[TAILLE_MESSAGE]; 
			sprintf(tubeClient,"%d",pidClient);
			printf("Nouveau client avec le pid %d \n",pidClient);
			CHECK(kill(pidClient, SIGUSR1),"erreur envoie signal au client\n");
			if(mkfifo(tubeClient, 0666) != 0)
			{
			    perror("mkfifo");
			    if (errno == EEXIST)
			    {
				printf("SERVEUR - Tube déjà existant \n");
			    }

			}
			if ((entreeTubeClients[utilisateurs] = open(tubeClient,O_RDWR)) == -1)
			{

				printf("SERVEUR - Impossible d'ouvrir l'entrée du FIFO \n");
				exit(EXIT_FAILURE);
			}
			else
			{
				
        			printf("SERVEUR - Ouverture du tube pour le client OK  \n");
			}	
			nbUsers++;			
			utilisateurs++;
			break;
		case SIGUSR2:
			nbUsers--;
			break;
		default: 
			break;
	}
}

/* ====================================================================== */
/*                                                                        */
/* main						                         */
/*                                                                        */
/* ====================================================================== */

int main(int argc, char **argv)
{
	/* déclaration des variables */
	struct sigaction newact;
	FILE *fic = NULL;
	key_t cleSegment; /* Clef du segment de mémoire partagée */
	int erreur = 0;
	int shmId; /* Identifiant de la zone de mémoire partagée */
	char *pidServeur;
	int ret; // verifier le retour des fonctions
	char nomTube[] = "walid.fifo"; // tube pour lecture des clients
	int sortieTube;
	char messageClient[TAILLE_MESSAGE];

	/* Partie concernant le traitement du signal envoyé par les Clients */
	newact.sa_sigaction = deroute;
	newact.sa_flags = SA_SIGINFO;
	CHECK(sigemptyset(&newact.sa_mask),"erreur init mask \n");
	CHECK(sigaction(SIGUSR1, &newact, NULL),"prb sigaction \n");
	CHECK(sigaction(SIGUSR2, &newact, NULL),"prb sigaction \n");

	/* Partie concernant la mémoire partagée */
	printf("Création de la clé en cours ... \n");	
	fic = fopen(argv[1], "w+"); // création du fichier pour la mémoire partagé 
	cleSegment = ftok(argv[1], 1);
	if (cleSegment == - 1)
	{
		erreur = errno;
		printf("Erreur de création de clef %s\n",strerror(erreur));
		exit(-1);
	}
	printf("Clé crée !\n");

	shmId = shmget(cleSegment, 100 * sizeof(char),0666 | IPC_CREAT | IPC_EXCL);
	if (shmId == -1)
	{
		erreur = errno;
		printf("Erreur de création du segment de mémoire partagée %s\n",strerror(erreur));
		exit(-1);	
	}

	pidServeur = shmat(shmId, NULL, 0);
	if (pidServeur ==(char *) -1)
	{
		printf("Impossible de s'attacher au segment de mémoire %d \n",shmId);
		exit(-1);
	}
	sprintf(pidServeur, "%d", getpid());// ecrire dans la memoire le pid du serveur

	printf("mon PID dispo dans la mémoire partagé ! \n");

	ret = shmdt(pidServeur);
	if(ret == -1)
	{
		erreur = errno;
		printf("Erreur lors du détachement de la mémoire %s\n",strerror(erreur));
		exit(-1);
	}

	//on attend jusqu'a l'arrivée d'au moins un client
	while(utilisateurs == 0);

	sleep(2);

	if ((sortieTube = open(nomTube, O_RDONLY)) == -1) {
		printf("Impossible d'ouvrir le tube en lecture \n");
		exit(EXIT_FAILURE);
	}
	else
	{
		printf("SERVEUR - Ouverture du FIFO OK \n");
	}	
	// tant que ya des clients, le programme serveur ne va pas s'arreter
	while(nbUsers >0)
	{
		read(sortieTube,  messageClient, TAILLE_MESSAGE);
		if(strlen(messageClient) > 0)
		{
			printf("Message recu du client : %s \n", messageClient);
			for(int k=0; k<utilisateurs; k++)
			{
				// ecrire dans les tubes ecriture de serveurs vers client
				write(entreeTubeClients[k],messageClient,TAILLE_MESSAGE);
			}
		}
	}
	close(sortieTube);
	for( int x=0; x<utilisateurs; x++)
	{
		close(entreeTubeClients[x]);
	}
	// suppression de la mémoire partagé 
	ret=shmctl(shmId, IPC_RMID, NULL);
	if(ret == -1)
	{
		erreur = errno;
		printf("Erreur lors du suppression de la mémoire %s\n",strerror(erreur));
		exit(-1);
	}
	// fermeture du fichier du tube
	fclose(fic);
	printf("SERVEUR - Fin de programme \n");
	return 0;
}
