make: 
	gcc comandos.c cliente.c socket.c socket.h functions.c -o cliente
	gcc comandos.c servidor.c socket.c socket.h functions.c -o servidor