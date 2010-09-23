CC=gcc


all: MyBot

clean:
	rm -rf *.o MyBot MyBot.exe

MyBot: MyBot.o PlanetWars.o Utils.o Bot.o GameForecaster.o
	gcc -O2 -o MyBot MyBot.o PlanetWars.o Utils.o Bot.o GameForecaster.o -lm

MyBot.o:
	$(CC) -O3 -funroll-loops -c -o MyBot.o MyBot.cc

PlanetWars.o:
	$(CC) -O3 -funroll-loops -c -o PlanetWars.o PlanetWars.cc

Utils.o:
	$(CC) -O3 -funroll-loops -c -o Utils.o Utils.cc
	
Bot.o:
	$(CC) -O3 -funroll-loops -c -o Bot.o Bot.cc
	
GameForecaster.o:
	$(CC) -O3 -funroll-loops -c -o GameForecaster.o GameForecaster.cc
	
	