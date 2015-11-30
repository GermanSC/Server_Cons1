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

	/* Arrancamos */
	printf("Arrancamos por hacer el Ejecutor de Comandos\n");

	child_pid = fork();
	if(child_pid == 0)
	{
		/* Proceso hijo */
		sleep(10);
		return 0;
	}
	else
	{
		/*Proceso Padre */
		waitpid(child_pid,NULL,0);
		return 0;
	}
}
