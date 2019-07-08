main : test_main.c stub.c
	gcc -g -Wall -o $@ $^

clean:
	rm -rf main

