/****************************************************************************/
/* Plantilla para cabeceras de funciones del cliente (rcftpclient)          */
/* Plantilla $Revision: 2.1 $ */
/* Autor: Vivas Gómez Javier  */
/* Autor: Solanas,Andrea */

/****************************************************************************/

/**
 * Obtiene la estructura de direcciones del servidor
 *
 * @param[in] dir_servidor String con la dirección de destino
 * @param[in] servicio String con el servicio/número de puerto
 * @param[in] f_verbose Flag para imprimir información adicional
 * @return Dirección de estructura con la dirección del servidor
 */
struct addrinfo* obtener_struct_direccion(char *dir_servidor, char *servicio, char f_verbose);

/**
 * Imprime una estructura sockaddr_in o sockaddr_in6 almacenada en sockaddr_storage
 *
 * @param[in] saddr Estructura de dirección
 */
void printsockaddr(struct sockaddr_storage * saddr);

/**
 * Configura el socket
 *
 * @param[in] servinfo Estructura con la dirección del servidor
 * @param[in] f_verbose Flag para imprimir información adicional
 * @return Descriptor del socket creado
 */
int initsocket(struct addrinfo *servinfo, char f_verbose);

/**
 * Funcion envio de mensaje
 */
void enviarDatos (struct rcftp_msg *mensaje_enviar, int socket, struct sockaddr *remote, socklen_t remotelen); 
	
/**
 * Funcion para recibir datos
 */
ssize_t recibirDatos (int socket, struct rcftp_msg *buffer, int len, struct addrinfo *servinfo);

/**
 * Funcion crear mensaje: Crea un mensaje nuevo RCFTP
 */
struct rcftp_msg crearMensajeRCFTP(char* msg, size_t len, size_t numseq, int ultimoMensaje);

/**
 * Funcion que comprueba si la version y el checksum del mensaje es valido.
 */
int esMensajeValido(struct rcftp_msg envio);

/**
 * Comprobacion de respuesta esperada correctamente.
 */
int respuestaEsperada(struct rcftp_msg env, struct rcftp_msg resp);

/**
 * Algoritmo 1 del cliente
 *
 * @param[in] socket Descriptor del socket
 * @param[in] servinfo Estructura con la dirección del servidor
 */
void alg_basico(int socket, struct addrinfo *servinfo);


/**
 * Algoritmo 2 del cliente
 *
 * @param[in] socket Descriptor del socket
 * @param[in] servinfo Estructura con la dirección del servidor
 */
void alg_ventana_anunciada(int socket, struct addrinfo *servinfo);


/**
 * Algoritmo 3 del cliente
 *
 * @param[in] socket Descriptor del socket
 * @param[in] servinfo Estructura con la dirección del servidor
 */
void alg_congestion(int socket, struct addrinfo *servinfo);


