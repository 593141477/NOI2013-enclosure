all: bot
bot: output.cpp
	g++ -O -DDEBUG_BOT output.cpp -o bot
output.cpp: $(wildcard src/*.cpp) $(wildcard src/*.h) $(wildcard src/*.c)
	./merge.sh
