oss: main.o
	cc -o oss main.o
	
user: chproc.o
	cc -o user chproc.o

main.o: main.c
	cc -c main.c
	
chproc.o: chproc.c
	cc -c chproc.c
  
clean :
	rm oss user main.o chproc.o