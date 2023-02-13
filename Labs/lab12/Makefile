PROGRAMS = prodcons_affinity \
           prodcons_attributes

LDLIBS = -lpthread -lm
# CPPFLAGS = -O0 -g3

all: $(PROGRAMS)

# // prodcons_affinity //
#
# run with:
# ./prodcons_affinity [number of consumers] [producer CPU]
# 
# # EXAMPLE: 1 producer, 3 consumers, producer in CPU3
# $ make && ./prodcons_affinity 3 3
# cc     prodcons_affinity.c  -lpthread -lm -o prodcons_affinity
# CONSUMER deployed in CPU: 0 1 2 4 5 6 7
# CONSUMER deployed in CPU: 0 1 2 4 5 6 7
# PRODUCER deployed in CPU: 3
# CONSUMER deployed in CPU: 0 1 2 4 5 6 7
# 
# 
# ps -u
# ... find the PID of process "prodcons_affinity"
# 
# start top -H -p [PID]
# then hit 'f' key and toggle the 'P' field to see the current CPU assigned