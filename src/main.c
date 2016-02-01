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

#include <stdarg.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MAX_CMD		20
#define BUFF_SIZE	100

int esDeamon = 0;
int verb = 0;

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

void print_Donde(int dflag, int prioridad, const char *formatString, ...)
{
	va_list arguments;
	va_start(arguments, formatString);

	if (dflag)
	{
		vsyslog(prioridad, formatString, arguments);
	}
	else
	{
		vprintf(formatString, arguments);
	}

	va_end(arguments);
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
		print_Donde(esDeamon, LOG_ERR, ">>ERROR: Fallo al aceptar la conexión.\n\n");
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

void configSelect(fd_set* set, int fd_a, int fd_b, int fd_c)
{
	FD_ZERO(set);
	FD_SET(fd_a, set);
	FD_SET(fd_b, set);
	FD_SET(fd_c, set);
}

int redirect(int input, int output, char * in_string)
{
	int temp = 0;
	static char buff[1024] = "";

	temp = read( input, buff, sizeof(buff) );
	if (temp != 0)
	{
		write(output, buff, temp);

		if( verb == 1 )
		{
			print_Donde(esDeamon, LOG_NOTICE, "%s ",in_string);
			print_Donde(esDeamon, LOG_NOTICE, "%.*s\n", temp, buff );
		}
	}

	if( strcmp(buff, "" ) == 0 )
	{
		return -5;
	}
	return temp;
}

void deamonSetUp(int fd1, int fd2, int fd3)
{
	setsid();
	chdir("/");
	umask(0);

	close(fd1);
	close(fd2);
	close(fd3);
}

int main(int argc, char *argv[])
{
	/*	Variables de Conexion	*/
	int sock_srv, nuevofd;

	/*	Variables de control	*/
	pid_t d_pid		=	0;
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

	/*	Configuración del Deamon	*/
	if(esDeamon)
	{
		d_pid = fork();
		if(d_pid > 0)
		{
			return 0;
		}

		deamonSetUp(STDIN_FILENO,STDOUT_FILENO,STDERR_FILENO);
		openlog("RMT_CMD_Server", LOG_PID, LOG_USER);
	}

	/* Inicio del Programa */

	print_Donde(esDeamon, LOG_NOTICE, ">Esperando conexiones...\n");

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
			print_Donde(esDeamon, LOG_ERR, ">>ERROR: Fallo el listen.\n\n");
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
				print_Donde(esDeamon, LOG_ERR, "Error al aceptar la conexion.\n");
				return -1;
			}

			print_Donde(esDeamon, LOG_NOTICE, "  Obteniendo comandos... ");

			ctrl = recv(nuevofd, buff, 100, 0);
			print_Donde(esDeamon, LOG_NOTICE, "%s\n\n", buff);

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
				print_Donde(esDeamon, LOG_ERR, ">> ERROR: Error al configurar las pipes");
				close(nuevofd);
				return -1;
			}

			child_pid = fork();
			if(child_pid != 0 )
			{
				close(stdout_p[1]);
				close(stderr_p[1]);
				close(stdin_p[0]);

				/*	Configuro el select	*/
				fd_set readfds;

				while( waitpid(-1, NULL, WNOHANG) != child_pid )
				{
					configSelect(&readfds, nuevofd, stdout_p[0], stderr_p[0]);

					ctrl = select( ( stderr_p[1]+1 ) , &readfds, NULL, NULL, NULL );
					if(ctrl == -1)	/*	Select Error	*/
					{
						print_Donde(esDeamon, LOG_ERR, "ERROR: En el select.\n");
					}

					if( FD_ISSET(stdout_p[0], &readfds) != 0 )
					{
						ctrl = redirect(stdout_p[0],nuevofd, "out: ");
					}

					if( FD_ISSET(stderr_p[0], &readfds) != 0 )
					{
						ctrl = redirect(stderr_p[0],nuevofd,"err: ");
					}

					if( FD_ISSET(nuevofd, &readfds) != 0)
					{
						ctrl = redirect(nuevofd,stdin_p[1],"cliente: ");
						if(ctrl == 0)
						{
							print_Donde(esDeamon, LOG_NOTICE, "  Conexión con cliente cerrada.\n");
							break;
						}
					}
					if (ctrl == -5)
					{
						write(nuevofd, "ERROR: Comando inválido.\n", 25);
						print_Donde(esDeamon, LOG_NOTICE, "  ERROR: Comando inválido.\n");
					}

				}

				print_Donde(esDeamon, LOG_NOTICE, "  Ejecución de comando finalizada.\n");
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

		} else {
			/*Proceso Padre */
			waitpid(child_pid, NULL, 0);
			print_Donde(esDeamon, LOG_NOTICE, ">Esperando proximo comando.\n");
		}
	}
	return 0;
}
