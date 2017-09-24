all: test-master

default: test-master

clean:
	rm -rf worker master.o master

worker: worker.c
	gcc -g -o worker worker.c

test-worker: worker
	./worker -x 2 -n 3

gdb:
	gdb --args ./worker -x -n

master.o: master.c
	gcc -c -Wall -Werror -fpic -o master.o master.c

master: master.o
	gcc -g -o master master.o

test-master: worker master
	./master --worker_path ./worker --num_workers 5 --wait_mechanism select -x 2 -n 12

dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
