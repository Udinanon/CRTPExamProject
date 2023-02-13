
PROGRAMS = prodcons_threads \
           prodcons_semaphores \
		   prodcons_msgqueue

LDLIBS = -lpthread

# NOTE:
# here we are using a hidden implicit rule that links any .c file into an executable
# to see all active implicit rules use: make -p 

all: $(PROGRAMS)

clean:
	rm $(PROGRAMS)

