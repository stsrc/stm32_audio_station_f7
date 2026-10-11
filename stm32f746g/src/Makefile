TEMPLATEROOT = ..

# compilation flags for gdb

CFLAGS  = -O1 -g3 -Werror -Wall
ASFLAGS = -g3

# object files

OBJS= main.o $(STARTUP) $(ST_BIN) stm32f7xx_hal_timebase_tim.o stm32f7xx_it.o sd_diskio.o play.o
OBJS+= sample.o buffer_fill.o ts.o gui_tile.o synth.o

# include common make file

include $(TEMPLATEROOT)/Makefile.common

UNDER_TEST=buffer_fill.c synth.c
UNITTESTS=buffer_fill_unittest.c synth_unittest.c
UNITY_MAIN=../$(UNITY_SRC)/unity.c

test : $(UNITTESTS) $(UNDER_TEST)
	gcc -Wall -Werror -O0 -g3 -o test_runner_buffer_fill buffer_fill.c buffer_fill_unittest.c $(UNITY_MAIN) -I. -I../$(UNITY_SRC) 
	./test_runner_buffer_fill
	gcc -Wall -Werror -O0 -g3 -o test_runner_synth synth.c synth_unittest.c $(UNITY_MAIN) -I. -I../$(UNITY_SRC) -lm 
	./test_runner_synth

plot: synth.c synth_plot.c
	gcc -Wall -Werror -O0 -g3 -o plot synth.c synth_plot.c -I. -lm
	- rm ./sine.dat ./sine.gif ./triangle.dat ./triangle.gif 
	./plot
	gnuplot -persist -e "set terminal gif; plot 'sine.dat' with lines" > sine.gif
	gnuplot -persist -e "set terminal gif; plot 'triangle.dat' with lines" > triangle.gif
	gnuplot -persist -e "set terminal gif; plot 'triangle2.dat' with lines" > triangle2.gif
	gnuplot -persist -e "set terminal gif; plot 'square.dat' with lines" > square.gif
	gnuplot -persist -e "set terminal gif; plot 'sawtooth.dat' with lines" > sawtooth.gif
	sync
	xdg-open ./sine.gif
	xdg-open ./triangle.gif
	xdg-open ./triangle2.gif
	xdg-open ./square.gif
	xdg-open ./sawtooth.gif
