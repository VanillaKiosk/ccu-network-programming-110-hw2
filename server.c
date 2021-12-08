#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define PORT 5000
#define MAXLINE 1024

// Change this to a variable if users are allowed to register
#define USERCOUNT 5

#define MAXBATTLE 5
#define MAXCONN 10

typedef struct user
{
    char *username;
    char *password;
} User;

static const User users[USERCOUNT] = {
    {.username = "alice", .password = "alice123"},
    {.username = "bob", .password = "bob123"},
    {.username = "charlie", .password = "charlie123"},
    {.username = "dave", .password = "dave123"},
    {.username = "eve", .password = "eve123"},
};

typedef struct battleData BattleData;
typedef struct connMetadata ConnMetadata;

void printNumpad() {
    printf(" ");
    printf("1");
    printf(" ");
    printf("|");
    printf(" ");
    printf("2");
    printf(" ");
    printf("|");
    printf(" ");
    printf("3");
    printf(" ");
    printf("\n-----------\n");
    printf(" ");
    printf("4");
    printf(" ");
    printf("|");
    printf(" ");
    printf("5");
    printf(" ");
    printf("|");
    printf(" ");
    printf("6");
    printf(" ");
    printf("\n-----------\n");
    printf(" ");
    printf("7");
    printf(" ");
    printf("|");
    printf(" ");
    printf("8");
    printf(" ");
    printf("|");
    printf(" ");
    printf("9");
    printf(" ");
    printf("\n");
    return;
}

int max(int x, int y)
{
    if (x > y)
        return x;
    else
        return y;
}

char *getword(char *str, char *word) {
    char *p = str;
    char *q = word;

    *q = '\0';

    while (*p == ' ')
        p++;
    
    while (!(*p == ' ' || *p == '\0'))
        *q++ = *p++;
    
    if (q == word)
        return NULL;

    *q = '\0';

    return p;
}

struct battleData {
    int launched;
    int turn;
    ConnMetadata *cm1;
    ConnMetadata *cm2;
    int data[9];
};

BattleData battles[MAXBATTLE];

int clearBattle(BattleData *battle) {
    battle->launched = 0;
    battle->turn = -1;
    battle->cm1 = NULL;
    battle->cm2 = NULL;
    for(int i = 0; i < 9; i++) {
        battle->data[i] = -1;
    }

    return 0;
}

int initBattles(BattleData * battles) {
    for(int i = 0; i < MAXBATTLE; i++) {
        clearBattle(&battles[i]);
    }

    return 0;
}

// Return battle address if create successfully, NULL if failed
BattleData *createBattle(ConnMetadata *cm1, ConnMetadata *cm2) {
    BattleData *battle = NULL;

    for(int i = 0; i < MAXBATTLE; i++) {
        if(battles[i].launched == 0) {
            battle = &battles[i];
            battle->launched = 1;
            battle->turn = 0;
            battle->cm1 = cm1;
            battle->cm2 = cm2;
            break;
        }
    }

    return battle;
}

// 0=continue, 1=cm1 win, 2=cm2 win, 3=par
int battleJudge(BattleData *battle) {
    int *data = battle->data;

    if((data[0] != -1) &&
       (data[1] != -1) &&
       (data[2] != -1) &&
       (data[3] != -1) &&
       (data[4] != -1) &&
       (data[5] != -1) &&
       (data[6] != -1) &&
       (data[7] != -1) &&
       (data[8] != -1)) {
        return 3;
    }

    if((data[0] == data[1]) && (data[1] == data[2]) && (data[2] != -1)) {
        return data[2]+1;
    }
    if((data[3] == data[4]) && (data[4] == data[5]) && (data[5] != -1)) {
        return data[5]+1;
    }
    if((data[6] == data[7]) && (data[7] == data[8]) && (data[8] != -1)) {
        return data[8]+1;
    }

    if((data[0] == data[3]) && (data[3] == data[6]) && (data[6] != -1)) {
        return data[6]+1;
    }
    if((data[1] == data[4]) && (data[4] == data[7]) && (data[7] != -1)) {
        return data[7]+1;
    }
    if((data[2] == data[5]) && (data[5] == data[8]) && (data[8] != -1)) {
        return data[8]+1;
    }

    if((data[0] == data[4]) && (data[4] == data[8]) && (data[8] != -1)) {
        return data[8]+1;
    }
    if((data[2] == data[4]) && (data[4] == data[6]) && (data[6] != -1)) {
        return data[6]+1;
    }

    return 0;
}


int closeBattle(BattleData *battle) {
    clearBattle(battle);

    return 0;
}

typedef enum CS_STATUS {CS_INVALID, CS_GUEST, CS_IDLE, CS_INVITER, CS_INVITEE, CS_BATTLE} CS_STATUS;

struct connMetadata {
    CS_STATUS status;
    int connfd;
    int userid;
    ConnMetadata *inviter;
    ConnMetadata *invitee;
    BattleData *battle;
};

ConnMetadata connMetadatas[MAXCONN];

void clearConnMetadata(ConnMetadata *cm) {
    cm->status = CS_INVALID;
    cm->connfd = -1;
    cm->userid = -1;
    cm->inviter = NULL;
    cm->invitee = NULL;
    cm->battle = NULL;
    return;
}

void initConnMetadatas(ConnMetadata *cm) {
    for(int i = 0; i < MAXCONN; i++) {
        clearConnMetadata(&cm[i]);
    }

    return;
}


ConnMetadata *createConnMetadata(int connfd) {
    ConnMetadata *cm = NULL;

    for(int i = 0; i < MAXCONN; i++) {
        if (connMetadatas[i].status == CS_INVALID) {
            cm = &connMetadatas[i];
            cm->status = CS_GUEST;
            cm->connfd = connfd;
            break;
        }
    }

    return cm;
}

ConnMetadata *getConnMetadataByConn(int connfd) {
    ConnMetadata *cm = NULL;

    for(int i = 0; i < MAXCONN; i++) {
        printf("->connfd %d status %d\n", connMetadatas[i].connfd, connMetadatas[i].status);
        if (connMetadatas[i].status != CS_INVALID && connMetadatas[i].connfd == connfd) {
            cm = &connMetadatas[i];
            break;
        }
    }

    return cm;
}

ConnMetadata *getConnMetadataByUser(int userid) {
    ConnMetadata *cm = NULL;

    for(int i = 0; i < MAXCONN; i++) {
        if (connMetadatas[i].status != CS_INVALID && connMetadatas[i].userid == userid) {
            cm = &connMetadatas[i];
            break;
        }
    }

    return cm;
}




int connectHandler(int connfd) {
    static const char *msg = "Welcome!\nPlease type '<username> <password>' to login.\n\n";
    // enroll in connMetas and check if success
    if (createConnMetadata(connfd) == NULL)
        return -1;
    
    // send welcome msg
    write(connfd, msg, strlen(msg)+1);
    
    return 0;
}


int disconnectHandler(ConnMetadata *cm) {
    ConnMetadata *peer = NULL;
    static const char *inviteCancelmsg = "Oops!\nThe peer seems disconnected.\nThe invitation has been canceled.\nType 'list' to view online users, 'invite <userid>' to invite a user.\n\n";
    static const char *battleCancelmsg = "Oops!\nThe peer seems disconnected.\nThe battle has been canceled.\nType 'list' to view online users, 'invite <userid>' to invite a user.\n\n";
    // check if in battle or invite, if so, finish the battle or calcel the invite
    if(cm->status == CS_INVITER) {
        peer = cm->invitee;
        write(peer->connfd, inviteCancelmsg, strlen(inviteCancelmsg)+1);
        peer->inviter = NULL;
        peer->invitee = NULL;
        peer->status = CS_IDLE;
    }
    else if(cm->status == CS_INVITEE) {
        peer = cm->inviter;
        write(peer->connfd, inviteCancelmsg, strlen(inviteCancelmsg)+1);
        peer->inviter = NULL;
        peer->invitee = NULL;
        peer->status = CS_IDLE;
    }
    else if(cm->status == CS_BATTLE) {
        if(cm->battle->cm1 == cm) {
            peer = cm->battle->cm2;
        }
        else {
            peer = cm->battle->cm1;
        }
        write(peer->connfd, battleCancelmsg, strlen(battleCancelmsg)+1);
        clearBattle(cm->battle);
        peer->battle = NULL;
        peer->status = CS_IDLE;
    }


    // clear meta
    clearConnMetadata(cm);

    return 0;
}

int guestHandler(ConnMetadata *cm, char *buffer) {
    char *p = buffer;
    char *username = malloc(MAXLINE);
    char *password = malloc(MAXLINE);

    int userid = -1;

    static const char *invalidFormatmsg = "Sorry!\nInvalid login format.\n\n";
    static const char *invalidCredmsg = "Sorry!\nWrong username or password.\n\n";
    static const char *otherLoginmsg = "Sorry!\nYou can only login at one device at the same time.\n\n";
    static const char *usagemsg = "Login successful!\nYou can use 'list' or 'invite <userid>' or 'logout' command.\n\n";

    // parse login parameter
    p = getword(p, username);

    if(p == NULL) {
        write(cm->connfd, invalidFormatmsg, strlen(invalidFormatmsg)+1);
        free(username);
        free(password);
        return -1;
    }

    p = getword(p, password);

    if(p == NULL) {
        write(cm->connfd, invalidFormatmsg, strlen(invalidFormatmsg)+1);
        free(username);
        free(password);
        return -1;
    }

    // check id
    for (int i = 0; i < USERCOUNT; i++) {
        if(strcmp(users[i].username, username) == 0){
            if(strcmp(users[i].password, password) == 0) {
                userid = i;
            }
            break;
        }
    }

    if(userid < 0) {
        write(cm->connfd, invalidCredmsg, strlen(invalidCredmsg)+1);
        free(username);
        free(password);
        return -1;
    }

    if(getConnMetadataByUser(userid) != NULL) {
        write(cm->connfd, otherLoginmsg, strlen(otherLoginmsg)+1);
        free(username);
        free(password);
        return -1;
    }

    // if success
    cm->userid = userid;
    cm->status = CS_IDLE;

    // print "list" or "invite" command hint
    write(cm->connfd, usagemsg, strlen(usagemsg)+1);
    free(username);
    free(password);

    return 0;
}

int idleUserHandler(ConnMetadata *cm, char *buffer) {
    char *p = buffer;
    char *opstr = malloc(MAXLINE);
    char *connfdstr = malloc(MAXLINE);
    char *outputbuff = malloc(MAXLINE);
    ConnMetadata *peer = NULL;

    outputbuff[0] = '\0';

    int connfd;

    static const char *invalidFormatmsg = "Sorry!\nInvalid command format.\n\n";
    static const char *invalidConnnfdmsg = "Sorry!\nInvalid id.\n\n";
    static const char *inviteSentmsg = "Invitation sent!\nWaiting for reply. Or send any key stroke to cancel.\n\n";
    static const char *inviteRecvmsg = "Invitation received!\nSend 'Y' to accept and start battle. Send other key stroke to reject.\n\n";
    static const char *inviteInvalidmsg = "Sorry!\nYou cannot invite yourself.\n\n";
    static const char *inviteBusymsg = "Sorry!\nThe user is busy.\n\n";
    static const char *logoutmsg = "Logout!\nYou are now logout.\n\n";

    p = getword(p, opstr);

    if(p == NULL) {
        write(cm->connfd, invalidFormatmsg, strlen(invalidFormatmsg)+1);
        free(opstr);
        free(connfdstr);
        free(outputbuff);
        return -1;
    }

    if(strcmp(opstr, "list") == 0) {
        p = outputbuff;
        strcpy(outputbuff, "Online List\n");
        p += strlen("Online List\n");
        strcat(outputbuff, "id\tname\n");
        p += strlen("id\tname\n");
        for(int i = 0; i < MAXCONN; i++) {
            if(connMetadatas[i].status != CS_INVALID && connMetadatas[i].status != CS_GUEST)
                p += sprintf(p, "%d\t%s\n", connMetadatas[i].connfd, users[connMetadatas[i].userid].username);
        }
        *p++ = '\n';
        *p++ = '\0';
        write(cm->connfd, outputbuff, strlen(outputbuff)+1);
        free(opstr);
        free(connfdstr);
        free(outputbuff);
        return 0;
    }
    else if(strcmp(opstr, "invite") == 0) {
        p = getword(p, connfdstr);
        if(p == NULL) {
            write(cm->connfd, invalidFormatmsg, strlen(invalidFormatmsg)+1);
            free(opstr);
            free(connfdstr);
            free(outputbuff);
            return -1;
        }
        connfd = atoi(connfdstr);
        peer = getConnMetadataByConn(connfd);
        if(peer == NULL) {
            write(cm->connfd, invalidConnnfdmsg, strlen(invalidConnnfdmsg)+1);
            free(opstr);
            free(connfdstr);
            free(outputbuff);
            return -1;
        }
        else if(peer == cm) {
            write(cm->connfd, inviteInvalidmsg, strlen(inviteInvalidmsg)+1);
            free(opstr);
            free(connfdstr);
            free(outputbuff);
            return -1;
        }
        else if(peer->status != CS_IDLE) {
            write(cm->connfd, inviteBusymsg, strlen(inviteBusymsg)+1);
            free(opstr);
            free(connfdstr);
            free(outputbuff);
            return -1;
        }
        sprintf(outputbuff, "Invitation received from %s!\nSend 'Y' to accept and start battle. Send other key stroke to reject.\n\n", users[cm->userid].username);
        cm->status = CS_INVITER;
        cm->inviter = cm;
        cm->invitee = peer;
        peer->status = CS_INVITEE;
        peer->inviter = cm;
        peer->invitee = peer;
        write(cm->connfd, inviteSentmsg, strlen(inviteSentmsg)+1);
        write(peer->connfd, outputbuff, strlen(outputbuff)+1);
        free(opstr);
        free(connfdstr);
        free(outputbuff);
        return 0;
    }
    else if(strcmp(opstr, "logout") == 0) {
        cm->status = CS_GUEST;
        cm->userid = -1;
        write(cm->connfd, logoutmsg, strlen(logoutmsg)+1);
        free(opstr);
        free(connfdstr);
        free(outputbuff);
    }
    else{
        write(cm->connfd, invalidFormatmsg, strlen(invalidFormatmsg)+1);
        free(opstr);
        free(connfdstr);
        free(outputbuff);
        return -1;
    }

    return 0;
}

int inviterHandler(ConnMetadata *cm, char *buffer) {
    static const char *cancelmsg = "Invitation has been canceled!\n\n";

    ConnMetadata *peer = cm->invitee;

    cm->status = CS_IDLE;
    cm->inviter = NULL;
    cm->invitee = NULL;

    peer->status = CS_IDLE;
    peer->inviter = NULL;
    peer->invitee = NULL;
    
    write(cm->connfd, cancelmsg, strlen(cancelmsg)+1);
    write(peer->connfd, cancelmsg, strlen(cancelmsg)+1);

    return 0;
}

void battleGridInfoSend(BattleData *battle, int showinfo) {
    char *grid = malloc(MAXLINE);
    char *output1 = malloc(MAXLINE);
    char *output2 = malloc(MAXLINE);

    output1[0] = '\0';
    output2[0] = '\0';

    ConnMetadata *cm1 = battle->cm1;
    ConnMetadata *cm2 = battle->cm2;

    static const char *yourturnmsg = "It's your turn!\n\n";
    static const char *waitpeermsg = "Waiting peer...\n\n";

    sprintf(grid, " %c | %c | %c \n-----------\n %c | %c | %c \n-----------\n %c | %c | %c \n",
    battle->data[0] == 0 ? 'O' : battle->data[0] == 1 ? 'X' : ' ',
    battle->data[1] == 0 ? 'O' : battle->data[1] == 1 ? 'X' : ' ',
    battle->data[2] == 0 ? 'O' : battle->data[2] == 1 ? 'X' : ' ',
    battle->data[3] == 0 ? 'O' : battle->data[3] == 1 ? 'X' : ' ',
    battle->data[4] == 0 ? 'O' : battle->data[4] == 1 ? 'X' : ' ',
    battle->data[5] == 0 ? 'O' : battle->data[5] == 1 ? 'X' : ' ',
    battle->data[6] == 0 ? 'O' : battle->data[6] == 1 ? 'X' : ' ',
    battle->data[7] == 0 ? 'O' : battle->data[7] == 1 ? 'X' : ' ',
    battle->data[8] == 0 ? 'O' : battle->data[8] == 1 ? 'X' : ' '
    );

    strcat(output1, grid);
    strcat(output2, grid);
    if(showinfo == 1) {
        strcat(output1, yourturnmsg);
        strcat(output2, waitpeermsg);
    }

    if(battle->turn == 0) {
        write(cm1->connfd, output1, strlen(output1)+1);
        write(cm2->connfd, output2, strlen(output2)+1);
    }
    else if(battle->turn == 1) {
        write(cm2->connfd, output1, strlen(output1)+1);
        write(cm1->connfd, output2, strlen(output2)+1);
    }

    free(grid);
    free(output1);
    free(output2);

    return;
}

int inviteeHandler(ConnMetadata *cm, char *buffer) {
    char *p = buffer;
    char *word = malloc(MAXLINE);
    BattleData * battle = NULL;

    static const char *rejectmsg = "Invitation rejected!\n\n";
    static const char *roomfullmsg = "Battle room is full!\n\n";
    static const char *battlemsg = "Battle start!\nType in the number of grid to put your chess.\n";
    static const char *gridnum = " 1 | 2 | 3 \n-----------\n 4 | 5 | 6 \n-----------\n 7 | 8 | 9 \n";

    ConnMetadata *peer = cm->inviter;

    // parse 
    p = getword(p, word);
    if(p == NULL) {
        return -1;
    }

    if(strcmp(word, "Y") == 0) {
        battle = createBattle(peer, cm);
        if(battle == NULL)
        {
            printf("\n Error: Battle room is full. \n");
            cm->status = CS_IDLE;
            cm->inviter = NULL;
            cm->invitee = NULL;

            peer->status = CS_IDLE;
            peer->inviter = NULL;
            peer->invitee = NULL;

            write(cm->connfd, roomfullmsg, strlen(roomfullmsg)+1);
            write(peer->connfd, roomfullmsg, strlen(roomfullmsg)+1);

            free(word);
            return -1;
        }
        cm->battle = battle;
        cm->invitee = NULL;
        cm->inviter = NULL;
        cm->status = CS_BATTLE;
        peer->battle = battle;
        peer->invitee = NULL;
        peer->inviter = NULL;
        peer->status = CS_BATTLE;

        write(cm->connfd, battlemsg, strlen(battlemsg)+1);
        write(peer->connfd, battlemsg, strlen(battlemsg)+1);
        write(cm->connfd, gridnum, strlen(gridnum)+1);
        write(peer->connfd, gridnum, strlen(gridnum)+1);
        sprintf(word, "You are %c. %s is %c.\n", 'X', users[peer->userid].username, 'O');
        write(cm->connfd, word, strlen(word)+1);
        sprintf(word, "You are %c. %s is %c.\n", 'O', users[cm->userid].username, 'X');
        write(peer->connfd, word, strlen(word)+1);
        battleGridInfoSend(cm->battle, 1);
    }
    else {
        cm->status = CS_IDLE;
        cm->inviter = NULL;
        cm->invitee = NULL;

        peer->status = CS_IDLE;
        peer->inviter = NULL;
        peer->invitee = NULL;

        write(cm->connfd, rejectmsg, strlen(rejectmsg)+1);
        write(peer->connfd, rejectmsg, strlen(rejectmsg)+1);
    }

    free(word);

    return 0;
}

int battleHandler(ConnMetadata *cm, char *buffer) {
    char *p = buffer;
    char *word = malloc(MAXLINE);
    int pos = 0;
    int judgeresult = 0;

    BattleData *battle = cm->battle;

    static const char *invalidmsg = "Invalid move!\n\n";
    static const char *notyourturnmsg = "Not your turn!\n\n";
    static const char *youwinmsg = "Congratulations! You Win!\n\n";
    static const char *youlosemsg = "Uh! You lose!\n\n";
    static const char *parmsg = "Par! But it's still a nice show.\n\n";

    // parse
    p = getword(p, word);
    if(p == NULL) {
        free(word);
        return -1;
    }
    
    pos = atoi(word);
    if(pos <= 1 || pos >=9) {
        //quit
    }

    pos -= 1;

    // if not quit, check whose turn
    if ((cm == battle->cm1 && battle->turn == 0) || (cm == battle->cm2 && battle->turn == 1)) {
        if(battle->data[pos] != -1) {
            write(cm->connfd, invalidmsg, strlen(invalidmsg)+1);
            free(word);
            return -1;
        }
        battle->data[pos] = battle->turn;
        battle->turn = battle->turn == 0 ? 1 : 0;
        judgeresult = battleJudge(battle);
        if(judgeresult != 0) {
            battleGridInfoSend(battle,0);
        }
        else {
            battleGridInfoSend(battle,1);
        }
    }
    // if not my turn, send msg
    else {
        write(cm->connfd, notyourturnmsg, strlen(notyourturnmsg)+1);
    }

    if(judgeresult != 0) {
        if(judgeresult == 1) {
            write(battle->cm1->connfd, youwinmsg, strlen(youwinmsg)+1);
            write(battle->cm2->connfd, youlosemsg, strlen(youlosemsg)+1);
        }
        else if(judgeresult == 2) {
            write(battle->cm2->connfd, youwinmsg, strlen(youwinmsg)+1);
            write(battle->cm1->connfd, youlosemsg, strlen(youlosemsg)+1);
        }
        else if(judgeresult == 3) {
            write(battle->cm1->connfd, parmsg, strlen(parmsg)+1);
            write(battle->cm2->connfd, parmsg, strlen(parmsg)+1);
        }

        battle->cm1->status = CS_IDLE;
        battle->cm1->battle = NULL;
        battle->cm2->status = CS_IDLE;
        battle->cm2->battle = NULL;
        clearBattle(battle);
    }

    free(word);

    return 0;
}

int dispatcher(int connfd, char *inputbuff, int buffsize) {
    ConnMetadata *cm;
    CS_STATUS cs;

    cm = getConnMetadataByConn(connfd);
    if(cm == NULL) {
        printf("\n Error: Can't getConnMetadataByConn! \n");
        exit(1);
    }

    cs = cm->status;

    if(buffsize == 0) { // Client lost connection
        printf("\n Info: Client lost connection \n");
        disconnectHandler(cm);

        return -1;
    }

    switch (cs)
    {
    case CS_INVALID:
        printf("\n Error: Invalid CS_STATUS \n");
        break;
    case CS_GUEST:
        guestHandler(cm, inputbuff);
        break;
    case CS_IDLE:
        idleUserHandler(cm, inputbuff);
        break;
    case CS_INVITER:
        inviterHandler(cm, inputbuff);
        break;
    case CS_INVITEE:
        inviteeHandler(cm, inputbuff);
        break;
    case CS_BATTLE:
        battleHandler(cm, inputbuff);
        break;
    default:
        printf("\n Error: Unknown CS_STATUS \n");
        break;
    }
    return 0;
}

int main()
{
    int listenfd, connfd, readyfd, maxfd, orimaxfd;
    char buffer[MAXLINE];
    fd_set fdset, tmpfdset;
    ssize_t readlen;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;

    initConnMetadatas(connMetadatas);
    initBattles(battles);

    /* create listening TCP socket */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // binding server addr structure to listenfd
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    listen(listenfd, MAXCONN);

    // clear the descriptor set
    FD_ZERO(&fdset);

    // add listener to fdset
    FD_SET(listenfd, &fdset);

    // get maxfd
    maxfd = listenfd + 1;
    while(1)
    {
        memcpy(&tmpfdset, &fdset, sizeof(fdset));

        // select the ready descriptor
        readyfd = select(maxfd, &tmpfdset, NULL, NULL, NULL);

        memset(buffer, 0, sizeof(buffer));

        if(readyfd < 0) {
            if (errno == EINTR)
                continue;
            printf("\n Error: Select fd failed! \n");
            exit(1);
        }

        for(int i = 0; i < maxfd; i++) {
            if(FD_ISSET(i, &tmpfdset)) {
                // New connection!
                if(i == listenfd) {
                    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
                    printf("\n Info: %d connect! \n", connfd);

                    if(connfd < 0) {
                        printf("\n Error: Accept connection failed! \n");
                        exit(1);
                    }

                    FD_SET(connfd, &fdset);

                    if(connfd + 1 > maxfd)
                        maxfd = connfd + 1;
                    
                    // Handle start from here
                    if (connectHandler(connfd) < 0) {
                        printf("\n Error: Too many connections! \n");
                        exit(1);
                    }
                }
                // Exist connection
                else {
                    readlen = read(i, buffer, sizeof(buffer));

                    // Connection closed
                    if(readlen == 0) {
                        printf("\n Info: Client %d lost connection! \n", i);

                        close(i);

                        FD_CLR(i, &fdset);

                        if(i + 1 == maxfd) {
                            orimaxfd = maxfd;
                            for(int j = 0; j < orimaxfd; j++) {
                                if (FD_ISSET(j, &fdset)) {
                                    maxfd = j + 1;
                                }
                            }
                        }

                        // Handle start from here
                        dispatcher(i, buffer, 0);
                    }
                    // Data Received
                    else {
                        // Handle start from here
                        if(buffer[readlen-1] == '\n') {
                            buffer[readlen-1] = '\0';
                        }
                        puts(buffer);
                        dispatcher(i, buffer, readlen);
                    }
                }
            }
        }
    }
    return 0;
}
