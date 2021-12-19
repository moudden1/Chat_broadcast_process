c=gcc

all : client.c serveur.c
	$(c) serveur.c -o serveur
	$(c) -o client client.c -lpthread
client :
	$(c) -o client client.c -lpthread
serveur :
	$(c) serveur.c -o serveur
clean : 
	rm serveur client
