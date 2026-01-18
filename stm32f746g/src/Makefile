TEMPLATEROOT = ..

# compilation flags for gdb

CFLAGS  = -O2 -g3 -Werror -Wall
ASFLAGS = -g3

# object files

OBJS= main.o $(STARTUP) $(ST_BIN) stm32f7xx_hal_timebase_tim.o stm32f7xx_it.o sd_diskio.o play.o
OBJS+= sample.o buffer_fill.o ts.o gui_tile.o

# include common make file

include $(TEMPLATEROOT)/Makefile.common

UNDER_TEST=buffer_fill.c
UNITTESTS=buffer_fill_unittest.c ../$(UNITY_SRC)/unity.c

test : $(UNITTESTS) $(UNDER_TEST)
	gcc -Wall -Werror -O0 -g3 -o test_runner $(UNDER_TEST) $(UNITTESTS) -I. -I../$(UNITY_SRC)
	./test_runner
