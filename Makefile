#
# MSYS /etc/fstab parser test program
#

all: fstab.exe

CC = g++

fstab.exe: fstab.o
	@echo Building fstab.exe
	rm -f fstab.exe
	$(CC) -o fstab.exe fstab.o

fstab.o: fstab.cpp
	$(CC) -c -o fstab.o fstab.cpp
	
clean:
	rm -f *.obj *.o *.lib *.a *.exe *.cmx *.dll *.exp *.cmi *~

test:
	./fstab.exe ./paths.txt
