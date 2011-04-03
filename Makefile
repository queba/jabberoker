all: jabberoker uptime_plugin

jabberoker: jabberoker.o
	g++ -o jabberoker jabberoker.o -lgloox -lpthread -lzmq

uptime_plugin: uptime_plugin.o plugin.o
	g++ -o uptime_plugin uptime_plugin.o plugin.o -lpthread -lzmq

clean:
	rm *.o; rm jabberoker; rm uptime_plugin
