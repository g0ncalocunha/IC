bitStream: bitStream.cpp
	g++ -o bitStream.out bitStream.cpp

bitStreamTest: bitStreamTest.cpp
	g++ -o bitStreamTest.out bitStreamTest.cpp

golomb: golomb.cpp
		g++ -o golomb.out golomb.cpp

golombTest: golombTest.cpp
		g++ -o golombTest.out golombTest.cpp

audioCoding: audioCoding.cpp
	g++ -o audioCoding.out audioCoding.cpp bitStream.cpp -lsfml-audio -lsfml-system -fsanitize=address

imageCoder: imageCoder.cpp
	g++ -o imageCoder.out imageCoder.cpp predictors.cpp golomb.cpp bitStream.cpp `pkg-config --cflags --libs opencv4`

imageTester: ImageTester.cpp
	g++ -o imageTester.out ImageTester.cpp CompressionVerifier.cpp `pkg-config --cflags --libs opencv4`

videoCoder: videoCoder.cpp
	g++ -o videoCoder.out videoCoder.cpp imageCoder.cpp predictors.cpp golomb.cpp bitStream.cpp `pkg-config --cflags --libs opencv4`

videoTester: VideoTester.cpp
	g++ -o VideoTester.out VideoTester.cpp CompressionVerifier.cpp bitStream.cpp predictors.cpp golomb.cpp imageCoder.cpp `pkg-config --cflags --libs opencv4`

clean:
	rm -f *.out
