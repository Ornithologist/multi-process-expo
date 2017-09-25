all: test-epoll

default: test-epoll

check: test-epoll

clean:
	rm -rf worker.o worker master.o master

worker.o: worker.c
	gcc -c -Wall -Werror -fpic -o worker.o worker.c

worker: worker.o
	gcc -g -o worker worker.c

test-worker: worker
	./worker -x 2 -n 3

gdb:
	gdb --args ./worker -x -n

master.o: master.c
	gcc -c -Wall -Werror -fpic -o master.o master.c

master: master.o worker
	gcc -g -o master master.o

test-epoll: worker master
	./master --worker_path ./worker --num_workers 5 --wait_mechanism epoll -x 2 -n 12

test-select: worker master
	./master --worker_path ./worker --num_workers 5 --wait_mechanism select -x 2 -n 12

dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
