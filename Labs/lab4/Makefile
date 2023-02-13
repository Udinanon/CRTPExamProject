

##
## OPTIMIZATION FLAGS
##
CFLAGS = -O3
#CFLAGS += -fdump-passes # add this to show active optimization passes

all: ackermann test_gradient ackermann_function.o

##             _                                          ##
##   __ _  ___| | _____ _ __ _ __ ___   __ _ _ __  _ __   ##
##  / _` |/ __| |/ / _ \ '__| '_ ` _ \ / _` | '_ \| '_ \  ##
## | (_| | (__|   <  __/ |  | | | | | | (_| | | | | | | | ##
##  \__,_|\___|_|\_\___|_|  |_| |_| |_|\__,_|_| |_|_| |_| ##
##                                                        ##

# ackermann application specific flags
ackermann: CFLAGS += -fdump-tree-original \
          			 -fdump-tree-optimized \
          			 -fopt-info-inline-optimized-missed=inline.txt

        #   -fopt-info-missed=missed.all # add this to see all missed optimizations

# ackermann application target
ackermann: ackermann.o
	${LINK.o} $^ -o $@ 




##       _                                     _   _           _          _   _              ##
##    __| |_   _ _ __ ___  _ __     ___  _ __ | |_(_)_ __ ___ (_)______ _| |_(_) ___  _ __   ##
##   / _` | | | | '_ ` _ \| '_ \   / _ \| '_ \| __| | '_ ` _ \| |_  / _` | __| |/ _ \| '_ \  ##
##  | (_| | |_| | | | | | | |_) | | (_) | |_) | |_| | | | | | | |/ / (_| | |_| | (_) | | | | ##
##   \__,_|\__,_|_| |_| |_| .__/   \___/| .__/ \__|_|_| |_| |_|_/___\__,_|\__|_|\___/|_| |_| ##
##                        |_|           |_|                                                  ##

DUMP_TREES = original cfg ssa optimized
DUMP_RTL = final
# DUMP_TREES = all
# DUMP_RTL = final

# all dumps of optimization passes are performed trying to compile this object
ackermann_function.o: \
CFLAGS = $(foreach t,${DUMP_TREES},-fdump-tree-$t -fdump-tree-$t-graph ) \
         $(foreach t,${DUMP_RTL},-fdump-rtl-$t )



.SUFFIXES: .dot .png 
.dot.png:
	dot -Tpng $< -o $@

GRAPH_FILES = $(patsubst %.dot,%.png,$(wildcard *.dot))

# use this target to convert .dot files into png images
graphs: ${GRAPH_FILES}
	


##   _            _                           _ _            _    ##
##  | |_ ___  ___| |_      __ _ _ __ __ _  __| (_) ___ _ __ | |_  ##
##  | __/ _ \/ __| __|    / _` | '__/ _` |/ _` | |/ _ \ '_ \| __| ##
##  | ||  __/\__ \ |_    | (_| | | | (_| | (_| | |  __/ | | | |_  ##
##   \__\___||___/\__|____\__, |_|  \__,_|\__,_|_|\___|_| |_|\__| ##
##                  |_____|___/                                   ##

# testing optimization timing on a real case
# the Sobel filter example

test_gradient: gradient.o test_gradient.o
	${LINK.o} $^ -o $@


clean:
	rm -rf *.o ackermann.c.* inline.txt ackermann_function.c.* 
