/*
 ============================================================================
 Name        : main.c
 Project	 : Server_Con1
 Author      : German Sc.
 Version     : 0.0
 Copyright   : Completamente copyrighteado 2015
 Description : Nada.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


int main(void)
{
	pid_t child_pid;
	char* arg_list[] = { "ls","-l", NULL };

	/* Arrancamos */
	printf("Arrancamos por hacer el Ejecutor de Comandos\n");

	child_pid = fork();
	if(child_pid == 0)
	{
		/* Proceso hijo */
		execvp("ls", arg_list);
		return 0;
	}
	else
	{
		/*Proceso Padre */
		waitpid(child_pid,NULL,0);
		printf("Termino mi Hijo: %d\n",child_pid);
		return 0;
	}
}
