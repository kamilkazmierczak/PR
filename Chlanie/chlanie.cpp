#include <mpi.h>
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <vector>

#define STAN_ZDROWIA 100
#define GLOSY 101
 
using namespace std;

int tid, size;
int zdrowy;

int zegar = -1;

MPI_Status status;
struct jednostka {
  int id;
  int votes;
};
struct votes {
  int staruszek;
  int lokalizacja;
  int zegar;
};

struct wysylka {
    int value;
    int zegar;
};

vector <jednostka> zdrowi;
vector <jednostka> lokalizacje;
vector <jednostka> zarzad;
int wybrana_lokalizacja = -1;

int moj_glos_staruszek;
int moj_glos_lokalizacja;

void generate_stan_zdrowia() {
  if((rand() % 10) <2) {
    zdrowy = 0;
  } else {
    zdrowy = 1;
  }
}

void send_stan_zdrowia() {
  for (int i=0; i<size; i++) {
    if(i != tid) {
        wysylka wysylkaobiekt;
        wysylkaobiekt.value = zdrowy;
        zegar++;
        wysylkaobiekt.zegar = zegar;
      printf( "%d %d: Wysyłam mój stan zdrowia %d do: %d \n", zegar, tid, zdrowy, i);
      MPI_Send(&wysylkaobiekt, sizeof(wysylka), MPI_BYTE, i, STAN_ZDROWIA, MPI_COMM_WORLD);
    }
  }
}

void receive_stan_zdrowia() {
  wysylka buf;
  if(zdrowy == 1) { 
    jednostka staruszek;
    staruszek.id = tid;
    staruszek.votes = 0;
    zdrowi.push_back(staruszek);
  }
  for (int i=0; i<size-1; i++) {
    MPI_Recv(&buf, sizeof(wysylka), MPI_BYTE, MPI_ANY_SOURCE, STAN_ZDROWIA, MPI_COMM_WORLD, &status);
    zegar++;
    zegar = max(zegar, buf.zegar);
    printf("%d %d: Odebrałem status zdrowia %d od: %d\n", zegar, tid, buf.value, status.MPI_SOURCE);
    if(buf.value == 1) {   
      jednostka staruszek;
      staruszek.id = status.MPI_SOURCE;
      staruszek.votes = 0; 
      zdrowi.push_back(staruszek);
    }
  }
}

void reset_lokalizacje() {
  lokalizacje.clear();
  for(int i=0; i<4; i++) {
    jednostka lokalizacja;
    lokalizacja.id = i;
    lokalizacja.votes = 0;
    lokalizacje.push_back(lokalizacja);
  }
}

int get_position(int id, vector<jednostka> collection) {

  for(int i = 0; i < collection.size(); i++) {
    if(collection.at(i).id == id) {
      return i;
    }
  }
  return -1;
}

int glos_na_staruszka() {
  int staruszek = tid;

  while(staruszek == tid && get_position(staruszek, zdrowi) > -1) {
    staruszek = (rand() % size);
  }
 
  return staruszek;
}

void send_votes() {
    moj_glos_staruszek = glos_na_staruszka();
    moj_glos_lokalizacja = (rand() % 4);
  votes glos;
  glos.staruszek = moj_glos_staruszek;
  glos.lokalizacja = moj_glos_lokalizacja;
    
  for (int i=0; i<zdrowi.size(); i++) {
    if(zdrowi.at(i).id != tid) {
        zegar++;
        glos.zegar = zegar;
      printf( "%d %d: Wysyłam mój głos na staruszka %d oraz na lokalizację %d do: %d \n", zegar, tid, glos.staruszek, glos.lokalizacja, zdrowi.at(i).id);
      MPI_Send(&glos, sizeof(votes), MPI_BYTE, zdrowi.at(i).id, GLOSY, MPI_COMM_WORLD);
    }
  }
}

void dodaj_glosy(int staruszek, int lokalizacja) {
  int staruszek_position = get_position(staruszek, zdrowi);
  int lokalizacja_position = get_position(lokalizacja, lokalizacje);
  
  if(staruszek_position > -1) {
    zdrowi.at(staruszek_position).votes++;
  }
  if(lokalizacja_position > -1) {
    lokalizacje.at(lokalizacja_position).votes++;
  }
}

void receive_votes() {
  if(zdrowy == 1) { 
    int staruszek = moj_glos_staruszek;
    int lokalizacja = moj_glos_lokalizacja;
    dodaj_glosy(staruszek, lokalizacja);
  }

  votes buf;
  for (int i=0; i<zdrowi.size(); i++) {
    if(zdrowi.at(i).id != tid) {
        MPI_Recv(&buf, sizeof(votes), MPI_BYTE, MPI_ANY_SOURCE, GLOSY, MPI_COMM_WORLD, &status);
        zegar++;
        zegar = max(zegar, buf.zegar);
        printf("%d %d: Odebrałem głosy (staruszek: %d, lokalizacja: %d) od: %d\n", zegar, tid, buf.staruszek, buf.lokalizacja, status.MPI_SOURCE);
        dodaj_glosy(buf.staruszek, buf.lokalizacja);
    }
  }
}

void sortuj_staruszkow() { 
  int n = zdrowi.size();
  while(n>1) {
    for(int i=0; i < n-1; i++) {
      if((zdrowi.at(i+1).votes > zdrowi.at(i).votes) || (zdrowi.at(i+1).votes == zdrowi.at(i).votes && zdrowi.at(i+1).id > zdrowi.at(i).id)) {
        jednostka tmp = zdrowi.at(i);
        zdrowi.at(i) = zdrowi.at(i+1);
        zdrowi.at(i+1) = tmp;
      }
    }
    n--;
  }
}

void choose_best() {
  int max_lokalizacja = -1;
  for(int i=0; i < lokalizacje.size(); i++) {
    if(lokalizacje.at(i).votes > max_lokalizacja) {
      max_lokalizacja = lokalizacje.at(i).votes;
      wybrana_lokalizacja = lokalizacje.at(i).id;
    }
    if(lokalizacje.at(i).votes == max_lokalizacja) {

      if(wybrana_lokalizacja < lokalizacje.at(i).id) {
        wybrana_lokalizacja = lokalizacje.at(i).id;
      }
    }
  }

  sortuj_staruszkow();
  zarzad.push_back(zdrowi.at(0));
  zarzad.push_back(zdrowi.at(1));
  zarzad.push_back(zdrowi.at(2));

    printf("%d: Staruszkowie: %d %d %d, lokalizacja: %d\n", tid, zdrowi.at(0).id, zdrowi.at(1).id, zdrowi.at(2).id, wybrana_lokalizacja);
}

int reset_all() {
    zdrowy = -1;
    zarzad.clear();
    zdrowi.clear();
    lokalizacje.clear();
    wybrana_lokalizacja = -1;
    moj_glos_staruszek = -1;
    moj_glos_lokalizacja = -1;
    zegar = -1;
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
    
  MPI_Comm_size( MPI_COMM_WORLD, &size );
	MPI_Comm_rank( MPI_COMM_WORLD, &tid );
    
  srand((int)time(0)+tid);
int i=0;
  while(i < 1) {
    printf("%d: Rozpoczynam rundę nr %d\n", tid, i);
    generate_stan_zdrowia();
    send_stan_zdrowia();
    receive_stan_zdrowia();
    printf("%d: stan - %d\n", tid, zdrowy);
	if(zdrowy == 1) {
        reset_lokalizacje();
        send_votes();
        receive_votes();
        choose_best();
	}
    printf("%d: Rozpoczynam spotkanie nr %d\n", tid, i);
    usleep(rand()%1000000);
    printf("%d: Kończę spotkanie nr %d\n", tid, i);
    reset_all();
    printf("%d: Kończę rundę nr %d\n", tid, i);
    i++;
  }

  MPI_Finalize();
  return 0;
}
