make: 
	gcc comandos.c comandos.h cliente.c socket.c socket.h functions.c functions.h -o cliente
	gcc comandos.c comandos.h servidor.c socket.c socket.h functions.c functions.h -o servidor