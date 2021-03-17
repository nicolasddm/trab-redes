make: 
	gcc comandos.c cliente.c socket.c socket.h -o cliente
	gcc comandos.c servidor.c socket.c socket.h -o servidor