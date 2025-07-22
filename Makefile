all:
	gcc -g fs.c simplefat.c -o simplefat
clean:
	rm -f simplefat
