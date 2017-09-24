all: test-worker

default: test-worker

clean:
	rm -rf worker

worker: worker.c
	gcc -g -o worker worker.c

test-worker: worker
	./worker -x 2 -n 3

gdb:
	gdb --args ./worker -x -n

# master: master.c
# 	gcc -c -Wall -Werror -fpic -o master master.c

# test-master: worker master
# 	./master --worker_path ./worker --num_workers 5 --wait_mechanism select -x 2 -n 12

dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
