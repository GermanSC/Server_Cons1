# Servidor de la Consigna 1

El servidor recibe un comando de shell de un cliente remoto por conexion TCP/IP, y lo ejecuta redirigiendo las entradas y salidas 
estandares al cliente mediante la conexión establecida.

Puede funcinar en modo Normal, donde toda la información es transmitida al usuario por consola, O en modo Daemon, donde se 
desconecta de la consola y vuelca la información en un syslog.
