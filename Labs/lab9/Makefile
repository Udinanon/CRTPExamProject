


PROGRAMS = prodcons \
           prodcons_0_err \
           prodcons_1_err \
           prodcons_2_err \
           prodcons_3_err \
           prodcons_4_ok \
		   prodcons_udp

LDLIBS = -lpthread -lm
# CPPFLAGS = -O0 -g3

all: $(PROGRAMS)

test0: prodcons_0_err
	@ echo "####################################################################"
	@ echo "# Try to get possible race conditions on threads in prodcons_0_err #"
	@ echo "# using the valgrind HELLGRIND tool.                               #"
	@ echo "####################################################################"

    # note: this is the same of (valgrind --tool=helgrind ./prodcons_0_err 1 )
	@ valgrind --tool=helgrind $(<D)/$< 1 
	

test1: prodcons_1_err
	@ echo "####################################################################"
	@ echo "# Try to get possible race conditions on threads in prodcons_1_err #"
	@ echo "# using the valgrind HELLGRIND tool.                               #"
	@ echo "####################################################################"

    # note: this is the same of (valgrind --tool=helgrind ./prodcons_1_err 1 )
	valgrind --tool=helgrind $(<D)/$< 1 
	
	@ echo 
	@ echo "####################################################################"
	@ echo "# it seems there is any detected error or race condition !         #"
	@ echo "# ... BUT TRY TO START THE PROGRAM WITH 2 THREADS:                 #" 
	@ echo "# is will get stuck with starvation ! ( press ctrl+C to exit )     #"
	@ echo "####################################################################"
	
    # note: this is the same of ( ./prodcons_1_err 2 )
	$(<D)/$< 2


test2: prodcons_2_err
	@ echo "####################################################################"
	@ echo "# even if valgrind reports no apparent errors, there are issues.   #"
	@ echo "# Try to repeat the operation several times ...                    #"
	@ echo "# the execution hangs, and sometimes not all values are evaluated  #"
	@ echo "####################################################################"

    # note: this is the same of ( valgrind --tool=helgrind ./prodcons_2_err 10 )
	valgrind --tool=helgrind $(<D)/$< 10

test3: prodcons_3_err
	@ echo "####################################################################"
	@ echo "# Here the program does not stop anymore.                          #"
	@ echo "# But sometimes we still not get all values properly evaluated     #"
	@ echo "####################################################################"

    # note: this is the same of ( ./prodcons_3_err 10 )
	$(<D)/$< 10

test4: prodcons_4_ok
	@ echo "####################################################################"
	@ echo "# Finally the program correctly works !                            #"
	@ echo "####################################################################"

    # note: this is the same of ( ./prodcons_3_err 10 )
	$(<D)/$< 10

test5: prodcons
	@ echo "####################################################################"
	@ echo "# PLAY WITH PRODCONS:                                              #"
	@ echo "#                                                                  #"
	@ echo "# 1) Try with prodcons without prints and look timings.            #"
	@ echo "# 2) Try to increase the buffer lenght and look the average time.  #"
	@ echo "# 3) Try to move the complex operation in and out the mutex lock.  #"
	@ echo "#                                                                  #"
	@ echo "####################################################################"

	@ echo
	@ echo "# one thread:"
	$(<D)/$< 1

	@ echo
	@ echo "# 10 threads:"
	$(<D)/$< 10
	@ echo



clean:
	rm -f $(PROGRAMS)

