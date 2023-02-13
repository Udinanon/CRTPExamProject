
all: read_mouse VideoCapture

read_mouse: read_mouse.c
	${CC} $^ -o $@

VideoCapture: VideoCapture.c
	${CC} $^ -o $@

clean:
	rm -f read_mouse VideoCapture