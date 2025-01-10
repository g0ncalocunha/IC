bitStream: bitStream.cpp
	g++ -o bitStream.out bitStream.cpp

bitStreamTest: tests/bitStreamTest.cpp
	g++ -o tests/bitStreamTest.out tests/bitStreamTest.cpp

golomb: golomb.cpp
	g++ -o golomb.out golomb.cpp

golombTest: tests/golombTest.cpp
	g++ -o tests/golombTest.out tests/golombTest.cpp golomb.cpp bitStream.cpp

audioCoding: audioCoding.cpp
	g++ -o audioCoding.out audioCoding.cpp bitStream.cpp -lsfml-audio -lsfml-system -fsanitize=address

imageCoder: imageCoder.cpp
	g++ -o imageCoder.out imageCoder.cpp predictors.cpp golomb.cpp bitStream.cpp `pkg-config --cflags --libs opencv4`

imageTester: tests/ImageTester.cpp
	g++ -o tests/imageTester.out tests/ImageTester.cpp tests/CompressionVerifier.cpp `pkg-config --cflags --libs opencv4`

videoCoder: videoCoder.cpp
	g++ -o videoCoder.out videoCoder.cpp imageCoder.cpp predictors.cpp golomb.cpp bitStream.cpp `pkg-config --cflags --libs opencv4`

videoTester: VideoTester.cpp
	g++ -o VideoTester.out VideoTester.cpp CompressionVerifier.cpp bitStream.cpp predictors.cpp golomb.cpp imageCoder.cpp `pkg-config --cflags --libs opencv4`

clean:
	rm -f *.out
