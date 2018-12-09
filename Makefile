.PHONY: build

build:
	mkdir -p cmake && cd cmake && cmake .. && make && cd ..

clean:
	rm -rf ./cmake
	rm -rf ./build
	rm -rf ./*.csv
