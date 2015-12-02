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
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_CMD		20
#define BUFF_SIZE	100

int main(int argc, char *argv[])
{
	/*	Variables de Conexión	*/
	int		port		=	15001;
	int		sock_srv, nuevofd;
	struct sockaddr_in	server_sock;
	struct sockaddr_in	client_info;

	/*	Variables de control	*/
	pid_t 	child_pid	=	0;
	int		ctrl		=	0;

	/*	Variables de Datos	*/


	/* Inicio del Programa */
	printf("Inicializando... ");

	/*	Configuración del Socket	*/
	sock_srv = socket(PF_INET,SOCK_STREAM,0);
	if(sock_srv == -1)	/*	Error de socket	*/
	{
		printf(">>ERROR: No se pudo abrir el socket.\n\n");
		return -1;
	}

	server_sock.sin_family		= AF_INET;
	server_sock.sin_port		= htons(port);
	server_sock.sin_addr.s_addr	= 0;

	printf("Listo\n");

	int yes = 1;
	setsockopt(sock_srv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	ctrl = bind(sock_srv, (struct sockaddr *)&server_sock, sizeof(struct sockaddr_in));
	if(ctrl < 0)	/*	Error de bind	*/
	{
		printf(">>ERROR: No se pudo enlazar el socket.\n\n");
		close(sock_srv);
		return -1;
	}

	ctrl = listen(sock_srv,1);
	if(ctrl < 0)	/*	Error de Listen	*/
	{
		printf(">>ERROR: Fallo el listen.\n\n");
		close(sock_srv);
		return -1;
	}

	printf(">Esperando conexiones...\n");

	child_pid = fork();
	if(child_pid == 0)
	{	/* Proceso hijo */
		/*	Acepto al conexión	y cierro el socket de server	*/
		unsigned int client_len = sizeof(struct sockaddr_in);
		char clientIP[INET6_ADDRSTRLEN];

		/*	Comandos a ejecutar	*/
		char* argls[MAX_CMD] = {"","","","","","","",""};

		nuevofd = accept(sock_srv,(struct sockaddr *)&client_info, &client_len);
		if(nuevofd<0)	/*	Error de Acceptar	*/
		{
			printf(">>ERROR: Fallo al aceptar la conexión.\n\n");
			close(sock_srv);
			return -1;
		}

		close(sock_srv);

		printf("\n  Nueva conexión desde %s asignada a socket %d\n"
				,inet_ntop(AF_INET,&(client_info.sin_addr)
				, clientIP, INET_ADDRSTRLEN),nuevofd);

		printf("  Obteniendo comandos... ");
		char buff[BUFF_SIZE] = "";

		ctrl = recv(nuevofd,buff,100,0);
		printf("%s\n\n", buff);

		/*	Desarmo la cadena obtenido en el comando y sus argumentos.	*/
		int i = 0;
		argls[0] = strtok(buff, " ");
		while (argls[i] != NULL)
		{
		    i++;
		    argls[i] = strtok (NULL, " \n");
		}

		/*	Señalo la correcta recepción	*/
		write(nuevofd,"Listo",(strlen("Listo")+1));

		if(fork()!=0)
		{	/* Nuevo proceso padre */


			wait(NULL);
			printf("  Ejecución de comando finalizada.\n");
			//send(nuevofd,"CMD_DONE",8,0);
			close(nuevofd);
			return 0;
		}
		else
		{
			close(1);
			dup(nuevofd);

			execvp(argls[0],argls);
			return 0;
		}
	}
	else
	{	/*Proceso Padre */
		//fflush(0);
		waitpid(child_pid,NULL,0);
		printf(">Termino el proceso hijo: %d\n",child_pid);
		close(sock_srv);
		return 0;
	}
}
