cmake_minimum_required( VERSION 3.5.0 )

add_executable( main src/main.c src/lexer.c src/parser.c src/semantics.c src/symtab.c src/gencode.c )
