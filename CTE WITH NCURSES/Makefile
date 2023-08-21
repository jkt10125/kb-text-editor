a: utils.o test.o
	g++ -o a test.o utils.o -g

utils.o: utils.cpp utils.hpp
      g++ -c utils.cpp

test.o: test.cpp utils.hpp
	g++ -c test.cpp

clean:
      rm -f *.o a
