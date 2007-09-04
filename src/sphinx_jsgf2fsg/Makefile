all: jsgfparser

CPPFLAGS := -DYYDEBUG=1
CFLAGS   := -g -Wall `pkg-config --cflags sphinxbase`
LIBS     := `pkg-config --libs sphinxbase`

jsgfparser: jsgf.tab.o jsgf.lex.o jsgf_impl.o main.o
	gcc -o $@ jsgf.lex.o jsgf.tab.o jsgf_impl.o main.o $(LIBS)

jsgf.lex.h jsgf.lex.c: jsgf.l jsgf.h
	flex -o $@ $<

jsgf.tab.h jsgf.tab.c: jsgf.y jsgf.h
	bison -d $< -o $@

jsgf.tab.o: jsgf.tab.c jsgf.lex.h

jsgf_impl.o: jsgf_impl.c jsgf.h

main.o: main.c jsgf.h

clean:
	$(RM) *.tab.* *.o *.lex.* jsgfparser
