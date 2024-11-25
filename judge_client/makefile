all: judge_client.cc
	g++ -Wall -c -I/usr/local/mysql/include/mysql -I/usr/include/mysql -I/usr/include/cjson judge_client.cc
	g++ -Wall -c shared.cc -I/usr/include/cjson
	g++ -Wall -c utils.cc
	g++ -Wall -o judge_client judge_client.o shared.o utils.o -L/usr/local/mysql/lib/mysql -L/usr/lib/mysql -lmysqlclient -lcurl -lcjson

clean:
	rm -f judge_client judge_client.o
