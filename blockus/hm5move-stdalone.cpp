#include <iostream>
#include <sstream>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include<mpi.h>


#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>

#include "board.h"
#include "search.h"
#include "opening.h"
using namespace std;

#define SERVER_HOST     "localhost"
#define PORT_NUMBER     8085

int server_fd;
int player_No;
bool now_playing = true;

int time_limit = 1000;
char *hostname = SERVER_HOST;
int port_no = PORT_NUMBER;
string first_move_fourCC = "";

int starting_turn_of_wld = 25;
int starting_turn_of_perfect = 27;

/* for MPI code */
int my_rank;
int p_num;
int count;
int count_sub;

Move parse_move(string fourcc)
{
    if (fourcc == "0000")
	return PASS;
    int x, y, d;
    char c;
    if (fourcc.length() == 4 &&
	sscanf(fourcc.c_str(), "%1X%1X%c%1d", &x, &y, &c, &d) == 4 &&
	x >= 1 && x <= 14 && y >= 1 && y <= 14 &&
	tolower(c) >= 'a' && tolower(c) <= 'u' &&
	d >= 0 && d <= 7)
	return Move(fourcc.c_str());
    else
	return INVALID_MOVE;
}

Move com_move(Board* b, int time_ms)
{
//printf("test\n");	
//cout<<my_rank<<" com_move"<<endl;
    Move move;
    int score = 100;
    quiet = true;

    move = opening_move(b);
    if (move == INVALID_MOVE) {
	SearchResult r;
#if 0   /* original */
	if (b->turn() < 25)
	    r = search_negascout(b, 10, time_ms / 2, time_ms);
	else if (b->turn() < 27)
	    r = wld(b, 1000);
	else
	    r = perfect(b);
#else
	if (b->turn() < starting_turn_of_wld){
	    r = search_negascout(b, 10, time_ms / 2, time_ms);
	}
	else if (b->turn() < starting_turn_of_perfect)
	    r = wld(b, 1000);
	else
	    r = perfect(b);
#endif
	move = r.first;
	score = r.second;
    }

    return move;
}


void connect_server(char *hostname, int port_no)
{
    struct sockaddr_in inet_addr;
    struct hostent *host;
    int len;

    //cout << "rank = " << my_rank << endl;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket open error\n");
        exit(1);
    }
    cout << "socket open OK." << endl;

    memset(&inet_addr, 0, sizeof(inet_addr));
	//printf("test1\n");
    inet_addr.sin_family = AF_INET;
	//printf("test2\n");
    host = gethostbyname(hostname);
	//printf("test3\n");
    memcpy(&inet_addr.sin_addr, host->h_addr, sizeof(inet_addr.sin_addr));
	//printf("test4\n");
    inet_addr.sin_port = htons(port_no);
	//printf("test5\n");

    len = sizeof(inet_addr);
	//printf("test6\n");
    if (connect(server_fd, (struct sockaddr *)&inet_addr, len) < 0) {
        perror("connect error\n");
        exit(1);
    }
    printf("server connect OK.\n");
}

void disconnect_server()
{
    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);
}

void send_fourcc(string fourcc)
{
    char sbuf[4];
    for (int i = 0; i < 4; i++)
        sbuf[i] = tolower(fourcc.c_str()[i]);
    write(server_fd, sbuf, 4);
//    write(server_fd, fourcc.c_str(), 4);
}

Move first_move(int player_No)
{
#define SIZEOF_ARRAY(ar) (sizeof(ar)/sizeof((ar)[0]))
    // borrowed from opening.cpp
    static const unsigned short moves[] = {
	0x0a45 /*56t2*/, 0x0054 /*65u0*/, 0x2c55 /*66p4*/, 0x3445 /*56o4*/,
	0x0e45 /*56t6*/, 0x3654 /*65o6*/, 0x0855 /*66t0*/, 0x1a53 /*64r2*/,
	0x0a44 /*55t2*/, 0x3264 /*75o2*/
#if 0
        /* ohshima's first move */
        0x1855 /*66r0*/, 0x1355 /*66s3*/
        /*56u0 is equivalent to 65u0, so isn't used */
        /*66t3 is equivalent to 66t0, so isn't used */
#endif
    };
// point symmetry pattern of player1 is bad choice
//    static const unsigned short moves2[] = {
//	0x0e98 /*a9t6*/, 0x0089 /*9au0*/, 0x2888 /*99p0*/, 0x3098 /*a9o0*/,
//	0x0a98 /*a9t2*/, 0x3289 /*9ao2*/, 0x0c88 /*99t4*/, 0x1e8a /*9br6*/,
//	0x0e99 /*aat6*/, 0x3679 /*8ao6*/
//#if 0
//        /* ohshima's first move */
//        0x1c88 /*99r4*/, 0x1388 /*99s3==99s7*/
//#endif
//    };
    if (first_move_fourCC != "")
        return parse_move(first_move_fourCC);

    int i = (int)((rand() / ((double)RAND_MAX+1.0f)) * SIZEOF_ARRAY(moves));
    return (unsigned short)(player_No == 1? moves[i]: moves[i] + 0x0055);
//    return (unsigned short)(player_No == 1? moves[i]: moves2[i]);
//    return moves[i];
#undef SIZEOF_ARRAY
}

void read_line(char *buf, int size)
{
    while (size > 0) {
        int readlen = read(server_fd, buf, size);
        if (readlen <= 0) {
            *buf = 0;
            if (readlen < 0)
                perror("read error\n");
            else
                printf("read EOF\n");
            break;
        }

        size -= readlen;
        buf += readlen;
    }
}

void process_opening_sequence(Board *b, Move *m)
{
    char ch;

    if (my_rank != 0) {
        cout << my_rank << " skips process_opening_seq." << endl;
        return;
    }
    
    cout << my_rank << " process_opening_seq." << endl;

    read_line(&ch, 1);
    if (ch != '0') {
        cout << "protocol error (code '0' is required)" << endl;
        exit(1);
    }

    // response to server with team name "XX"
    write(server_fd, "1XX", 3);

    read_line(&ch, 1);
    if (ch != '2') {
        cout << "protocol error (code '2' is required)" << endl;
        exit(1);
    }

    read_line(&ch, 1);
    if (ch == '5') {
        player_No = 1;
    }
    else if (ch == 'A') {
        player_No = 2;
    }
    else {
        cout << "protocol error (code '25' or '2A' is required)" << endl;
        exit(1);
    }

    *m = first_move(player_No);

    // response to server with first move
    send_fourcc(m->fourcc());
}

void update_board_after_opening_sequence(Board *b, Move first_m)
{
    assert(player_No == 1 || player_No == 2);

    if (player_No == 1) {
        char sbuf[1+4];

        if (my_rank == 0) {
            read_line(sbuf, 1+4);
            MPI_Bcast(sbuf,1+4,MPI_BYTE,0,MPI_COMM_WORLD);
         }
        else {
            MPI_Bcast(sbuf,1+4,MPI_BYTE,0,MPI_COMM_WORLD);
        }

        if (sbuf[0] != '3') {
            cout << "protocol error (code '3' is required)" << endl;
            exit(1);
        }

        for (int i = 0; i < 4; i++)
            sbuf[i] = sbuf[i+1];
        sbuf[4] = 0;
        sbuf[0] = toupper(sbuf[0]);
        sbuf[1] = toupper(sbuf[1]);
        Move opponent_m(sbuf);

        b->do_move(first_m);
        b->do_move(opponent_m);

        cout << "first " << first_m.fourcc() << endl;
        cout << "second " << opponent_m.fourcc() << endl;
    }
    else if (player_No == 2) {
        char sbuf[1+8];

        if (my_rank == 0) {
            read_line(sbuf, 1+8);
            MPI_Bcast(sbuf,1+8,MPI_BYTE,0,MPI_COMM_WORLD);
         }
        else {
            MPI_Bcast(sbuf,1+8,MPI_BYTE,0,MPI_COMM_WORLD);
        }

        if (sbuf[0] != '4') {
            cout << "protocol error (code '4' is required)" << endl;
            exit(1);
        }

        for (int i = 0; i < 4; i++)
            sbuf[i] = sbuf[i+1];
        sbuf[4] = 0;
        sbuf[0] = toupper(sbuf[0]);
        sbuf[1] = toupper(sbuf[1]);
        Move opponent_m(sbuf);

        for (int i = 0; i < 4; i++)
            sbuf[i] = sbuf[i+5];
        sbuf[4] = 0;
        sbuf[0] = toupper(sbuf[0]);
        sbuf[1] = toupper(sbuf[1]);
        Move opponent_m2(sbuf);

        b->do_move(opponent_m);
        b->do_move(first_m);
        b->do_move(opponent_m2);

        cout << "first " << opponent_m.fourcc() << endl;
        cout << "second " << first_m.fourcc() << endl;
        cout << "third " << opponent_m2.fourcc() << endl;
    }
}

void usage()
{
    cout << "command line options:\n";
    cout << "   -l <limit time>: time limit (msec) [default 1000]\n";
    cout << "   -h <server hostname>: server host [default 'localhost']\n";
    cout << "   -p <port number>: port number [default 8085]\n";
    cout << "   -s <start-move>: start-move (four CC)  [default NONE]\n";
    cout << "   -t <turn no. pair>: pair of turn number for wld and perfect"
        " [default 25:27]\n";
    cout << "   -h: this help\n";
    cout << endl;
}

void check_options(int ac, char *av[])
{
    int ch;
    char *endptr;

    while ((ch = getopt(ac, av, "l:h:p:s:t:?")) != -1) {
        switch (ch) {
        case 'l':
            time_limit = atoi(optarg);
            cout << "limit time = " << time_limit << endl;
            break;

        case 'h':
            hostname = optarg;
            cout << "hostname = " << hostname << endl;
            break;

        case 'p':
            port_no = atoi(optarg);
            cout << "portno = " << port_no << endl;
            break;

        case 's':
            first_move_fourCC = optarg;
            cout << "start move = " << first_move_fourCC << endl;
            if (parse_move(first_move_fourCC) == INVALID_MOVE) {
                cout << first_move_fourCC << " is invalid" << endl;
                cout << "(switching to default first move)" << endl;
                first_move_fourCC = "";
            }
            break;

        case 't':
            starting_turn_of_wld = strtol(optarg, &endptr, NULL);
            if (endptr != NULL) {
                if (*endptr == ':') {
                    starting_turn_of_perfect = strtol(endptr+1, &endptr, NULL);
                }
                else {
//                    cout << "':' is required" << endl;
                    starting_turn_of_perfect = starting_turn_of_wld;
                }
            }
            else {
                // if pair is not provided
                starting_turn_of_perfect = starting_turn_of_wld;
            }
            cout << "start wld from " << starting_turn_of_wld << endl;
            cout << "start perfect from " << starting_turn_of_perfect << endl;
            break;

        case '?':
        default:
            usage();
            exit(0);
        }
    }
}

int main(int argc, char *argv[])
{
    cout << "start main" << endl;

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size (MPI_COMM_WORLD, &p_num);

    //cout <<my_rank<< " main MPI_init done " << endl;

    check_options(argc, argv);
    
    nice(5);
    srand(time(NULL));
    if (my_rank == 0){
        atexit(disconnect_server);
        connect_server(hostname, port_no);
        cout << "main process " << my_rank << " connect to host" << endl;
    }

    Board board;
    Move first_m;		
	
    process_opening_sequence(&board, &first_m);
    //cout<<my_rank<<" process_opening done"<<endl;

    if (my_rank == 0) {
        unsigned short send_m;
        send_m = first_m.m();
        MPI_Bcast(&send_m,1,MPI_UNSIGNED_SHORT,0,MPI_COMM_WORLD);
    }
    else {
        unsigned short recv_m;
        MPI_Bcast(&recv_m,1,MPI_UNSIGNED_SHORT,0,MPI_COMM_WORLD);
        first_m.m()=recv_m;
    }
    MPI_Bcast(&player_No,1,MPI_INT,0,MPI_COMM_WORLD);

    update_board_after_opening_sequence(&board, first_m);
	
    //cout << my_rank << " update done" << endl;
    // barrier synchronization
    MPI_Barrier(MPI_COMM_WORLD);

    int count = 2;
    if(my_rank==0&&count==2){
    		int count_sub=0;
    }
    while (now_playing) {
        static string pass_fourcc("0000");
        // my turn
	
        Move my_move = com_move(&board, time_limit);
	if(my_rank==0)
        {
		cout << "my turn"<<endl;
		//cout << "my turn in " <<my_rank<<" "<<my_move.fourcc() << endl;
	}
        if (my_move.is_pass()) {
            send_fourcc(pass_fourcc);
            cout << "pass..." << endl;
        }
        else
            send_fourcc(my_move.fourcc());

        board.do_move(my_move);
	count++;
	if(my_rank==0)
	{
		cout<<count<<"手目"<<endl;
		//cout<< "rank " << my_rank << " " << count << " 手目" <<endl;
		MPI_Bcast(&count,1,MPI_INT,0,MPI_COMM_WORLD);
	}else{
		
		MPI_Bcast(&count_sub,1,MPI_INT,0,MPI_COMM_WORLD);
	}
	

        // opponent's turn
        char sbuf[1+4];

        if (my_rank == 0) {
            read_line(sbuf, 1+4);
            MPI_Bcast(sbuf,1+4,MPI_BYTE,0,MPI_COMM_WORLD);
        }
        else {
            MPI_Bcast(sbuf,1+4,MPI_BYTE,0,MPI_COMM_WORLD);
        }

        if (sbuf[0] == '9') {     // game end
            cout << "termination code received" << endl;
            now_playing = false;
         }
        else if (sbuf[0] == '3') {
            for (int i = 0; i < 4; i++)
                sbuf[i] = sbuf[i+1];
            sbuf[4] = 0;
            sbuf[0] = toupper(sbuf[0]);
            sbuf[1] = toupper(sbuf[1]);
            string fourcc(sbuf);

            Move opponent_m = parse_move(fourcc);
            if (opponent_m == INVALID_MOVE || !board.is_valid_move(opponent_m)) {
                cout << "XXXX invalid move " << fourcc << endl;
                now_playing = false;
            }
            else {
		  if(my_rank==0){
                cout << "opponent's turn " << opponent_m.fourcc() << endl;
		  }
                board.do_move(opponent_m);

                if (opponent_m.is_pass()) {
                    cout << "opponent pass" << endl;
                    if (my_move.is_pass()) {
                        cout << "both sides pass" << endl;
                        now_playing = false;
                    }
                }
            }
        }
        else {
            cout << "protocol error (code '3' or '9' is required)" << endl;
            exit(1);
        }
    }
	if (my_rank == 0)
    		disconnect_server();
	
    MPI_Finalize ();
    // board final state
    board.show();

    return 0;
}
