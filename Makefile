frog_game.out:frog_game.cpp
	g++ frog_game.cpp -lpthread

.phony: clean
clean:
	rm -rf *.out