all: lex.yy.mdp.o mdp.cc
	g++ -Wall -std=c++2a -fconcepts lex.yy.mdp.o mdp.cc -ll -lfl -o interpretador
        
lex.yy.mdp.o: lex.yy.mdp.c 
	g++ -Wall -std=c++2a -fconcepts -c lex.yy.mdp.c
    
lex.yy.mdp.c: mdp.l 
	lex -o lex.yy.mdp.c mdp.l

clean:
	rm -f *.o lex.yy.mdp.c interpretador