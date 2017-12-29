SRCS = simpleloop.c matmul.c blocked.c midtermsim.c
PROGS = simpleloop matmul blocked midtermsim

all : $(PROGS)

$(PROGS) : % : %.c
	gcc -Wall -g -o $@ $<


traces: $(PROGS)
	./runit simpleloop
	./runit matmul 100
	./runit blocked 100 25
	./runit midtermsim 6

.PHONY: clean
clean : 
	rm -f simpleloop matmul blocked midtermsim tr-*.ref *.marker *~
