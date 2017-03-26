#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "mpi.h"

#define N 16	     /*ボードの大きさ*/

int COUNT = 0;
int TOTAL_COUNT = 0;

/*addtion*/
extern int connection_count=0;

int test_count = 0;
int test_total_count = 0;
extern int test=0;

/* (i,j)の位置から縦横斜めの方向にある board のすべての要素の値に dを加える */
void changeBoard(int board[N][N], int i, int j, int d)
{
    int k;
    
    for(k = 0; k < N; k++) { 
        board[i][k] += d;             /* 横方向 */
        board[k][j] += d;             /* 縦方向 */
    }

    if(i > j) {
        for(k = 0; k < N - (i - j); k++) {
            board[k + (i - j)][k] += d;   /* 右下がり斜め方向 (i > jのとき） */
        }
    }

    else {
        for(k = 0; k < N - (j - i); k++) {
            board[k][k + (j - i)] += d;   /* 右下がり斜め方向 (i <= jのとき） */
        }
    }

    if(i + j < N) {
        for(k = 0; k <= i + j; k++) {
            board[i + j - k][k] += d;     /* 左下がり斜め方向（i + j < Nのとき） */
        }
    }

    else {
        for(k = i + j - N + 1; k < N; k++) {
            board[i + j - k][k] += d;     /* 左下がり斜め方向（i + j >= Nのとき） */
        }
    }

}

/* i 行目のクイーンの位置を設定して， setQueen(queen, board, i+1) を呼び出す
　ただし， i == N のときは，一つの解が queen に入っているのでそれを表示するだけである */
void setQueen(int queen[N], int board[N][N], int i)
{
    int j;

    if(i == N) {                /* 解が見つかった */
        COUNT++;		/*解の総数をカウント*/
        test_count++;
        return;
    }
    
    for(j = 0; j < N; j++) {
        if(board[i][j] == 0) {                      /* (i,j) の位置に置けるならば */
            queen[i] = j;			    /* (i,j) の位置にクイーンを置く */
            changeBoard(board, i, j, + 1);	    /* (i,j) から縦横斜めの方向のboard値を+1する */
            setQueen(queen, board, i + 1);	    /* i + 1 行目のクイーンの位置を決める */
            changeBoard(board, i, j, - 1);	    /* (i,j) から縦横斜めの方向のboard値を元に戻す */
        }
    }
}

void setQueen_1(int rank, int size)
{
    int board[N][N];
    int queen[N];
    int j;
    int m, n;
   
    MPI_Status status;

    for(m = 0; m < N; m++) {
        for(n = 0; n < N; n++) {
	  board[m][n] = 0;
        }
    }

    for(m = 0; m < N; m++) {
      queen[m] = 0;
    }

    for(j = rank; j < N; j += size) {
        queen[0] = j;			                //(0,j) の位置にクイーンを置く
        changeBoard(&board[0], 0, j, + 1);	        //(0,j) から縦横斜めの方向のboard値を+1する
        setQueen(&queen[0], &board[0], 1);	        //i + 1 行目のクイーンの位置を決める
        changeBoard(&board[0], 0, j, - 1);	        //(0,j) から縦横斜めの方向のboard値を元に戻す
    }

    TOTAL_COUNT += COUNT;

    /*addtion*/
    
    test_total_count += test_count;
    

    
    
    MPI_Reduce(&COUNT, &TOTAL_COUNT, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    /*addtion*/
    if(rank==0){
                connection_count++;
    }

    
   
}

int main(int argc, char* argv[])
{
    int rank, size;

    int queen[N];
    int board[N][N];
    struct timeval st, ed;
    struct timeval ts,te;
    double time,time2;
    

    memset(board, 0, sizeof(board));

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    printf("start NQUEEN program at rank %d...\n", rank);

    gettimeofday(&st, NULL);
    setQueen_1(rank, size);
    gettimeofday(&ed, NULL);

    time = (ed.tv_sec - st.tv_sec) + (ed.tv_usec - st.tv_usec) * 1e-6;

    if(rank == 0) {
        printf("Number of Solution is ｢%d｣\n", TOTAL_COUNT);
        printf("Excution Time is ｢%.3f｣s\n", time);
        printf("connection_count=%d\n",connection_count);
        printf("Number of test_Solution is ｢%d｣\n", test_total_count);
        printf("next test connect\n");
    }
   
     
    MPI_Finalize();
    return 0;

}
