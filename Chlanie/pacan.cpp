#include <mpi.h>
#include <stdio.h>
#include <cstdlib>
#include <signal.h>
#include <unistd.h>
#include <vector>
#include <algorithm>

//<----PARAMETRY
#define GUIDES 2
#define GROUPSIZE 2
//----->

#define BEATING_CHANCE 30

//<----TYPY WIADOMOSCI
#define REQUEST 100
#define CONFIRM 101
#define GUIDEINFO 102
#define TRIPSTART 103
#define TRIPEND 104
//----->

//<-----LOG SETUP
//#define SEND_LOG
#define RECEIVE_LOG
#define MAIN
//#define DELETE
//----->

using namespace std;

struct tripData {
    int time;
    int owner;
    int guideId;
    int amount;
    int type;

    bool operator<(const tripData& a) const
    {
        if (time == a.time){
            return owner < a.owner;
        }
        else{
            return time < a.time;
        }
    }
};

//<-----ZMIENNE GLOBALNE
int myguide = -1;
int ourTime = 0;
int tid;
int size;
vector <tripData> queue;
MPI_Status status;
int noTourists[GUIDES] = {0};
bool isWaiting = false;
bool firstTourist = false;

vector <int> tickets[GUIDES];
bool started = false;
int noTrip = 0;
//----->

void fillTickets(){
    for(int i=0; i<GUIDES; i++){
        for(int j=0; j<GROUPSIZE; j++){
            tickets[i].push_back(1);
        }
    }
}

int checkAmountOfGuides(){
    return tickets[myguide].size();
}

void deleteMyConfirm(){

    for (int i=0; i<queue.size(); i++){
        if (queue[i].type == CONFIRM){
            queue.erase(queue.begin()+i);
            i--;
        }
    }
}

void printQueue(){
    printf("%d: -----Queue-----\n", tid);
    for( int i = 0; i < queue.size(); i++ ){
        printf("%d: Time: %d Owner: %d Guide: %d Type:%d\n", tid, queue[i].time, queue[i].owner, queue[i].guideId, queue[i].type);
    }
    printf("%d: ---------------\n", tid);
}

void printNoTourists(){
    printf("%d: -----noTourists-----\n", tid);
    for(int i=0; i < GUIDES; i++){
        printf("%d: noTourists[%d] = %d\n", tid, i, noTourists[i]);
    }
    printf("%d: --------------------\n", tid);
}

void sendRequest(){
    tripData data;
    data.guideId = myguide;
    data.owner = tid;
    data.type = REQUEST;
    data.time = ourTime;

    #ifdef SEND_LOG
    printf("%d: Wysylamy REQUEST\n", tid);
    #endif

    for (int i=0; i<size; i++){
        MPI_Send(&data, sizeof( struct tripData ), MPI_BYTE, i, REQUEST, MPI_COMM_WORLD);
    }
    ourTime++;
}

void sendGuideInfo(){
    tripData data;
    data.guideId = myguide;
    data.owner = tid;
    data.type = GUIDEINFO;
    data.time = -1; //Niepotrzebny poniewaz nie wpisujemy tej informacji do zadnej z kolejek

    #ifdef SEND_LOG
    printf("%d: Wysylamy GUIDEINFO dla %d\n", tid,myguide);
    #endif

    for (int i=0; i<size; i++){
         MPI_Send(&data, sizeof( struct tripData ), MPI_BYTE, i, GUIDEINFO, MPI_COMM_WORLD);
    }
}

void sendTripStart(){
    tripData data;
    data.guideId = myguide;
    data.owner = tid;
    data.type = TRIPSTART;
    data.time = -1; //

    #ifdef SEND_LOG
    printf("%d: Wysylamy TRIPSTART\n", tid);
    #endif
    for (int i=0; i<size; i++){
        MPI_Send(&data, sizeof( struct tripData ), MPI_BYTE, i, TRIPSTART, MPI_COMM_WORLD);
    }
}

void sendTripEnd(){
    tripData data;
    data.guideId = myguide;
    data.owner = tid;
    data.type = TRIPEND;
    data.time = -1; //

    #ifdef SEND_LOG
    printf("%d: Wysylamy TRIPEND\n", tid);
    #endif
    for (int i=0; i<size; i++){
        MPI_Send(&data, sizeof( struct tripData ), MPI_BYTE, i, TRIPEND, MPI_COMM_WORLD);
    }
}

void handle(tripData data){
    // Synchronizacja zegara
    ourTime = max(data.time, ourTime) + 1;

    if (status.MPI_TAG == REQUEST){
        #ifdef RECEIVE_LOG
        printf("%d: Otrzymano wiadomosc typu REQUEST od %d\n", tid, data.owner);
        #endif
            tripData dataToInsert;
            dataToInsert.guideId = data.guideId;
            dataToInsert.owner = data.owner;
            dataToInsert.type = REQUEST;
            dataToInsert.time = data.time;

            //Umieszczenie w kolejce i posortowanie
            //Jesli REQUEST nie dotyczy naszego przewodnika to nie zapisujemy tego w kolejce
            //if(data.guideId == myguide){
                queue.push_back(dataToInsert);
                std::sort(queue.begin(), queue.end());
            //}

            //Nie wysylamy potwierdzenia do siebie
            if(data.owner != tid){
            //Przygotowanie do wyslania CONFIRM
                dataToInsert.time = ourTime;
                dataToInsert.type = CONFIRM;
                dataToInsert.owner = tid;
                MPI_Send(&dataToInsert, sizeof( struct tripData ), MPI_BYTE, status.MPI_SOURCE, CONFIRM, MPI_COMM_WORLD);
            }
    }
    else if (status.MPI_TAG == CONFIRM){
        queue.push_back(data);
        std::sort(queue.begin(), queue.end());
    }
    else if (status.MPI_TAG == GUIDEINFO){
        #ifdef RECEIVE_LOG
        printf("%d: Otrzymano wiadomosc typu GUIDEINFO od %d (guide = %d)\n", tid, data.owner, data.guideId);
        #endif

        tickets[data.guideId].pop_back();

        //if(data.guideId == myguide){
            //Usuniecie zadania ownera z kolejki
            for (int i=0; i<queue.size(); i++){
                if((queue[i].type == REQUEST) && (queue[i].owner == data.owner)){
                    #ifdef DELETE
                    printf("%d: Usuwam wiadomosc: Time: %d Owner: %d Guide: %d Type: %d\n", tid, queue[i].time, queue[i].owner, queue[i].guideId, queue[i].type);
                    #endif
                    queue.erase(queue.begin()+i);
                    i--;
                }
            }
        //}
    }
    else if (status.MPI_TAG == TRIPSTART){
        if ((data.guideId == myguide) && (isWaiting)){
            started = true;
            printf("%d: (%d) Rozpoczynam wycieczke z przewodnikiem %d\n", tid, firstTourist, myguide);
            noTrip++;
            if(firstTourist){
                usleep((rand()%500000) + 50000);
                sendTripEnd();
            }
        }
    }
    else if (status.MPI_TAG == TRIPEND){

        for(int i=0; i<GROUPSIZE; i++){
            tickets[data.guideId].push_back(1);
        }
            printf("%d: Otrzymano wiadomosc typu TRIPEND od %d (guide = %d, %d)\n", tid, data.owner, data.guideId,GROUPSIZE);
        if ((data.guideId == myguide) && started){
            printf("%d: Koncze wycieczke z przewodnikiem %d\n", tid, myguide);
            //printf("%d: tickets[myguide].size() = %d\n", tid, tickets[myguide].size());

            myguide = -1;
            if (rand()%100 < BEATING_CHANCE){
               printf("%d: Pobito mnie\n", tid);
               usleep(rand()%50000);
            }
        }
    }
}

//Funkcja sprawdzajaca warunek czy nasze zadanie jest na szczycie kolejki
/*bool isFirst(){
    if ((queue[0].owner == tid) && (queue[0].type == REQUEST)){
        return true;
    }
    return false;
}*/

bool isFirst(){
    for (int i=0; i<queue.size(); i++){
        if((queue[i].type == REQUEST) && (queue[i].guideId == myguide)){
            if (queue[i].owner == tid){
                return true;
            }
            else {
                return false;
            }
        }
    }
}

//Funkcja sprawdzajaca czy otrzymalismy wiadomosci od pozostalych procesow
//ze znacznikiem czasowym wiekszy niz nasz.
bool isConfirmed(){
    //Poszukiwanie naszego znacznika czasowego
    int timestamp = -1;
    for(int i=0; i < queue.size(); i++){
        if((queue[i].owner == tid) && (queue[i].type == REQUEST)){
            timestamp = queue[i].time;
            break;
        }
    }

    int confirmCounter = 0;
    bool result = true;
    for(int i=0; i < queue.size(); i++){
        if ((queue[i].type == CONFIRM) && (queue[i].guideId == myguide)){
            confirmCounter++;
            if(queue[i].time > timestamp){
                //OK
            }
            else {
                result = false;
            }
        }
    }

    if ((confirmCounter == (size - 1)) && result){
        return true;
    }
    else {
        return false;
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    MPI_Comm_size( MPI_COMM_WORLD, &size );
	MPI_Comm_rank( MPI_COMM_WORLD, &tid );

    tripData data;

    srand(tid);

    fillTickets();

    while(1){
        usleep( rand()%50000 );

        //Wybieranie przewodnika
        if (myguide == -1){
            //<---
            //queue.clear();
            isWaiting = false;
            firstTourist = false;
            started = false;
            //--->
            int randomNo = rand();
            myguide = randomNo%GUIDES; 
            #ifdef MAIN
            printf("%d: randomNo = %d, GUIDES = %d\n", tid, randomNo, GUIDES);
            printf("%d: Moj przewodnik ma numer %d\n", tid, myguide);
            #endif
            sendRequest();
        }

        MPI_IRecv( &data, sizeof( struct tripData), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        handle(data);

        //Wchodzenie do sekcji krytycznej
        #ifdef MAIN
        //printf("%d: isFirst() = %d, isConfirmed() = %d\n", tid, isFirst(), isConfirmed());
        #endif
        if((isFirst() && isConfirmed()) && !isWaiting && (checkAmountOfGuides() > 0)){
            #ifdef MAIN
            printf("%d: Wejscie do sekcji krytycznej\n",tid);
            #endif
        printf("%d: checkAmountOfGuides() dla %d = %d\n", tid, myguide, checkAmountOfGuides());
            deleteMyConfirm();
            isWaiting = true;
            if (checkAmountOfGuides() == GROUPSIZE){
                firstTourist = true;
            }
            sendGuideInfo();
            #ifdef MAIN
            printf("%d: Wyjscie z sekcji krytycznej\n",tid);
            #endif
        }


        //Pierwszy turysta rozpoczyna wycieczke gdy jest odpowiednia liczba w grupie
        if(firstTourist && (checkAmountOfGuides() == 0) && !started){
            sendTripStart();
            started = true;
        }
    }

    MPI_Finalize();
}
