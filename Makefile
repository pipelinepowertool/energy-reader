energy_reader: main.c
	cc ./main.c -o ./energy_reader
	mkdir -p ./build
	mv ./energy_reader build/
