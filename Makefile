energy_reader: main.c
	gcc main.c -o energy_reader
	mkdir -p build
	mv energy_reader build/
