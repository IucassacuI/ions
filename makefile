OBJ = bin/main.o bin/ui.o bin/callbacks.o bin/helpers.o
CC = gcc
IUP_INCLUDE = -IC:/tcc/iup/include/
IUP_LIB = -LC:/tcc/iup/
CUTILS_INCLUDE = -IC:/Users/lucas_inueglc/Documents/C/cutils
CFLAGS = -Wall -Wl,-subsystem=windows
GOFLAGS = -ldflags "-s -w"
LIBS = -liup -lgdi32 -lcomdlg32 -lcomctl32 -luuid -loleaut32 -lole32

bin/ion.exe: $(OBJ) bin/librarian.exe
	$(CC) $(CFLAGS) -o bin/ion.exe $(OBJ) $(CUTILS_INCLUDE) $(IUP_INCLUDE) $(IUP_LIB) $(LIBS)

bin/main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c -o bin/main.o $(IUP_INCLUDE) $(CUTILS_INCLUDE)

bin/ui.o: src/ui.c
	$(CC) $(CFLAGS) -c src/ui.c -o bin/ui.o $(IUP_INCLUDE) $(CUTILS_INCLUDE)

bin/callbacks.o: src/callbacks.c
	$(CC) $(CFLAGS) -c src/callbacks.c -o bin/callbacks.o $(IUP_INCLUDE) $(CUTILS_INCLUDE)

bin/helpers.o: src/helpers.c
	$(CC) $(CFLAGS) -c src/helpers.c -o bin/helpers.o $(IUP_INCLUDE) $(CUTILS_INCLUDE)

bin/librarian.exe: src/librarian/librarian.go
	cd src/librarian; go build $(GOFLAGS) -o ../../bin/librarian.exe .

clean:
	rm bin/*.exe bin/*.o
