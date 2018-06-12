main : main.o lexer.o parser.o semantics.o symtab.o gencode.o
	cc -g -o main main.o lexer.o parser.o semantics.o symtab.o gencode.o

gencode.o : src/gencode.c
	cc -g -c src/gencode.c

semantics.o : src/semantics.c
	cc -g -c src/semantics.c

lexer.o : src/lexer.c
	cc -g -c src/lexer.c

parser.o : src/parser.c
	cc -g -c src/parser.c

main.o : src/main.c
	cc -g -c src/main.c

lxmain.o : src/lxmain.c
	cc -g -c src/lxmain.c

test : test.o symtab.o
	cc -g -o test test.o symtab.o

test.o : src/test.c
	cc -g -c src/test.c

symtab.o : src/symtab.c
	cc -g -c src/symtab.c
