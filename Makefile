#make file - this is a comment section
 
all:    #target name
	gcc server.c -o server
	gcc client.c utils.c -o client