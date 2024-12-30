bitStream: bitStream.cpp
	g++ -o bitStream.out bitStream.cpp

bitStreamTest: bitStreamTest.cpp
	g++ -o bitStreamTest.out bitStreamTest.cpp

golomb: golomb.cpp
		g++ -o golomb.out golomb.cpp

golombTest: golombTest.cpp
		g++ -o golombTest.out golombTest.cpp

audioCoding: audioCoding.cpp
		g++ -o audioCoding.out audioCoding.cpp -lsfml-audio -lsfml-system

imageCoder: imageCoder.cpp
	g++ -o imageCoder.out imageCoder.cpp golomb.cpp predictors.cpp bitStream.cpp `pkg-config --cflags --libs opencv4`
clean:
	rm -f *.out
