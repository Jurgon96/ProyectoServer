/****************************************************************************/
/* Plantilla para implementación de funciones del cliente (rcftpclient)     */
/* $Revision: 2.1 $ */
/* Aunque se permite la modificación de cualquier parte del código, se */
/* recomienda modificar solamente este fichero y su fichero de cabeceras asociado. */
/****************************************************************************/

/**************************************************************************/
/* INCLUDES                                                               */
/**************************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "rcftp.h" // Protocolo RCFTP
#include "rcftpclient.h" // Funciones ya implementadas
#include "multialarm.h" // Gestión de timeouts
#include "misfunciones.h"


/**************************************************************************/
/* VARIABLES GLOBALES                                                     */
/**************************************************************************/

char* autores="Autor: Solanas de Vicente, Andrea\nAutor: Vivas Gomez, Javier"; // dos autores

// variable para indicar si mostrar información extra durante la ejecución
// como la mayoría de las funciones necesitaran consultarla, la definimos global
extern char verb;


// variable externa que muestra el número de timeouts vencidos
// Uso: Comparar con otra variable inicializada a 0; si son distintas, tratar un timeout e incrementar en uno la otra variable
extern volatile const int timeouts_vencidos;


/**************************************************************************/
/* Obtiene la estructura de direcciones del servidor */
/**************************************************************************/
struct addrinfo* obtener_struct_direccion(char *dir_servidor, char *servicio, char f_verbose){
    struct addrinfo hints,     // variable para especificar la solicitud
                    *servinfo; // puntero para respuesta de getaddrinfo()
    int status;      // finalización correcta o no de la llamada getaddrinfo()
    int numdir = 1;  // contador de estructuras de direcciones en la lista de direcciones de servinfo
    struct addrinfo *direccion; // puntero para recorrer la lista de direcciones de servinfo

    // sobreescribimos con ceros la estructura para borrar cualquier dato que pueda malinterpretarse
    memset(&hints, 0, sizeof hints); 

    // genera una estructura de dirección con especificaciones de la solicitud
    if (f_verbose)
    {
        printf("1 - Especificando detalles de la estructura de direcciones a solicitar... \n");
        fflush(stdout);
    }
    
    hints.ai_family = AF_UNSPEC; // opciones: AF_UNSPEC; IPv4: AF_INET; IPv6: AF_INET6; etc.

    if (f_verbose)
    { 
        printf("\tFamilia de direcciones/protocolos: ");
        switch (hints.ai_family)
        {
            case AF_UNSPEC: printf("IPv4 e IPv6\n"); break;
            case AF_INET:   printf("IPv4)\n"); break;
            case AF_INET6:  printf("IPv6)\n"); break;
            default:        printf("No IP (%d)\n", hints.ai_family); break;
        }
        fflush(stdout);
    }

    hints.ai_socktype = SOCK_DGRAM ; // especificar tipo de socket 

    if (f_verbose)
    { 
        printf("\tTipo de comunicación: ");
        switch (hints.ai_socktype)
        {
            case SOCK_STREAM: printf("flujo (TCP)\n"); break;
            case SOCK_DGRAM:  printf("datagrama (UDP)\n"); break;
            default:          printf("no convencional (%d)\n", hints.ai_socktype); break;
        }
        fflush(stdout);
    }
   
    // flags específicos dependiendo de si queremos la dirección como cliente o como servidor
    if (dir_servidor != NULL)
    {
        // si hemos especificado dir_servidor, es que somos el cliente y vamos a conectarnos con dir_servidor
        if (f_verbose) printf("\tNombre/dirección del equipo: %s\n", dir_servidor); 
    }
    else
    {
        // si no hemos especificado, es que vamos a ser el servidor
        if (f_verbose) printf("\tNombre/dirección del equipo: ninguno (seremos el servidor)\n"); 
        hints.ai_flags = AI_PASSIVE; // especificar flag para que la IP se rellene con lo necesario para hacer bind
    }
    if (f_verbose) printf("\tServicio/puerto: %s\n", servicio);

    // llamada a getaddrinfo() para obtener la estructura de direcciones solicitada
    // getaddrinfo() pide memoria dinámica al SO, la rellena con la estructura de direcciones,
    // y escribe en servinfo la dirección donde se encuentra dicha estructura
    // la memoria *dinámica* reservada por una función NO se libera al salir de ella.
    // Para liberar esta memoria, usar freeaddrinfo()
    if (f_verbose)
    {
        printf("2 - Solicitando la estructura de direcciones con getaddrinfo()... ");
        fflush(stdout);
    }
    status = getaddrinfo(dir_servidor, servicio, &hints, &servinfo);
    if (status != 0)
    {
        fprintf(stderr,"Error en la llamada getaddrinfo(): %s\n", gai_strerror(status));
        exit(1);
    } 
    if (f_verbose)
    {
        printf("hecho\n");
    }

    // imprime la estructura de direcciones devuelta por getaddrinfo()
    if (f_verbose)
    {
        printf("3 - Analizando estructura de direcciones devuelta... \n");
        direccion = servinfo;
        while (direccion != NULL)
        {   // bucle que recorre la lista de direcciones
            printf("    Dirección %d:\n", numdir);
            printsockaddr((struct sockaddr_storage*) direccion->ai_addr);
            // "avanzamos" direccion a la siguiente estructura de direccion
            direccion = direccion->ai_next;
            numdir++;
        }
    }

    // devuelve la estructura de direcciones devuelta por getaddrinfo()
    return servinfo;
}

/**************************************************************************/
/* Imprime una direccion */
/**************************************************************************/
void printsockaddr(struct sockaddr_storage * saddr)
{
    struct sockaddr_in  *saddr_ipv4; // puntero a estructura de dirección IPv4
    // el compilador interpretará lo apuntado como estructura de dirección IPv4
    struct sockaddr_in6 *saddr_ipv6; // puntero a estructura de dirección IPv6
    // el compilador interpretará lo apuntado como estructura de dirección IPv6
    void *addr; // puntero a dirección. Como puede ser tipo IPv4 o IPv6 no queremos que el compilador la interprete de alguna forma particular, por eso void
    char ipstr[INET6_ADDRSTRLEN]; // string para la dirección en formato texto
    int port; // para almacenar el número de puerto al analizar estructura devuelta

    if (saddr == NULL)
    { 
        printf("La dirección está vacía\n");
    }
    else
    {
        printf("\tFamilia de direcciones: ");
        fflush(stdout);
        if (saddr->ss_family == AF_INET6)
        {   //IPv6
            printf("IPv6\n");
            // apuntamos a la estructura con saddr_ipv6 (el cast evita el warning),
            // así podemos acceder al resto de campos a través de este puntero sin más casts
            saddr_ipv6 = (struct sockaddr_in6 *)saddr;
            // apuntamos a donde está realmente la dirección dentro de la estructura
            addr = &(saddr_ipv6->sin6_addr);
            // obtenemos el puerto, pasando del formato de red al formato local
            port = ntohs(saddr_ipv6->sin6_port);
        }
        else if (saddr->ss_family == AF_INET)
        {   //IPv4
            printf("IPv4\n");
            saddr_ipv4 = (struct sockaddr_in *)saddr;
            addr = &(saddr_ipv4->sin_addr);
            port = ntohs(saddr_ipv4->sin_port);
        }
        else
        {
            fprintf(stderr, "familia desconocida\n");
            exit(1);
        }
        // convierte la dirección ip a string 
        inet_ntop(saddr->ss_family, addr, ipstr, sizeof ipstr);
        printf("\tDirección (interpretada según familia): %s\n", ipstr);
        printf("\tPuerto (formato local): %d\n", port);
    }
}
/**************************************************************************/
/* Configura el socket, devuelve el socket y servinfo */
/**************************************************************************/
int initsocket(struct addrinfo *servinfo, char f_verbose)
{
    int sock;

    printf("\nSe usará ÚNICAMENTE la primera dirección de la estructura\n");

    // crea un extremo de la comunicación y devuelve un descriptor
    if (f_verbose)
    {
        printf("Creando el socket (socket)... ");
        fflush(stdout);
    }
    sock = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol);
    if (sock < 0)
    {
        perror("Error en la llamada socket: No se pudo crear el socket");
        /*muestra por pantalla el valor de la cadena suministrada por el programador, dos puntos y un mensaje de error que detalla la causa del error cometido */
        exit(1);
    }
    if (f_verbose) printf("hecho\n");

   
    return sock;
}


/**************************************************************************/
/*  Función para recibir datos  */
/**************************************************************************/
ssize_t recibirDatos(int socket, struct rcftp_msg *buffer, int len, struct addrinfo *servinfo){
	ssize_t datosRecibidos = recvfrom(socket, (char*) buffer, len, 0, (struct sockaddr*) servinfo, &servinfo->ai_addrlen);
	if(errno!=EAGAIN && datosRecibidos < 0){
		printf("Error en rcvfrom");
		exit(1);
	}
	return datosRecibidos;
}


/**************************************************************************/
/* Funcion que comprueba si la version y el checksum del mensaje es valido. */
/**************************************************************************/
int esMensajeValido(struct rcftp_msg envio){
	int Comprobacion=1;
	if (envio.version!=RCFTP_VERSION_2) { 	//Error por version no correcta.
		Comprobacion=0;
		printf("Error: El mensaje recibido tiene versiÃ³n incorrecta\n");
	}
	else if (issumvalid(&envio,sizeof(envio))==0) { // Checksum del mensaje incorrecto
		Comprobacion=0;
		printf("Error: El mensaje recibido tiene checksum incorrecto\n");
	}
	return Comprobacion;
}


/**************************************************************************/
/*  Función para ver si la respuesta es la esperada  */
/**************************************************************************/
int respuestaEsperada(struct rcftp_msg env, struct rcftp_msg resp){
	int Comprobacion=1;
	if (ntohl(resp.next) != (ntohl(env.numseq) + ntohs(env.len))){
		Comprobacion = 0;
		printf("Error");
	}
	else if (resp.flags == F_BUSY){
		Comprobacion = 0;
		printf("Error");
	}
	else if (resp.flags == F_ABORT){
		Comprobacion = 0;
		printf("Error");
	}
	else if (env.flags == F_FIN && resp.flags != F_FIN){
		Comprobacion = 0;
		printf("Error");
	}
	return Comprobacion;
}



/**************************************************************************/
/* Funcion envio de mensaje */
/**************************************************************************/
void enviarDatos(struct rcftp_msg *mensaje_enviar, int socket, struct sockaddr *remote, socklen_t remotelen){
	ssize_t datosEnviados = sendto(socket, mensaje_enviar, sizeof(*mensaje_enviar), 0, remote, remotelen);
	if(datosEnviados != sizeof(*mensaje_enviar) || datosEnviados < 0){
		printf("Error en sendto");
		exit(1);
	}
}



/**************************************************************************/
/* Funcion crear mensaje: Crea un mensaje nuevo RCFTP */
/**************************************************************************/
struct rcftp_msg crearMensajeRCFTP(char* msg, size_t len, size_t numseq, int ultimoMensaje){
	struct rcftp_msg mensaje_enviar;
	mensaje_enviar.version = RCFTP_VERSION_2;
	mensaje_enviar.numseq = htonl((uint32_t) numseq);
	mensaje_enviar.next = htonl(0);

	if(ultimoMensaje==1){
		mensaje_enviar.flags=F_FIN;
	}
	else{
		mensaje_enviar.flags=F_NOFLAGS;
	}
	
	mensaje_enviar.len = htons((uint16_t) len);
	int j;
	for(j=0; j<len; j++){
		mensaje_enviar.buffer[j]=msg[j];
	}
	
	mensaje_enviar.sum=0;
	mensaje_enviar.sum=xsum((char*)&mensaje_enviar,sizeof(mensaje_enviar));
	return mensaje_enviar;
}



/**************************************************************************/
/*  algoritmo 1 (basico)  */
/**************************************************************************/
void alg_basico(int socket, struct addrinfo *servinfo) {
	
	printf("Comunicacion mediante el algoritmo basico\n");

	int ultimoMensaje=0;
	int ultimoMensajeConfirmado=0;
	
	char buffer[RCFTP_BUFLEN]; //Buffer para almacenar los datos. 
	ssize_t len; //longitud de lo que vamos a leer del fichero
	ssize_t numeroSec = 0; //numero de secuencia
	struct rcftp_msg mensajeEnvio;	//Estructura de el envío.
	struct rcftp_msg mensajeRespuesta; //Estructura de la respuesta

	len=readtobuffer(buffer,RCFTP_BUFLEN); 
	if (len<RCFTP_BUFLEN && len>= 0){ //Cuando tenemos menos de 512 bytes es cuando estamos enviando el ultimo paquete.
		ultimoMensaje = 1;
	}
	
	mensajeEnvio=crearMensajeRCFTP(buffer, len, numeroSec, ultimoMensaje); //Creamos la estructura del mensaje que vamos a enviar.

	while (ultimoMensajeConfirmado==0){
		//Enviando mensaje
		enviarDatos(&mensajeEnvio, socket, servinfo->ai_addr, servinfo->ai_addrlen);
		printf("Mensaje enviado\n");
		print_rcftp_msg(&mensajeEnvio, sizeof(mensajeEnvio));
		printf("\n");
		
		//Recibo mensaje
		recibirDatos(socket, &mensajeRespuesta, sizeof(mensajeRespuesta), servinfo);
		printf("Mensaje recibido\n");
		print_rcftp_msg(&mensajeRespuesta, sizeof(mensajeRespuesta));
		printf("\n");

		if(esMensajeValido(mensajeRespuesta) && respuestaEsperada(mensajeEnvio,mensajeRespuesta)){
			if(ultimoMensaje==1){
				ultimoMensajeConfirmado = 1;
			}
			else{
				numeroSec = numeroSec + len; //modifica el numero de secuencia segun lo que ha leido
				len = readtobuffer(buffer,RCFTP_BUFLEN);
				if(len<RCFTP_BUFLEN && len>= 0){ //Fin del fichero
					ultimoMensaje= 1;
				}
				mensajeEnvio=crearMensajeRCFTP(buffer, len, numeroSec, ultimoMensaje); 
			}
		}
	}
}

/**************************************************************************/
/*  algoritmo 2 (ventana_anunciada)  */
/**************************************************************************/
void alg_ventana_anunciada(int socket, struct addrinfo *servinfo) {
	
	printf("Comunicacion mediante el algoritmo de ventana anunciada\n");

	int ultimoMensaje=0;
	int ultimoMensajeConfirmado=0;
	ssize_t len; //longitud de lo que vamos a leer del fichero
	size_t numeroSec = 0; //numero de secuencia
	size_t ventana; //longitud de la ventana anunciada (nos lo da el servidor)
	char buffer[RCFTP_BUFLEN]; //Buffer para almacenar los datos.
	struct rcftp_msg mensajeEnvio;	//Estructura del envio
	struct rcftp_msg mensajeRespuesta; //Estructura de la respuesta

	len=readtobuffer(buffer,RCFTP_BUFLEN);
	if (len<RCFTP_BUFLEN && len>= 0){ //Cuando tenemos menos de los bytes que lee es cuando estamos enviando el ultimo paquete.
		ultimoMensaje = 1;
	}

	mensajeEnvio=crearMensajeRCFTP(buffer, len, numeroSec, ultimoMensaje); //Creamos la estructura del mensaje que vamos a enviar

	while (ultimoMensajeConfirmado==0){
		//Enviando mensaje
		enviarDatos(&mensajeEnvio, socket, servinfo->ai_addr, servinfo->ai_addrlen);
		printf("Mensaje enviado\n");
		print_rcftp_msg(&mensajeEnvio, sizeof(mensajeEnvio));
		printf("\n");

		//Recibo mensaje
		recibirDatos(socket,&mensajeRespuesta,sizeof(mensajeRespuesta),servinfo);
		printf("Mensaje recibido\n");
		print_rcftp_msg(&mensajeRespuesta, sizeof(mensajeRespuesta));
		printf("\n");

		ventana = (size_t) ntohs(mensajeRespuesta.len); //actualiza el tamaño de ventana 
		printf("Tamaño de ventana: %zu\n", ventana);

		if (esMensajeValido(mensajeRespuesta) && respuestaEsperada(mensajeEnvio,mensajeRespuesta)){
			if (ultimoMensaje==1){
				ultimoMensajeConfirmado = 1;
			}
			else{
				numeroSec = numeroSec + ntohs(mensajeEnvio.len); //actualiza el numero de secuencia segun lo que ha leido
				len = readtobuffer(buffer,ventana);
				if (len<ventana && len>= 0){ //Fin del fichero
					ultimoMensaje= 1;
				}
				mensajeEnvio = crearMensajeRCFTP(buffer, len, numeroSec, ultimoMensaje);
			}
		}
	}
}

/**************************************************************************/
/*  algoritmo 3 (control de congestión)  */
/**************************************************************************/
void alg_congestion(int socket, struct addrinfo *servinfo) {
	
	printf("Comunicacion mediante el algoritmo de congestion\n");
	
	int ultimoMensaje=0;
	int ultimoMensajeConfirmado=0;
	int timeouts_procesados = 0; //variable para ver si se ha expirado un timeout
	char buffer[RCFTP_BUFLEN]; //Buffer para almacenar los datos.
	ssize_t numeroSec = 0; //numero de secuencia
	ssize_t len, nuevalen; //longitud de lo que vamos a leer del fichero y la nueva longitud
	struct rcftp_msg mensajeEnvio;	//Estructura del envio
	struct rcftp_msg mensajeRespuesta; //Estructura de la respuesta

	// especificamos el manejador de alarmas
	signal(SIGALRM, handle_sigalrm);

	// pasamos a socket no bloqueante
	int sockflags=fcntl(socket,F_GETFL,0);
	fcntl(socket,F_SETFL,sockflags|O_NONBLOCK);

	len=readtobuffer(buffer,RCFTP_BUFLEN); 
	if(len<RCFTP_BUFLEN && len>= 0){
		ultimoMensaje = 1;
	}
	
	mensajeEnvio=crearMensajeRCFTP(buffer, len, numeroSec, ultimoMensaje); //Creamos la estructura del mensaje que vamos a enviar.

	while(ultimoMensajeConfirmado==0){
		//Enviando mensaje
		enviarDatos(&mensajeEnvio, socket, servinfo->ai_addr, servinfo->ai_addrlen);
		addtimeout(); //añadimos un timeout
		printf("Mensaje enviado\n");
		print_rcftp_msg(&mensajeEnvio, sizeof(mensajeEnvio));
		printf("\n");

		while(recibirDatos(socket, &mensajeRespuesta, sizeof(mensajeRespuesta), servinfo) < 0);
		//hasta que no reciba datos se queda en bucle

		printf("Mensaje recibido\n");
		print_rcftp_msg(&mensajeRespuesta, sizeof(mensajeRespuesta));
		printf("\n");

		if(timeouts_procesados!=timeouts_vencidos){ //ha vencido el timeout
				timeouts_procesados++;
				if(len > 16){
					nuevalen = len/2;
				}
				else{ nuevalen = 16; }
		}
		else{
			canceltimeout(); //si no ha vencido, cancela
			if(len < RCFTP_BUFLEN){
				nuevalen = len*2;
			}
			else{ nuevalen = RCFTP_BUFLEN; }
		}

		printf("Tamaño de buffer: %zu\n", nuevalen);
		if(esMensajeValido(mensajeRespuesta) && respuestaEsperada(mensajeEnvio,mensajeRespuesta)){
			if(ultimoMensaje==1){
				ultimoMensajeConfirmado = 1;
			}
			else{
				numeroSec = numeroSec + len; //actualiza el numero de secuencia con lo que ha leido 
				len = readtobuffer(buffer,nuevalen);
				if (len<nuevalen && len>= 0){ //Fin del fichero
					ultimoMensaje= 1;
				}

				mensajeEnvio = crearMensajeRCFTP(buffer, len, numeroSec, ultimoMensaje);
			}
		}
	}
}



