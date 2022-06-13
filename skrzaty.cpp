#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <mpi.h>
#include <time.h>
#include <random>
#include <thread>
#include <vector>

using namespace std;

const int W = 10;
const int K = 5;
const int S = 5;

pthread_mutex_t mutexLamport = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSend = PTHREAD_MUTEX_INITIALIZER;

struct KonieData {
    int rank;
    int lamport_clock;
};

struct WstazkiData {
    int rank;
    int lamport_clock;
    int wstazki;
};


int getWating(vector<KonieData> data KonieData konieData) {
    auto index = find(data.begin(), data.end(), konieData);

    if (index != data.end())
        return index - kolejka_z_info.begin() + 1;
    return -1;
}

void *message_processor_skrzat (void * arg) {
    int reciverMessage[3];
    int message[3];
    while (true) {
        MPI_Status status;
        MPI_Recv(reciverMessage, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    }

    return 0;
}

int main (int argc, char** argv) {
    int size, rank, len;
	MPI_Init(&argc,&argv);

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    srand(time(NULL));

    #shared mem
    int message[3]; //komunikat
    int lamport_clock = rank;
    int ack;
    KonieData konieData;
    WstazkiData wstazkiData;
    vector<KonieData> konieQueue;
    vector<WstazkiData> wstazkiQueue;

    pthread_t message_processor;
    pthread_create(&message_processor, NULL, message_processor_skrzat, 0);

    while (true) {
        //if skarzty
        //WSTAZKI START
        int wstazki = rand() % W;

        pthread_mutex_lock(&mutexLamport);
            message[0] = rank;
            message[1] = lamport_clock;
            message[3] = wstazki;
            wstazkiData.lamport_clock = lamport_clock;
            wstazkiData.rank = rank;
            wstazkiData.wstazki = wstazki;
        pthread_mutex_unlock(&mutexLamport);

        pthread_mutex_lock(&mutexSend);
            for (int i = 0; i < size; i++) {
                MPI_Send(&message, 3, MPI_INT, i, 6000, MPI_COMM_WORLD); //GET WSĄŻKA
                cout << "Sent \"wstazki\" request from " << rank << " to " << i 
                << " with lamport clock " << lamport_clock << endl;
            }
        pthread_mutex_unlock(&mutexSend);
        ack = 0;
        while(ack < size);
        cout << rank << ": Waiting for \"wstazki\"" << endl;
        while (getWating(wstazkiQueue, wstazkiData) > W);
        cout << rank << ": Got  \"wstazki\"" << endl;
        //WSTAZKI END

        //KONIE START
        pthread_mutex_lock(&mutexLamport);
            message[0] = rank;
            message[1] = lamport_clock;
            konieData.lamport_clock = lamport_clock;
            konieData.rank = rank;
        pthread_mutex_unlock(&mutexLamport);

        pthread_mutex_lock(&mutexSend);
            for (int i = 0; i < size; i++) {
                MPI_Send(&message, 3, MPI_INT, i, 5000, MPI_COMM_WORLD); //GET KOŃ
                cout << "Sent \"konie\" request from " << rank << " to " << i 
                << " with lamport clock " << lamport_clock << endl;
            }
        pthread_mutex_unlock(&mutexSend);
        ack = 0;
        while(ack < size);
        cout << rank << ": Waiting for \"koń\"" << endl;
        while (getWating(konieQueue, konieData) > K);
        cout << rank << ": Got  \"koń\"" << endl;
        //KONIE END

        sleep(rand() % 1000); //DO STUFF

        pthread_mutex_lock(&mutexSend);
            for(int i = 0; i < size; i++) {
                MPI_Send(&message, 3, MPI_INT, i, 7000, MPI_COMM_WORLD); //RELEASE
                cout << "Release from " << rank << endl;
            }
        pthread_mutex_unlock(&mutexSend);
    }
    //if psycholożki
    //do psycholożki
	
	MPI_Finalize();
    return 0;
}
