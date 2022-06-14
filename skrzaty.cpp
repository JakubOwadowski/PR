#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <mpi.h>
#include <time.h>
#include <random>
#include <thread>
#include <vector>
#include <algorithm>
#include <unistd.h>

//using namespace std;
using std::cout;
using std::vector;
using std::endl;
using std::remove_if;

const int W = 10;
const int K = 5;
const int S = 5;
const int STATUS_KONIE = 5000;
const int STATUS_WSTAZKI = 6000;
const int STATUS_RELEASE = 7000;
const int STATUS_ACK = 1000;

int ACK = 0;
int lamport_clock;
int size, rank, len; 

struct KonieData {
    int rank;
    int lamport_clock;
};

struct WstazkiData {
    int rank;
    int lamport_clock;
    int wstazki;
};

bool operator==(KonieData const& first, KonieData const& second){
	return first.rank == second.rank;
}

bool operator==(WstazkiData const& first, WstazkiData const& second){
	return first.rank == second.rank;
}

vector<KonieData> konieQueue;
vector<WstazkiData> wstazkiQueue;
pthread_mutex_t mutexLamport = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSend = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexQueueWstazki = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexACK = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexKonie = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexWstazki = PTHREAD_MUTEX_INITIALIZER;


int getWating(vector<KonieData>data, KonieData konieData) {
    auto index = find(data.begin(), data.end(), konieData);

    if (index != data.end())
        return index - data.begin() + 1;
    return -1;
}


int getWstazki(vector<WstazkiData> wstazki, WstazkiData order) {

    int suma = order.wstazki;

    for(vector<WstazkiData>::const_iterator it = wstazki.begin(); it != find(wstazki.begin(), wstazki.end(), order); it++)
    {
        WstazkiData element = *it;
        suma += element.wstazki;
    }

    return suma;
}

void increaseLamport(int lamport){
    pthread_mutex_lock(&mutexLamport);
    lamport_clock = (lamport > lamport_clock ? lamport : lamport_clock)+1;
    pthread_mutex_unlock(&mutexLamport);
}

bool compare_KonieData(KonieData first, KonieData second){
    if (first.lamport_clock < second.lamport_clock){
			return true;
    }
    if (first.lamport_clock < second.lamport_clock){
        return false;
    }
    else {
        return first.rank < second.rank;
    }
}

bool compare_WstazkiData(WstazkiData first, WstazkiData second){
    if (first.lamport_clock < second.lamport_clock){
			return true;
    }
    if (first.lamport_clock < second.lamport_clock){
        return false;
    }
    else {
        return first.rank < second.rank;
    }
}

bool compare_removeKon(KonieData first, KonieData second){
    return first.rank == second.rank;
}

bool compare_removeWstazka(WstazkiData first, WstazkiData second){
    return first.rank == second.rank;
}


void removeFromKonieQueue(std::vector<KonieData> & konieDataQueue, int rank){
		remove_if(konieDataQueue.begin(), konieDataQueue.end(), [&](KonieData const & place){
				return place.rank== rank;
				});
        return;
}


void removeFromWstazkiQueue(std::vector<WstazkiData> & wstazkiDataQueue, int rank){
	remove_if(wstazkiDataQueue.begin(), wstazkiDataQueue.end(), [&](WstazkiData const & place){
				return place.rank == rank;
				});
    return;
}


void *message_processor_skrzat (void * arg) {
    int reciverMessage[3];
    int message[3];
    while (true) {
        MPI_Status status;
        MPI_Recv(reciverMessage, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == STATUS_KONIE){
            increaseLamport(reciverMessage[1]);
            message[0] = rank;
            message[1] = lamport_clock;
            message[2] = 2137;
            cout << "Timestamp: " << message[1] << "Cout process rank: " << rank << " Msg: Got STATUS_KONIE from " << status.MPI_SOURCE << endl;
            KonieData tempData;
            tempData.rank = status.MPI_SOURCE;
            tempData.lamport_clock = message[1];

            pthread_mutex_lock(&mutexQueue);
                konieQueue.push_back(tempData);
                sort(konieQueue.begin(), konieQueue.end(), compare_KonieData);
            pthread_mutex_unlock(&mutexQueue);
	
            pthread_mutex_lock(&mutexSend);
                MPI_Send(&message, 3, MPI_INT, status.MPI_SOURCE, STATUS_ACK, MPI_COMM_WORLD);
		    pthread_mutex_unlock(&mutexSend);
        }
        else if(status.MPI_TAG == STATUS_WSTAZKI){
            increaseLamport(reciverMessage[1]);
            message[0] = rank;
            message[1] = lamport_clock;
            message[2] = 2137;
            cout << "Timestamp: " << message[1] << "Cout process rank: " << rank << " Msg: Got STATUS_WSTAZKI from " << status.MPI_SOURCE << endl;
            WstazkiData tempData;
            tempData.lamport_clock = message[1];
            tempData.rank = status.MPI_SOURCE;
            tempData.wstazki = message[2];
            pthread_mutex_lock(&mutexQueueWstazki);
                wstazkiQueue.push_back(tempData);
                sort(wstazkiQueue.begin(), wstazkiQueue.end(), compare_WstazkiData);
            pthread_mutex_unlock(&mutexQueueWstazki);

            pthread_mutex_lock(&mutexSend);
                MPI_Send(&message, 3, MPI_INT, status.MPI_SOURCE, STATUS_ACK, MPI_COMM_WORLD);
            pthread_mutex_unlock(&mutexSend);
        }
        else if(status.MPI_TAG == STATUS_ACK){
            pthread_mutex_lock(&mutexACK);
                ACK++;
            pthread_mutex_unlock(&mutexACK);
            cout << "Timestamp: " << message[1] << "Cout process rank: " << rank << " Msg: Got STATUS_ACK from " << status.MPI_SOURCE << endl;
        }
        else if(status.MPI_TAG == STATUS_RELEASE){
            pthread_mutex_lock(&mutexKonie);
                removeFromKonieQueue(konieQueue, status.MPI_SOURCE);
            pthread_mutex_unlock(&mutexKonie);

            pthread_mutex_lock(&mutexWstazki);
                removeFromWstazkiQueue(wstazkiQueue, status.MPI_SOURCE);
            pthread_mutex_unlock(&mutexWstazki);

        }


    }

    return 0;
}


int main (int argc, char** argv) {
	MPI_Init(&argc,&argv);

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    srand(time(NULL));

    //#shared mem -- po co to było? 
    int message[3]; //komunikat
    lamport_clock = rank;
    KonieData konieData;
    WstazkiData wstazkiData;

    pthread_t message_processor;
    pthread_create(&message_processor, NULL, message_processor_skrzat, 0);

    while (true) {
        //if skarzty
        //WSTAZKI START
        int wstazki = rand() % W;

        pthread_mutex_lock(&mutexLamport);
            message[0] = rank;
            message[1] = lamport_clock;
            message[2] = wstazki;
            wstazkiData.lamport_clock = lamport_clock;
            wstazkiData.rank = rank;
            wstazkiData.wstazki = wstazki;
        pthread_mutex_unlock(&mutexLamport);

        pthread_mutex_lock(&mutexSend);
            for (int i = 0; i < size; i++) {
                MPI_Send(&message, 3, MPI_INT, i, STATUS_WSTAZKI, MPI_COMM_WORLD); //GET WSĄŻKA
                cout << "Sent \"wstazki\" request from " << rank << " to " << i 
                << " with lamport clock " << lamport_clock << endl;
            }
        pthread_mutex_unlock(&mutexSend);
        ACK = 0;
        while(ACK < size);
        cout << rank << ": Waiting for \"wstazki\"" << endl;
        while (getWstazki(wstazkiQueue, wstazkiData) > W);
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
                MPI_Send(&message, 3, MPI_INT, i, STATUS_KONIE, MPI_COMM_WORLD); //GET KOŃ
                cout << "Sent \"konie\" request from " << rank << " to " << i 
                << " with lamport clock " << lamport_clock << endl;
            }
        pthread_mutex_unlock(&mutexSend);
        ACK = 0;
        while(ACK < size);
        cout << rank << ": Waiting for \"koń\"" << endl;
        while (getWating(konieQueue, konieData) > K);
        cout << rank << ": Got  \"koń\"" << endl;
        //KONIE END

        sleep(rand() % 1000); //DO STUFF

        pthread_mutex_lock(&mutexSend);
            for(int i = 0; i < size; i++) {
                MPI_Send(&message, 3, MPI_INT, i, STATUS_RELEASE, MPI_COMM_WORLD); //RELEASE
                cout << "Release from " << rank << endl;
            }
        pthread_mutex_unlock(&mutexSend);
    }
    //if psycholożki
    //do psycholożki
	
	MPI_Finalize();
    return 0;
}
