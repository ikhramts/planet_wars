CC=g++


all: MyBot

clean:
	rm -rf *.o MyBot MyBot.exe

MyBot: MyBot.o PlanetWars.o Utils.o Bot.o GameTimeline.o Actions.o
	$(CC) -O2 -o MyBot MyBot.o PlanetWars.o Utils.o Bot.o GameTimeline.o Actions.o -lm

MyBot.o:
	$(CC) -O3 -funroll-loops -c -o MyBot.o MyBot.cc

PlanetWars.o:
	$(CC) -O3 -funroll-loops -c -o PlanetWars.o PlanetWars.cc

Utils.o:
	$(CC) -O3 -funroll-loops -c -o Utils.o Utils.cc
	
Bot.o:
	$(CC) -O3 -funroll-loops -c -o Bot.o Bot.cc
	
GameTimeline.o:
	$(CC) -O3 -funroll-loops -c -o GameTimeline.o GameTimeline.cc

Actions.o:
	$(CC) -O3 -funroll-loops -c -o Actions.o Actions.cc
	
	