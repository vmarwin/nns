
bot: bot.o
	gcc -Wall -L/home/vladimir/cgate/lib -o bot bot.o -lm -lcgate -lP2Sys -lP2ReplClient -lP2SysExt -lP2Tbl -lP2DB -lstdc++ -lgomp

bot.o: bot.cpp
	gcc -Wall -I/home/vladimir/cgate/include -c -fopenmp -o bot.o bot.cpp

clean:
	rm -f *.o
	rm -f bot
