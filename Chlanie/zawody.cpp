#include <mpi.h>
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <vector>
#include <stdio.h>
#include <string>

#define INVITATION 100
#define CONFIRMATION 110
using namespace std;
const int probability = 30;

//globlas
int invitationCounter = 0;
int invitationSuccessCounter = 0;

int bestStudent = -1;
int bestClock = -1;
int studentClock = -1;
int sendClock = -1;
int tid, size;
int wantToDrink = 0;
int iteratorr = -1;
vector <bool> processedInvitations;
vector <int> team;
string state;


struct msg {
    int value;
    int studentClock;
};

void sendInvitation();
void receiveMsg();
void receive(msg buf);
void confirmInvitation();
void sendResponseToInvitation(int id);

MPI_Status status;



int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  state = "free";
  MPI_Comm_size( MPI_COMM_WORLD, &size );
	MPI_Comm_rank( MPI_COMM_WORLD, &tid );

  srand((int)time(0)+tid);
  msg buf;
  int a = -1;
  while(1) {
    processedInvitations.resize(size,false);
    usleep( rand()%50000 );
    a++;

    if(state == "free"/*wantToDrink==0*/ && (rand() % 100) <probability){
      wantToDrink = 1;
      sendInvitation();
      wantToDrink = 0;
      state = "wait for response";
    }else if(state=="free"){
      studentClock++;
      continue;
      //sendInvitation();
    }



    //if (wantToDrink == 1) {



    //}

    MPI_Recv(&buf, sizeof(msg), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    receive(buf);
    //printf("sadsan\n");


    printf("%d: beststudent = %d\n",tid,bestStudent);

    if (invitationCounter >= size-1 && invitationSuccessCounter > 0) {

  //  printf("%d: Hurraaa :) succCounter = %d beststudent=%d\t",tid,invitationSuccessCounter,bestStudent);
    string concat = "";
    for( int i = 0; i < team.size(); i++ ){
      concat+=to_string(team.at(i));
      concat+=";";
      //printf("%d;",team.at(i));
    }
    //concat+="\n";
    printf("%d: Hurraaa :) succCounter = %d beststudent=%d\t %s\n",tid,invitationSuccessCounter,bestStudent,concat.c_str());
    //printf("%s\n",concat.c_str());

    state = "send confirmation";
    confirmInvitation();
    }


  }

  MPI_Finalize();
  return 0;
}


void sendInvitation(){
  msg myMsg;
  myMsg.studentClock = ++studentClock;
  myMsg.value = wantToDrink;
  sendClock = myMsg.studentClock;

  if (wantToDrink == 1) {
    for( int i = 0; i < processedInvitations.size(); i++ ){
      processedInvitations.at(i) = true;
    }
  }


  for (int i=0; i<size; i++) {
    if(i != tid) {
        printf("%d: Wysylam do %d zaproszenie o wartosci %d, clock=%d\n",tid,i,wantToDrink,sendClock);
        MPI_Send(&myMsg, sizeof(msg), MPI_BYTE, i, INVITATION, MPI_COMM_WORLD);
    }
  }
  if (wantToDrink == 1) {
    bestStudent = tid;
    bestClock = myMsg.studentClock;
  }
  invitationCounter = 0;
  invitationSuccessCounter = 0;
  team.clear();
  team.push_back(tid);
}

void sendResponseToInvitation(int id){
  msg myMsg;
  myMsg.studentClock = ++studentClock;
  myMsg.value = 0;//wantToDrink;
  //sendClock = myMsg.studentClock;
  printf("%d: Wysylam odpowiedz na zaproszenie do %d, clock=%d\n",tid,id,sendClock);
  MPI_Send(&myMsg, sizeof(msg), MPI_BYTE, id, INVITATION, MPI_COMM_WORLD);

}

void confirmInvitation(){
  //wysylamy bestClocka i bestStudenta porownujemy do naszych aby dowiedziec sie czy mamy aktualne dane
  for (int i=0; i<team.size() ;i++) {

  }
}

void receive(msg buf){
  printf("%d: Dostalem odpowiedz od %d, o tagu %d\n",tid,status.MPI_SOURCE,status.MPI_TAG);
  //msg buf;
  // Synchronizacja zegara
  studentClock = max(buf.studentClock, studentClock) + 1;
  int id = status.MPI_SOURCE;

  switch (status.MPI_TAG) {
    case INVITATION:
      if (processedInvitations.at(id) == true) {
        if (buf.value == 0 && buf.studentClock < sendClock) {
          return;
        }
        processedInvitations.at(id) = false;
        if (state == "wait for response") {
          invitationCounter++;
          if (buf.value == 1) {
            invitationSuccessCounter++;
            team.push_back(id);
            if (buf.studentClock < bestClock || (buf.studentClock == bestClock && bestStudent > id )) {
              bestStudent = id;
              bestClock = buf.studentClock;
            }
          }
            printf("%d: buf value = %d od %d mojClock:%d, jego Clock:%d\n",tid,buf.value,id,sendClock,buf.studentClock);
        }

      }else if(state != "free"){
        sendResponseToInvitation(id);
      }
      break;

      case CONFIRMATION:


      break;
    default:
      break;
  }


}

// void receiveMsg(){
//   msg buf;
//   int suma = wantToDrink;
//   //int bestStudent = -1;
//
//   if(wantToDrink == 1){
//     bestStudent = tid;
//   }
//
//   for (int i=0; i<size-1; i++) {
//     MPI_Recv(&buf, sizeof(msg), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
//
//     //printf("%d %d: Odebrałem status zdrowia %d od: %d\n", zegar, tid, buf.value, status.MPI_SOURCE);
//     if(buf.value == 1 && wantToDrink ==1) {
//       if(buf.studentClock < sendClock){
//         //printf("buf stuent clock %d\n", );
//         bestStudent = status.MPI_SOURCE;
//       }else if(buf.studentClock == sendClock && status.MPI_SOURCE<tid){
//         bestStudent = status.MPI_SOURCE;
//       }
//     }
//
//     studentClock++;
//     studentClock = max(studentClock, buf.studentClock);
//
//   }
//   printf("best = %d tid= %d sendClock=%d\n",bestStudent,tid,sendClock);
//
//
// }
