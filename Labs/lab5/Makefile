


all: threads process print_segments

print_segments: print_segments.o
	$(LINK.o) $^ -o $@

threads_example: threads_example.o
	$(LINK.o) $^ -o $@ -lpthread

process_example: process_example.o
	$(LINK.o) $^ -o $@

.SUFFIXES: .pix .png .csv
.pix.png:
	python from_pixmap.py $< $@
.csv.png:
	python benchmark.py $<

threads_benchmark: threads_benchmark.o
	$(LINK.o) $^ -o $@ -lpthread

MAX_THREADS = 20
threads_benchmark.csv:
	./threads_benchmark $(MAX_THREADS)




threads: threads_example threads_benchmark \
         threads_benchmark.png

CLEANFILES = threads_example \
             threads_benchmark \
             threads_benchmark.csv \
             threads_benchmark.png 
			 
			 
process: process_example 

clean:
	rm -rf *.o $(CLEANFILES)
