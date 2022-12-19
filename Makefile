gbtm: src/main.c
	$(CC) -o gbtm -lm src/main.c

clean:
	rm -f gbtm

.PHONY: clean
