/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"

void display_cmd(struct cmdline *l){
	/* Display each command of the pipe */
		for (int i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			printf("seq[%d]: ", i);
			for (int j=0; cmd[j]!=0; j++) {
				printf("%s ", cmd[j]);
			}
			printf("\n");
		}
}

//Fondtion d'execution de commandes
void execute_cmd (char **args){
	if (execvp(args[0],args) < 0){ //Tester si la commande est inexistante et afficher une erreur si oui
		fprintf(stderr,"%s: command not found\n",args[0]);
		exit(errno);
	}

}

//Gerer les redirections E/S

int redirection_E (char *file_in, int default_E){
	int fdin;
	if (file_in){
			fdin = open(file_in,O_RDONLY);
			if (fdin ==-1){
				fprintf(stderr,"%s: No such file or directory\n",file_in);
			}
	}
	else
		fdin = dup(default_E);
	return fdin;
}

int redirection_S (char *file_out,int default_S){
	int fdout;
	if (file_out){
		if ((fdout = creat(file_out , 0644)) < 0) {
			perror("Couldn't open the output file");
		}      
	}
	else
		fdout = dup(default_S);
	return fdout;
}

int main()
{	
	struct cmdline *l;
	while (1) {
		
		printf("shell> ");
		l = readcmd();

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}

		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}
		//Cas ou la sequence de commande est vide
		if (l->seq[0] == NULL){
			continue;
		}
		//Si la commande tapé par l'utilisateur est "exit" alors on quitte le terminal (du mini-shell)
		if (strcmp(l->seq[0][0],"quit")==0){
			exit(0);
		}

		//Gerer quelques commandes internes (built_in) du shell

		if (strcmp(l->seq[0][0],"cd")==0){ //le cas ou la commande tapé est "cd"
			if (!l->seq[0][1])
				chdir(getenv("HOME"));
			else if (chdir(l->seq[0][1])==-1)
			       fprintf(stderr,"%s: No such file or directory\n",l->seq[0][1]);
			continue;
		}
		//Gerer le cas des commandes externes du shell
		else{	
				// Sauvegarder l'entrée et la sortie standard resp. dans tmpE et tmpS ( des decripteurs de fichiers temporaires )
				int tmpE = dup(0), tmpS = dup(1); 
				
				int fdin;//Creation du descripteur de fichier d'entré ( < )
				fdin = redirection_E(l->in,tmpE);
				
				pid_t ret; // variable pour la sauvegarde du retour de notre Fork()
				int fdout; //creation du descripteur de fichier de sortie ( > )

				for (int i=0; l->seq[i]!=0; i++) {
					dup2(fdin,0);//Rediriger l'entrée standart 
					close(fdin);
					//Si nous sommes a la derniere commande alors nous verifions si nous avons une redirections de sortie
					if (l->seq[i+1] == 0){
						fdout = redirection_S(l->out,tmpS);
					}//Sinon l'entrée et la sortie sont passés dans des tubes d'ou la creation de tubes
					else{ 
						int tube[2];
						if ( pipe(tube) < 0 ){
							fprintf(stderr,"erreur dans pipe\n");
							exit(errno);
						}
						fdout=tube[1];
						fdin=tube[0];
					}
					dup2(fdout,1);//Rediriger la sortie standart
					close(fdout);

					ret = Fork(); // Creation du processus 
					if (ret == 0){ // Executer la commande avec "execv" si c'est le fils
						if (fdout != -1 && fdin != -1)//Si l'ouverture ou la creation de notre fichier de sortie ou d'entrée se passe mal on execute pas la commande 
							execute_cmd(l->seq[i]);
					}
					else{ // Attendre la terminaison du fils Sinon
						Wait(NULL);
					}
				}
				//Restorer l'entrée et la sortie standart de base du shell
				dup2(tmpE,0);
				dup2(tmpS,1);
				close(tmpE);
				close(tmpS);
						
			}
	}
}
