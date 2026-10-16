# Build:  make        Run (from this folder):  ./hub
CXX      ?= g++
CXXFLAGS  = -std=c++17 -Wall -Wextra -pedantic -Iashutosh -Iprachi -Ivikalp -Iswastika
ifeq ($(OS),Windows_NT)
  LIBS = -lws2_32
  OUT  = hub.exe
else
  OUT  = hub
endif

$(OUT): ashutosh/main.cpp $(wildcard ashutosh/*.h prachi/*.h vikalp/*.h swastika/*.h)
	$(CXX) $(CXXFLAGS) ashutosh/main.cpp -o $(OUT) $(LIBS)

clean:
	rm -f hub hub.exe
