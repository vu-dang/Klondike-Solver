CXX := g++
CXXFLAGS := -Wall -g -std=c++11
LDLIBS := -lpthread

# Sources shared by every front end (everything except the main() files).
shared   := Card.cpp Move.cpp Pile.cpp Random.cpp Solitaire.cpp
sharedobj := $(patsubst %.cpp, %.o, $(shared))

all: KlondikeSolver SimpleSolver

KlondikeSolver: KlondikeSolver.o $(sharedobj)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

SimpleSolver: SimpleSolver.o $(sharedobj)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f *.o

dist-clean: clean
	rm -f *~ KlondikeSolver SimpleSolver
