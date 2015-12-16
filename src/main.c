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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_CMD		20
#define BUFF_SIZE	100

int esDeamon = 0;
int verb = 0;

/*	TODO
 *
 * Arreglar Deamon.
 * Multiples conexiones.
 * Modulizar
 *
 */

int printHelp(char * str)
{
	printf("\nUso: %s [opciones]\n\n", str);
	printf("  -d	Modo daemon.\n"
			"  -v	Verbose.\n\n");
	printf( "El programa servidor atiende conexiones en el puerto TCP 15001 "
			"esperando conexiones de procesos clientes. El servidor puede "
			"ejecutarse en modo normal ­en el cual informa a través de la"
			"consola los eventos, conexiones, errores, etc­ o en modo demonio"
			"­en el cual se desconecta de la consola y presenta errores y"
			"mensajes a través de syslog.\n\n");
	return 0;
}

//void print_Donde(int D_Flag, int PRI, const char *formatString, ...)
//{
//	static char buffer[100];
//
//	va_list arguments;
//
//	va_start(arguments, formatString);
//
//	vsnprintf(buffer, 100, formatString, arguments);
//	buffer[99] = 0;
//
//
//	if (D_Flag)
//	{
//		syslog(PRI,"%s", buffer);
//	}
//	else
//	{
//		printf("%s", buffer);
//	}
//
//	va_end(arguments);
//}

void printDonde(char * str, int D_Flag, int PRI)
{
	if (D_Flag != 0)
	{
		syslog(PRI, "%s", str);

	} else {

		printf("%s", str );
	}
}

int socketSetUp(int port){

	/*	Variables Generales	*/
	int temp = -1;


	/*	Variables de Conexión	*/
	int		sock;
	struct sockaddr_in	server_sock;
	int yes = 1;

	/*	Configuración del Socket	*/
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)	/*	Error de socket	*/
	{
		return -1;
	}

	server_sock.sin_family		= AF_INET;
	server_sock.sin_port		= htons(port);
	server_sock.sin_addr.s_addr	= 0;


	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	temp = bind(sock, (struct sockaddr *)&server_sock, sizeof(struct sockaddr_in));
	if(temp < 0)	/*	Error de bind	*/
	{
		close(sock);
		return -1;
	}

	return sock;
}

int acceptConnection(int temp_srv){

	int temp_sock = -1;
	struct sockaddr_in	client_info;
	unsigned int client_len = sizeof(struct sockaddr_in);
	char clientIP[INET6_ADDRSTRLEN];

	temp_sock = accept(temp_srv, (struct sockaddr *)&client_info, &client_len);
	if(temp_sock < 0)	/*	Error de Acceptar	*/
	{
		printDonde(">>ERROR: Fallo al aceptar la conexión.\n\n", esDeamon, LOG_ERR);
		return -1;
	}

	if(!esDeamon)
	{
		printf("\n  Nueva conexión desde %s asignada a socket %d\n",
				inet_ntop(AF_INET, &(client_info.sin_addr), clientIP, INET_ADDRSTRLEN),
				temp_sock);
	} else {
		syslog(LOG_NOTICE,  "\n  Nueva conexión desde %s asignada a socket %d\n"
				,inet_ntop(AF_INET,&(client_info.sin_addr)
						, clientIP, INET_ADDRSTRLEN), temp_sock);
	}

	return temp_sock;
}

int pipeSetUp(int* pipe_in, int* pipe_out,int* pipe_err)
{
	int temp = -1;

	temp = pipe(pipe_in);
	if(temp == -1)	/*	Error de pipe	*/
	{
		return -1;
	}

	temp = pipe(pipe_out);

	if(temp == -1)		/*	Error de pipe	*/
	{
		return -1;
	}

	temp = pipe(pipe_err);
	if(temp == -1)		/*	Error de pipe	*/
	{
		return -1;
	}
	return 0;
}

int runCommand(int* pipe_in, int* pipe_out, int* pipe_err, char ** list)
{
	close(pipe_in[1]);
	close(pipe_out[0]);
	close(pipe_err[0]);

	dup2(pipe_in[0], STDIN_FILENO);
	dup2(pipe_out[1], STDOUT_FILENO);
	dup2(pipe_err[1], STDERR_FILENO);

	execvp(list[0], list);
	return 0;
}

int main(int argc, char *argv[])
{
	/*	Variables de Conexion	*/
	int sock_srv, nuevofd;

	/*	Variables de control	*/
	pid_t child_pid	=	0;
	int ctrl		=	0;
	int tmp_i		=	0;

	/*	Configuracióon de opciones	*/

	int opt_sig;
	const char* const opc_cort = "hdv";

	/*	Buffers	*/

	char buff[BUFF_SIZE] = "";

	/*	Pipes	*/

	int stdin_p[2];
	int stdout_p[2];
	int stderr_p[2];

	/*	Inicio	*/

	do
	{
		opt_sig = getopt (argc, argv, opc_cort);

		switch (opt_sig)
		{
		case 'h':
			printHelp(argv[0]);
			return 0;
			break;
		case 'd':
			esDeamon = 1;
			break;
		case 'v':
			verb = 1;
			break;
		case '?':
			printHelp(argv[0]);
			return 0;
			break;

		case -1:
			break;

		default:
			abort();
		}
	} while (opt_sig != -1);

	/*	Configuración del Log	*/
	if(esDeamon)
	{
		openlog("RMT_CMD_Server", LOG_PID, LOG_USER);
	}

	/* Inicio del Programa */

	printDonde(">Esperando conexiones...\n", esDeamon, LOG_NOTICE);

	sock_srv = socketSetUp(15001);
	if(sock_srv < 0)
	{
		return -1;
	}

	while(1)
	{
		ctrl = listen(sock_srv, 5);
		if(ctrl < 0)	/*	Error de Listen	*/
		{
			printDonde(">>ERROR: Fallo el listen.\n\n", esDeamon, LOG_ERR);
			close(sock_srv);
			return -1;
		}

		child_pid = fork();
		if(child_pid == 0)
		{
			/*	Primer hijo	*/
			char * argls[MAX_CMD] = {NULL};

			nuevofd = acceptConnection(sock_srv);
			close(sock_srv);
			if (nuevofd < 0)	/*	Error de Accept	*/
			{
				printf("Error al aceptar la conexion.\n");
				return -1;
			}

			printDonde("  Obteniendo comandos... ", esDeamon, LOG_NOTICE);

			ctrl = recv(nuevofd, buff, 100, 0);
			printf("%s\n\n", buff);

			/*	Desarmo la cadena obtenido en el comando y sus argumentos.	*/

			argls[0] = strtok( buff, " ");
			while ( argls[tmp_i] != NULL )
			{
				tmp_i++;
				argls[tmp_i] = strtok( NULL, " \n" );
			}

			/*	Señalo la correcta recepción	*/
			write(nuevofd, "Listo", 6 );

			/*	Seteo de pipes	*/

			ctrl = pipeSetUp(stdin_p, stdout_p, stderr_p);
			if(ctrl == -1)		/*	Error de pipe	*/
			{
				printDonde(">> ERROR: Error al configurar las pipes", esDeamon, LOG_ERR);
				close(nuevofd);
				return -1;
			}

			child_pid = fork(); /* FIXME */
			if(child_pid != 0 )
			{
				close(stdout_p[1]);
				close(stderr_p[1]);
				close(stdin_p[0]);

				char buff[1024] = "";

				/*	Configuro el select	*/
				fd_set readfds;
				FD_ZERO(&readfds);
				FD_SET(nuevofd, &readfds);
				FD_SET(stdout_p[0], &readfds);
				FD_SET(stderr_p[0], &readfds);

				while( waitpid(-1, NULL, WNOHANG) != child_pid )
				{
					FD_SET(nuevofd, &readfds);
					FD_SET(stdout_p[0], &readfds);
					FD_SET(stderr_p[0], &readfds);

					ctrl = select( 11, &readfds, NULL, NULL, NULL );
					if(ctrl == -1)	/*	Select Error	*/
					{
						printDonde("ERROR: En el select.\n", esDeamon, LOG_ERR);
					}

					if( FD_ISSET(stdout_p[0], &readfds) != 0 )
					{
						if( verb != 0 )
						{
							printDonde("out: ", esDeamon, LOG_NOTICE);
						}
						ctrl = read( stdout_p[0], buff, sizeof(buff) );
						if (ctrl != 0)
						{
							write(nuevofd, buff, ctrl);

							if( (esDeamon == 0) && (verb == 1) )
							{
								printf( "%.*s\n", ctrl, buff );

							} else {

								syslog(LOG_NOTICE, "%.*s\n", ctrl, buff);
							}
						}
					}

					if( FD_ISSET(stderr_p[0], &readfds) != 0 )
					{
						ctrl = read(stderr_p[0], buff, sizeof buff);
						if (ctrl != 0)
						{
							write(nuevofd, buff, ctrl);
							if(verb != 0)
							{
								printDonde("err: ", esDeamon, LOG_NOTICE);
							}
							if( (esDeamon == 0) && (verb == 1) )
							{
								printf("%.*s\n", ctrl, buff);
							}
							else
							{
								syslog(LOG_NOTICE, "%.*s\n", ctrl, buff);
							}
						}
					}

					if( FD_ISSET(nuevofd, &readfds) != 0)
					{
						ctrl = read(nuevofd, buff, sizeof(buff));
						if(verb != 0)
						{
							printDonde("cliente:", esDeamon, LOG_NOTICE);
						}
						if(ctrl == 0)
						{
							printDonde("  Conexión con cliente cerrada.\n", esDeamon, LOG_NOTICE);
							break;
						}
						else
						{
							write(stdin_p[1], buff, ctrl);

							if( (esDeamon == 0) && (verb == 1) )
							{
								printf("%.*s\n", ctrl, buff);
							}
							else
							{
								syslog(LOG_NOTICE, "%.*s\n",ctrl,buff);
							}
						}
					}

				}
				if( strcmp(buff, "" ) == 0 )
				{
					write(nuevofd, "ERROR: Comando inválido.\n", 25);
					printDonde("  ERROR: Comando inválido.\n", esDeamon, LOG_NOTICE);
				}
				printDonde("  Ejecución de comando finalizada.\n", esDeamon, LOG_NOTICE);
				close(stdin_p[1]);
				close(stdout_p[0]);
				close(stderr_p[0]);
				close(nuevofd);
				return 0;

			} else {
				/*	Hijo Comando	*/

				close(nuevofd);
				runCommand(stdin_p, stdout_p, stderr_p, argls);

				return 0;
			}

		} else {	/*Proceso Padre */

			waitpid(child_pid, NULL, 0);
			printDonde(">Esperando proximo comando.\n", esDeamon, LOG_NOTICE);

		}
	}

	if(esDeamon)
	{
		closelog();
	}
	close(sock_srv);
	return 0;
}
