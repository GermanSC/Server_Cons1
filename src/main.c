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
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_CMD	20

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
//	char* arg_list[MAX_CMD] = {NULL};

	/* Inicio del Programa */
	printf("Inicializando...\n");

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

	child_pid = fork();
	if(child_pid == 0)
	{	/* Proceso hijo */
		/*	Acepto al conexión	y cierro el socket de server	*/
		unsigned int client_len = sizeof(struct sockaddr_in);
		char clientIP[INET6_ADDRSTRLEN];

		nuevofd = accept(sock_srv,(struct sockaddr *)&client_info, &client_len);
		close(sock_srv);

		printf("Nueva conexión desde %s asignada a socket %d\n"
				,inet_ntop(AF_INET,&(client_info.sin_addr)
				, clientIP, INET_ADDRSTRLEN),nuevofd);

		write(nuevofd,"Hola!",6);
		close(nuevofd);

		//execvp(arg_list[0], arg_list);
		return 0;
	}
	else
	{	/*Proceso Padre */
		waitpid(child_pid,NULL,0);
		printf("Termino mi Hijo: %d\n",child_pid);

		close(sock_srv);
		return 0;
	}
}
