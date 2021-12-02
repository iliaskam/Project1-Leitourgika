all: 	sem_parent  \
	sem_child \

sem_parent: sem_parent.c
	gcc -g -Wall sem_parent.c -o sem_parent -lpthread

sem_child: sem_child.c
	gcc -g -Wall sem_child.c -o sem_child -lpthread

clean:
	rm -f sem_parent sem_child