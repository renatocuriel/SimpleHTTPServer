all: httpd

httpd: httpd.o
	gcc -g httpd.o -o httpd

httpd.o:
	gcc -c httpd.c

clean:
	rm httpd *.o
