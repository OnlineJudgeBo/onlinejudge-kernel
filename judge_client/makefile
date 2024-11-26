all: judge_client.cc
	g++ -Wall -c -I/usr/local/mysql/include/mysql -I/usr/include/mysql -I/usr/include/cjson judge_client.cc
	g++ -Wall -c -I/usr/include/cjson shared.cc
	g++ -Wall -c -I/usr/include/cjson utils.cc
	g++ -Wall -c -I/usr/include/cjson ./similar_code/similar_code.cc
	g++ -Wall -o judge_client judge_client.o shared.o utils.o similar_code.o -L/usr/local/mysql/lib/mysql -L/usr/lib/mysql -lmysqlclient -lcurl -lcjson

clean:
	rm -f judge_client judge_client.o
