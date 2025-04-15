chesspos: chesscmp.o cmpmsc.o cmprd.o bitfuns.o chesscmp.res
	g++ -mwindows -g -L"/cygdrive/c/Program Files (x86)/Windows Kits/8.0/Lib/win8/um/x86" -o chesscmp chesscmp.o cmpmsc.o cmprd.o bitfuns.o chesscmp.res -lGdi32 -lcurses -lComDlg32 -lComCtl32

chesscmp.o: chesscmp.c
	g++ -g -O0 -c -I./common chesscmp.c

cmpmsc.o: ./common/cmpmsc.c
	g++ -g -O0 -c -I./common ./common/cmpmsc.c

cmprd.o: ./common/cmprd.c
	g++ -g -O0 -c -I./common ./common/cmprd.c

bitfuns.o: ./common/bitfuns.c
	g++ -g -O0 -c -I./common ./common/bitfuns.c

chesscmp.res: ./common/chesscmp.rc
	windres ./common/chesscmp.rc -O coff -o chesscmp.res

clean:
	rm *.o *.res *.exe
