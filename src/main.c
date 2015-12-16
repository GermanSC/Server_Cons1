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

/*	TODO
 *
 * Arreglar Deamon.
 * Multiples conexiones.
 * Modulizar
 *
 */

int print_help(char * str)
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

void print_Donde(char * str, int D_Flag, int PRI)
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

int main(int argc, char *argv[])
{
	/*	Variables de Conexion	*/
	int sock_srv, nuevofd;

	/*	Variables de control	*/
	pid_t child_pid	=	0;
	int ctrl		=	0;
	int i			= 	0;

	/*	Configuracióon de opciones	*/

	int opt_sig;
	const char* const opc_cort = "hdv";
	int esDeamon = 0;
	int verb 	 = 0;

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
			print_help(argv[0]);
			return 0;
			break;
		case 'd':
			esDeamon = 1;
			break;
		case 'v':
			verb = 1;
			break;
		case '?':
			print_help(argv[0]);
			return 0;
			break;

		case -1:
			break;

		default:
			abort();
		}
	} while (opt_sig != -1);

	if(esDeamon)
	{
		/*	Configuración del Log	*/
		openlog("RMT_CMD_Server", LOG_PID, LOG_USER);
	}

	/* Inicio del Programa */

	print_Donde(">Esperando conexiones...\n", esDeamon, LOG_NOTICE);

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
			print_Donde(">>ERROR: Fallo el listen.\n\n", esDeamon, LOG_ERR);
			close(sock_srv);
			return -1;
		}

		child_pid = fork();

		if(child_pid == 0)
		{
			/*	Proceso hijo	*/
			/*	Acepto al conexión	y cierro el socket de server	*/
			struct sockaddr_in	client_info;
			unsigned int client_len = sizeof(struct sockaddr_in);
			char clientIP[INET6_ADDRSTRLEN];

			/*	Comandos a ejecutar	*/
			char* argls[MAX_CMD] = {NULL};

			nuevofd = accept(sock_srv, (struct sockaddr *)&client_info, &client_len);
			if(nuevofd<0)	/*	Error de Acceptar	*/
			{
				print_Donde(">>ERROR: Fallo al aceptar la conexión.\n\n", esDeamon, LOG_ERR);
				close(sock_srv);
				return -1;
			}

			close(sock_srv);

			if(!esDeamon)
			{
				printf("\n  Nueva conexión desde %s asignada a socket %d\n",
						inet_ntop(AF_INET, &(client_info.sin_addr), clientIP, INET_ADDRSTRLEN),
						nuevofd);
			} else {
				syslog(LOG_NOTICE,  "\n  Nueva conexión desde %s asignada a socket %d\n"
						,inet_ntop(AF_INET,&(client_info.sin_addr)
								, clientIP, INET_ADDRSTRLEN), nuevofd);
			}

			print_Donde("  Obteniendo comandos... ", esDeamon, LOG_NOTICE);

			ctrl = recv(nuevofd,buff,100,0);
			printf("%s\n\n", buff);

			/*	Desarmo la cadena obtenido en el comando y sus argumentos.	*/

			argls[0] = strtok( buff, " ");
			while ( argls[i] != NULL )
			{
				i++;
				argls[i] = strtok( NULL, " \n" );
			}

			/*	Señalo la correcta recepción	*/
			write(nuevofd, "Listo", 6 );

			/*	Seteo de pipes	*/

			ctrl = pipe(stdin_p);
			if(ctrl == -1)	/*	Error de pipe	*/
			{
				print_Donde(">> ERROR: Error al configurar las pipes", esDeamon, LOG_ERR);
				close(nuevofd);
				return -1;
			}

			ctrl = pipe(stdout_p);

			if(ctrl == -1)		/*	Error de pipe	*/
			{
				print_Donde(">> ERROR: Error al configurar las pipes", esDeamon, LOG_ERR);
				close(nuevofd);
				return -1;
			}

			ctrl = pipe(stderr_p);
			if(ctrl == -1)		/*	Error de pipe	*/
			{
				print_Donde(">> ERROR: Error al configurar las pipes", esDeamon, LOG_ERR);
				close(nuevofd);
				return -1;
			}

			child_pid = fork(); /* FIXME */
			if(child_pid	!=0	)
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
					if(ctrl == -1)
					{
						print_Donde("ERROR: En el select.\n", esDeamon, LOG_ERR);
					}

					if( FD_ISSET(stdout_p[0], &readfds) != 0 )
					{
						if( verb != 0 )
						{
							print_Donde("out: ", esDeamon, LOG_NOTICE);
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
								print_Donde("err: ", esDeamon, LOG_NOTICE);
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
							print_Donde("cliente:", esDeamon, LOG_NOTICE);
						}
						if(ctrl == 0)
						{
							print_Donde("  Conexión con cliente cerrada.\n", esDeamon, LOG_NOTICE);
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
				if( strcmp(buff, "" ) ==0 )
				{
					write(nuevofd, "ERROR: Comando inválido.\n", 25);
					print_Donde("  ERROR: Comando inválido.\n", esDeamon, LOG_NOTICE);
				}
				print_Donde("  Ejecución de comando finalizada.\n", esDeamon, LOG_NOTICE);
				close(stdin_p[1]);
				close(stdout_p[0]);
				close(stderr_p[0]);
				close(nuevofd);
				return 0;

			} else {	/*	Hijo Comando	*/

				close(stdin_p[1]);
				close(stdout_p[0]);
				close(stderr_p[0]);
				close(nuevofd);

				dup2(stdin_p[0], STDIN_FILENO);
				dup2(stdout_p[1], STDOUT_FILENO);
				dup2(stderr_p[1], STDERR_FILENO);

				execvp(argls[0], argls);
				return 0;
			}

		} else {	/*Proceso Padre */

			waitpid(child_pid, NULL, 0);
			print_Donde(">Esperando proximo comando.\n", esDeamon, LOG_NOTICE);

		}
	}

	if(esDeamon)
	{
		closelog();
	}
	close(sock_srv);
	return 0;
}
