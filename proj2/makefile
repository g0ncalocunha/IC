bitStream: util/bitStream.cpp
	g++ -o output/bitStream.out util/bitStream.cpp
	./output/bitStream.out

bitStreamTest: tests/bitStreamTest.cpp
	g++ -o output/bitStreamTest.out tests/bitStreamTest.cpp
	./output/bitStreamTest.out

golomb: util/golomb.cpp
	g++ -o output/golomb.out util/golomb.cpp
	./output/golomb.out

golombTest: tests/golombTest.cpp
	g++ -o output/golombTest.out tests/golombTest.cpp util/golomb.cpp util/bitStream.cpp
	./output/golombTest.out

audioCoding: audio/audioCoding.cpp
	g++ -o output/audioCoding.out audio/audioCoding.cpp util/bitStream.cpp -lsfml-audio -lsfml-system -fsanitize=address
	./output/audioCoding.out

audioTester: tests/AudioTest.cpp
	g++ -o output/AudioTest.out tests/AudioTest.cpp audio/audioCoding.cpp util/bitStream.cpp -lsfml-audio -lsfml-system -fsanitize=address
	./output/AudioTest.out

imageCoder: image/imageCoder.cpp
	g++ -o output/imageCoder.out image/imageCoder.cpp util/predictors.cpp util/golomb.cpp util/bitStream.cpp `pkg-config --cflags --libs opencv4`
	./output/imageCoder.out

imageTester: tests/ImageTester.cpp
	g++ -o output/imageTester.out tests/ImageTester.cpp tests/CompressionVerifier.cpp `pkg-config --cflags --libs opencv4`
	./output/imageTester.out

videoCoder: video/videoCoder.cpp
	g++ -o output/videoCoder.out video/videoCoder.cpp image/imageCoder.cpp util/predictors.cpp util/golomb.cpp util/bitStream.cpp `pkg-config --cflags --libs opencv4`
	./output/videoCoder.out

videoTester: VideoTester.cpp
	g++ -o output/VideoTester.out video/VideoTester.cpp tests/CompressionVerifier.cpp util/bitStream.cpp util/predictors.cpp util/golomb.cpp image/imageCoder.cpp `pkg-config --cflags --libs opencv4`
	./output/VideoTester.out

demo:	demo.cpp
	g++ -o output/demo.out demo.cpp  -lsfml-audio -lsfml-system -fsanitize=address `pkg-config --cflags --libs opencv4`
	./output/demo.out
clean:
	rm -f output/*.out
	rm -f output/*.bin
