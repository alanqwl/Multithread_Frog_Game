#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include <termios.h>
#include <fcntl.h>

#define ROW 10
#define COLUMN 50
#define SPEED 50000   // control the speed of log moving

struct Node{
	int x , y; 
	Node( int _x , int _y ) : x( _x ) , y( _y ) {}; 
	Node(){} ; 
} frog ; 

bool game_over = false;  // when game is over, game_status = true
int game_status = 0;	 // 0: unfinished, 1: win, 2: lose, 3: quit
char map[ROW+10][COLUMN] ; 
int log_head_idx[COLUMN];
bool log_move = false;

pthread_t threads[3] ;  // pthread id array with a capacity of 3
int thread_id[3] = {1, 2, 3};
pthread_mutex_t mutex;

/* some prototypes of useful global functions */
void map_show(void);

// Determine a keyboard is hit or not. If yes, return 1. If not, return 0. 
int kbhit(void){
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);

	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);

	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}
	return 0;
}


void *logs_move( void *t ){

	/*  Move the logs  */
	if ((long)t == 1){
		while(!game_over){
			int r, c;
			pthread_mutex_lock(&mutex);
			for (r = 1; r < ROW; ++r){
				if (r % 2 == 0){  // right shifting
					int k;
					char rightend = map[r][48];
					if (rightend == '0'){
						game_over = true;
						game_status = 2;
						pthread_mutex_unlock(&mutex);
						pthread_exit(NULL);
					}
					for (k = COLUMN - 2; k > 0; k--){
						if (map[r][k - 1] == '0'){
							frog.y += 1;
						}	
						map[r][k] = map[r][k - 1];	
					}
					map[r][0] = rightend;
				}else{  // left shifting
					int t;
					char leftend = map[r][0];
					if (leftend == '0'){
						game_over = true;
						game_status = 2;
						pthread_mutex_unlock(&mutex);
						pthread_exit(NULL);
					}
					for (t = 1; t < COLUMN - 1; t++){
						if (map[r][t] == '0'){
							frog.y -= 1;
						}	
						map[r][t - 1] = map[r][t];
					}
					map[r][48] = leftend;
				}
			}
			pthread_mutex_unlock(&mutex);
			usleep(SPEED);
			log_move = true;	
		}
		pthread_exit(NULL);
	}else if ((long)t == 2){
		/*  Print the map on the screen  */
		while(!game_over){
			if (log_move){
				map_show();
				log_move = !log_move;
			}	
		}
		pthread_exit(NULL);
	}else if ((long)t == 3){
		/*  Check keyboard hits, to change frog's position or quit the game. */
		while (!game_over){
			bool frog_move = false;
			if (kbhit()){
				char key = getchar();
				pthread_mutex_lock(&mutex);
				if (key == 'q' || key == 'Q'){
					game_over = true;
					game_status = 3;
					pthread_mutex_unlock(&mutex);
					break;
				}

				if (key == 'a' || key == 'A'){
					if (frog.x == ROW){
						map[frog.x][frog.y] = '|';
					}else{
						map[frog.x][frog.y] = '=';
					}
					frog.y -= 1;

					// The frog is out of the boundary.
					if (frog.y < 0){
						game_over = true;
						game_status = 2;
						pthread_mutex_unlock(&mutex);
						break;
					}

					// The frog jumps into the river.
					if (frog.x != 0 && frog.x != ROW && (map[frog.x][frog.y] != '=')){
						game_over = true;
						game_status = 2;
						pthread_mutex_unlock(&mutex);
						break;
					}
					map[frog.x][frog.y] = '0';
					frog_move = true;
				}

				if (key == 'd' || key == 'D'){
					if (frog.x == ROW){
						map[frog.x][frog.y] = '|';
					}
					else{
						map[frog.x][frog.y] = '=';
					}
					
					frog.y += 1;
					if (frog.y > 48){
						game_over = true;
						game_status = 2;
						pthread_mutex_unlock(&mutex);
						break;
					}
					
					if (frog.x != 0 && frog.x != ROW && (map[frog.x][frog.y] != '=')){
						game_over = true;
						game_status = 2;
						pthread_mutex_unlock(&mutex);
						break;
					}
					frog_move = true;
					map[frog.x][frog.y] = '0';
				}

				if (key == 'w' || key == 'W'){

					if (ROW - 1 < frog.x < ROW){
						map[ROW][frog.y] = '|';
					}
					if (frog.x < ROW){
						map[frog.x][frog.y] = '=';
					}
					
					frog.x -= 1;
					if (frog.x < 1){
						game_over = true;
						game_status = 1;
						pthread_mutex_unlock(&mutex);
						break;
					}

					if (frog.x != ROW && (map[frog.x][frog.y] != '=')){
						game_over = true;
						game_status = 2;
						pthread_mutex_unlock(&mutex);
						break;
					}
					frog_move = true;
					map[frog.x][frog.y] = '0';
				}

				if (key == 's' || key == 'S'){

					if (frog.x <= ROW - 1){
						map[frog.x][frog.y] = '=';
					}
					frog.x += 1;
					if (frog.x > ROW){
						game_over = true;
						game_status = 2;
						pthread_mutex_unlock(&mutex);
						break;
					}

					if (frog.x != ROW && (map[frog.x][frog.y] != '=')){
						game_over = true;
						game_status = 2;
						pthread_mutex_unlock(&mutex);
						break;
					}
					frog_move = true;
					map[frog.x][frog.y] = '0';
				}
				pthread_mutex_unlock(&mutex);
				if (frog_move){
					map_show();
				}
			}
		}
		pthread_exit(NULL);	
	}
	/*  Check game's status  */
	
}

// Print the map into screen
void map_show(void){
	system("clear");
	int i;
	for( i = 0; i <= ROW; ++i)	
		puts( map[i] );
}

int main( int argc, char *argv[] ){
	int rc1, rc2;
	int rc3;
	pthread_attr_t attr_1;
	pthread_mutex_init(&mutex, NULL);

	// Initialize the river map and frog's starting position
	memset( map , 0, sizeof( map ) ) ;
	int i , j ;
	int m; 
	int log_start_init;
	int log_length[9];
	for( i = 1; i < ROW; ++i ){	
		srand((unsigned)time(NULL) + i);  // randomly select the starting position of the logs
		log_start_init = rand() % 49;
		log_head_idx[i] = log_start_init;
		for( m = 0; m < 9; m++){
			log_length[m] = (rand() % (15 - 7 + 1)) + 7;
		}
		for( j = 0; j < COLUMN - 1; j++ ){
			int length = log_length[i - 1];
			if (log_start_init > 49 - length){
				if (j >= log_start_init || j <= (log_start_init + length - 1) % 49){
					map[i][j] = '=';
				}else{
					map[i][j] = ' ';
				}			
			}
			
			else if ((j >= log_start_init) && (j <= log_start_init + length - 1)){
				map[i][j] = '=';
			}
	
			else{
				map[i][j] = ' ' ;
			} 
		}	
			 
	}	

	for( j = 0; j < COLUMN - 1; ++j )	
		map[ROW][j] = map[0][j] = '|' ;

	for( j = 0; j < COLUMN - 1; ++j )	
		map[0][j] = map[0][j] = '|' ;

	frog = Node( ROW, (COLUMN-1) / 2 ) ; 
	map[frog.x][frog.y] = '0' ;

	map_show();

	/*  Create pthreads for wood move and frog control.  */
	/* preset the pthread attribute */
	pthread_attr_init(&attr_1);
	pthread_attr_setdetachstate(&attr_1, PTHREAD_CREATE_JOINABLE);

	/* create threads*/
	rc1 = pthread_create(&threads[0], &attr_1, logs_move, (void*)thread_id[0]);
	rc2 = pthread_create(&threads[1], &attr_1, logs_move, (void*)thread_id[1]);
	rc3 = pthread_create(&threads[2], &attr_1, logs_move, (void*)thread_id[2]);
	if (rc1 != 0){
		printf("Error: %d", rc1);
		exit(1);
	}
	if (rc2 != 0){
		printf("Error: %d", rc2);
		exit(1);
	}
	if (rc3 != 0){
		printf("Error: %d", rc3);
		exit(1);
	}

	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	pthread_join(threads[2], NULL);
	
	/*  Display the output for user: win, lose or quit.  */

	system("clear");
	if (game_status == 1){
		printf("You win the game!!\n");
	}else if (game_status == 2){
		printf("You lose the game!!\n");
	}else if (game_status == 3){
		printf("You exit the game.\n");
	}

	pthread_mutex_destroy(&mutex);
	pthread_exit(NULL);
	return 0;
}
