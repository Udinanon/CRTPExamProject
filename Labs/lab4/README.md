# LAB4 - OPTIMIZATION

This labs is on Compiler Optimizations

## PREREQUISITES

* Graphviz ( tool to render the .dot files into a png image )

## TEST 1

* Show missed Optimizations (and inlining) compiling ackermann.c program
* Show GIMPLE, and other IR representations for ackermann.c

`make ackermann`


## TEST 2

compile a simple object code and dump optimizations passes

* show all graphs for the optimizations selected in env DUMP_TREES
* show all graphs for the optimizations selected in env DUMP_RTL

`make ackermann_function.o`

* render png images of those graphs

`make graphs`

## TEST 3

Compile a simple program that benchmark the time to evaluate the Sobel filtering of an image

`make test_gradient`

* Non optimized timing
`test_gradient test_image.pix test_image_borders.pix 0`

* Optimized timing
`test_gradient test_image.pix test_image_borders.pix 1`