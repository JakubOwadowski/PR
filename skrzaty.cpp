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

// using namespace std;
using std::cout;
using std::endl;
using std::max;
using std::remove_if;
using std::vector;

const int W = 20;
const int K = 5;
const int S = 4;
const float SPREC = 0.8;
const int STATUS_KONIE = 5000;
const int STATUS_WSTAZKI = 6000;
const int STATUS_SALKI = 7000;
const int STATUS_RELEASE_KONIE = 5001;
const int STATUS_RELEASE_WSTAZKI = 6001;
const int STATUS_RELEASE_SALKI = 7001;
const int STATUS_SKRZAT_DONE = 8000;
const int STATUS_PSYCHO = 9000;
const int STATUS_TRAUMA = 10000;
const int STATUS_ACK = 1000;

int ACK, lamport_clock;
int size, rank, len;

struct KonieData
{
    int rank;
    int lamport_clock;
};

struct WstazkiData
{
    int rank;
    int lamport_clock;
    int wstazki;
};

struct SalkiData
{
    int rank;
    int lamport_clock;
};

bool operator==(KonieData const &first, KonieData const &second)
{
    return first.rank == second.rank;
}

bool operator==(WstazkiData const &first, WstazkiData const &second)
{
    return first.rank == second.rank;
}

bool operator==(SalkiData const &first, SalkiData const &second)
{
    return first.rank == second.rank;
}

vector<KonieData> konieQueue;
vector<WstazkiData> wstazkiQueue;
vector<SalkiData> salkiQueue;
vector<WstazkiData> traumaQueue;
vector<KonieData> psychoQueue;
pthread_mutex_t mutexLamport = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSend = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexKonieQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexWstazkiQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSalkiQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPsychoQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexTraumaQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexACK = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexKonie = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexWstazki = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSalki = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPsycho = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexTrauma = PTHREAD_MUTEX_INITIALIZER;

int getKonie(vector<KonieData> queue, KonieData konieData)
{
    auto index = find(queue.begin(), queue.end(), konieData);

    if (index != queue.end())
        return index - queue.begin() + 1;
    return -1;
}

int getWstazki(vector<WstazkiData> wstazki, WstazkiData wstazkiData)
{

    int result = wstazkiData.wstazki;

    for (vector<WstazkiData>::const_iterator index = wstazki.begin();
        index != find(wstazki.begin(), wstazki.end(), wstazkiData);
        index++)
    {
        WstazkiData element = *index;
        result += element.wstazki;
    }

    return result;
}

int getSalki(vector<SalkiData> queue, SalkiData salkiData)
{
    auto index = find(queue.begin(), queue.end(), salkiData);

    if (index != queue.end())
        return index - queue.begin() + 1;
    return -1;
}

int getPsycho(vector<KonieData> queue, KonieData psychoData)
{
    auto index = find(queue.begin(), queue.end(), psychoData);

    if (index != queue.end())
        return index - queue.begin() + 1;
    return -1;
}

void increaseLamport(int lamport)
{
    pthread_mutex_lock(&mutexLamport);
    lamport_clock = max(lamport_clock, lamport) + 1;
    pthread_mutex_unlock(&mutexLamport);
}

bool compare_KonieData(KonieData first, KonieData second)
{
    if (first.lamport_clock < second.lamport_clock)
    {
        return true;
    }
    if (first.lamport_clock > second.lamport_clock)
    {
        return false;
    }
    else
    {
        return first.rank < second.rank;
    }
}

bool compare_WstazkiData(WstazkiData first, WstazkiData second)
{
    if (first.lamport_clock < second.lamport_clock)
    {
        return true;
    }
    if (first.lamport_clock > second.lamport_clock)
    {
        return false;
    }
    else
    {
        return first.rank < second.rank;
    }
}

bool compare_SalkiData(SalkiData first, SalkiData second)
{
    if (first.lamport_clock < second.lamport_clock)
    {
        return true;
    }
    if (first.lamport_clock > second.lamport_clock)
    {
        return false;
    }
    else
    {
        return first.rank < second.rank;
    }
}

void removeFromKonieQueue(std::vector<KonieData> &konieDataQueue, int message[3])
{

    konieDataQueue.erase(remove_if(konieDataQueue.begin(), konieDataQueue.end(), [&](KonieData const &place)
        { return place.rank == message[0]; }),
        konieDataQueue.end());
    return;
}

void removeFromWstazkiQueue(std::vector<WstazkiData> &wstazkiDataQueue, int message[3])
{
    wstazkiDataQueue.erase(remove_if(wstazkiDataQueue.begin(), wstazkiDataQueue.end(), [&](WstazkiData const &place)
        { return place.rank == message[0]; }),
        wstazkiDataQueue.end());
    return;
}

void removeFromSalkiQueue(std::vector<SalkiData> &salkiDataQueue, int message[3])
{

    salkiDataQueue.erase(remove_if(salkiDataQueue.begin(), salkiDataQueue.end(), [&](SalkiData const &place)
        { return place.rank == message[0]; }),
        salkiDataQueue.end());
    return;
}

void removeFromTraumaQueue(std::vector<WstazkiData> &traumaDataQueue, int message[3])
{

    traumaDataQueue.erase(remove_if(traumaDataQueue.begin(), traumaDataQueue.end(), [&](WstazkiData const &place)
        { return place.rank == message[0]; }),
        traumaDataQueue.end());
    return;
}

void *monitor(void *arg)
{
    int recivedMessage[3];
    int message[3];
    while (true)
    {
        MPI_Status status;
        MPI_Recv(recivedMessage, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == STATUS_KONIE)
        {
            increaseLamport(recivedMessage[1]);
            message[0] = rank;
            message[1] = lamport_clock;
            message[2] = 2137;
            // cout << "Timestamp: " << message[1] << " process rank: " << rank << " Msg: Got STATUS_KONIE from " << status.MPI_SOURCE << endl;
            KonieData tempData;
            tempData.rank = status.MPI_SOURCE;
            tempData.lamport_clock = message[1];

            pthread_mutex_lock(&mutexKonieQueue);
            konieQueue.push_back(tempData);
            sort(konieQueue.begin(), konieQueue.end(), compare_KonieData);
            pthread_mutex_unlock(&mutexKonieQueue);

            pthread_mutex_lock(&mutexSend);
            MPI_Send(&message, 3, MPI_INT, status.MPI_SOURCE, STATUS_ACK, MPI_COMM_WORLD);
            pthread_mutex_unlock(&mutexSend);
        }
        else if (status.MPI_TAG == STATUS_WSTAZKI)
        {
            message[0] = rank;
            message[1] = lamport_clock;
            message[2] = 2137;
            increaseLamport(recivedMessage[1]);
            // cout << "Timestamp: " << message[1] << "  process rank: " << rank << " Msg: Got STATUS_WSTAZKI from " << status.MPI_SOURCE << endl;
            WstazkiData tempData;
            tempData.lamport_clock = message[1];
            tempData.rank = status.MPI_SOURCE;
            tempData.wstazki = message[2];
            pthread_mutex_lock(&mutexWstazkiQueue);
            wstazkiQueue.push_back(tempData);
            sort(wstazkiQueue.begin(), wstazkiQueue.end(), compare_WstazkiData);
            pthread_mutex_unlock(&mutexWstazkiQueue);

            pthread_mutex_lock(&mutexSend);
            MPI_Send(&message, 3, MPI_INT, status.MPI_SOURCE, STATUS_ACK, MPI_COMM_WORLD);
            pthread_mutex_unlock(&mutexSend);
        }
        else if (status.MPI_TAG == STATUS_SALKI)
        {
            increaseLamport(recivedMessage[1]);
            message[0] = rank;
            message[1] = lamport_clock;
            message[2] = 2137;
            // cout << "Timestamp: " << message[1] << " process rank: " << rank << " Msg: Got STATUS_KONIE from " << status.MPI_SOURCE << endl;
            SalkiData tempData;
            tempData.rank = status.MPI_SOURCE;
            tempData.lamport_clock = message[1];

            pthread_mutex_lock(&mutexSalkiQueue);
            salkiQueue.push_back(tempData);
            sort(salkiQueue.begin(), salkiQueue.end(), compare_SalkiData);
            pthread_mutex_unlock(&mutexSalkiQueue);

            pthread_mutex_lock(&mutexSend);
            MPI_Send(&message, 3, MPI_INT, status.MPI_SOURCE, STATUS_ACK, MPI_COMM_WORLD);
            pthread_mutex_unlock(&mutexSend);
        }
        else if (status.MPI_TAG == STATUS_PSYCHO)
        {
            increaseLamport(recivedMessage[1]);
            message[0] = rank;
            message[1] = lamport_clock;
            message[2] = 2137;
            // cout << "Timestamp: " << message[1] << " process rank: " << rank << " Msg: Got STATUS_KONIE from " << status.MPI_SOURCE << endl;
            KonieData tempData;
            tempData.rank = status.MPI_SOURCE;
            tempData.lamport_clock = message[1];

            pthread_mutex_lock(&mutexPsychoQueue);
            psychoQueue.push_back(tempData);
            sort(psychoQueue.begin(), psychoQueue.end(), compare_KonieData);
            pthread_mutex_unlock(&mutexPsychoQueue);

            pthread_mutex_lock(&mutexSend);
            MPI_Send(&message, 3, MPI_INT, status.MPI_SOURCE, STATUS_ACK, MPI_COMM_WORLD);
            pthread_mutex_unlock(&mutexSend);
        }
        else if (status.MPI_TAG == STATUS_ACK)
        {
            pthread_mutex_lock(&mutexACK);
            ACK++;
            pthread_mutex_unlock(&mutexACK);
            // cout << "Timestamp: " << message[1] << "  process rank: " << rank << " Msg: Got STATUS_ACK from " << status.MPI_SOURCE << endl;
        }
        else if (status.MPI_TAG == STATUS_TRAUMA)
        {
            pthread_mutex_lock(&mutexTraumaQueue);
            removeFromTraumaQueue(traumaQueue, recivedMessage);
            pthread_mutex_unlock(&mutexTraumaQueue);
        }
        else if (status.MPI_TAG == STATUS_RELEASE_KONIE)
        {
            pthread_mutex_lock(&mutexKonie);
            removeFromKonieQueue(konieQueue, recivedMessage);
            pthread_mutex_unlock(&mutexKonie);
        }
        else if (status.MPI_TAG == STATUS_RELEASE_WSTAZKI)
        {
            pthread_mutex_lock(&mutexWstazki);
            removeFromWstazkiQueue(wstazkiQueue, recivedMessage);
            pthread_mutex_unlock(&mutexWstazki);
        }
        else if (status.MPI_TAG == STATUS_RELEASE_SALKI)
        {
            pthread_mutex_lock(&mutexSalki);
            removeFromSalkiQueue(salkiQueue, recivedMessage);
            pthread_mutex_unlock(&mutexSalki);
        }
        else if (status.MPI_TAG == STATUS_SKRZAT_DONE)
        {
            //cout << "SKRZAT " << recivedMessage[0] << " DONE, USED " << recivedMessage[2] << " WSTĄŻKI" << endl;
            pthread_mutex_lock(&mutexTraumaQueue);
            WstazkiData traumaData;
            traumaData.rank = recivedMessage[0];
            traumaData.lamport_clock = recivedMessage[1];
            traumaData.wstazki = recivedMessage[2];
            traumaQueue.push_back(traumaData);
            pthread_mutex_unlock(&mutexTraumaQueue);
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    // MPI_Init(&argc,&argv);
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int S = SPREC * size;
    int P = (1 - SPREC) * size; //nieistotne ale 

    srand(time(NULL));
    // make true rand
    for (int i = 0; i < 100 * rank; i++)
        rand();

    //#shared mem
    int message[3], time; // komunikat
    lamport_clock = rank;
    KonieData konieData;
    WstazkiData wstazkiData;
    SalkiData salkiData;
    KonieData psychoData;

    pthread_t monitor_t;
    pthread_create(&monitor_t, NULL, monitor, 0);

    while (true)
    {
        if (rank < S) {
            // KONIE START
            time = rand() % 10;
            cout << rank << ": resting " << time << "s" << endl;
            sleep(time); // DO STUFF
            cout << rank << ": rested" << endl;
            ACK = 0;
            pthread_mutex_lock(&mutexLamport);
            message[0] = rank;
            message[1] = lamport_clock;
            konieData.lamport_clock = lamport_clock;
            konieData.rank = rank;
            pthread_mutex_unlock(&mutexLamport);

            pthread_mutex_lock(&mutexSend);
            for (int i = 0; i < size; i++)
            {
                MPI_Send(&message, 3, MPI_INT, i, STATUS_KONIE, MPI_COMM_WORLD); // GET KOŃ
                // cout << "Sent \"konie\" request from " << rank << " to " << i
                //<< " with lamport clock " << lamport_clock << endl;
            }
            pthread_mutex_unlock(&mutexSend);
            while (ACK < size);
            cout << rank << ": Waiting for \"koń\"" << endl;
            while (getKonie(konieQueue, konieData) > K)
            {
                // cout<<rank<< ": getkonie:  " << getKonie(konieQueue, konieData) << " K: " << K << endl;
            }
            cout << rank << ": Got  \"koń\"" << endl;
            // KONIE END

            // WSTAZKI START
            int wstazki = rand() % W;
            ACK = 0;

            pthread_mutex_lock(&mutexLamport);
            message[0] = rank;
            message[1] = lamport_clock;
            message[2] = wstazki;
            wstazkiData.lamport_clock = lamport_clock;
            wstazkiData.rank = rank;
            wstazkiData.wstazki = wstazki;
            pthread_mutex_unlock(&mutexLamport);

            pthread_mutex_lock(&mutexSend);
            for (int i = 0; i < size; i++)
            {
                MPI_Send(&message, 3, MPI_INT, i, STATUS_WSTAZKI, MPI_COMM_WORLD); // GET WSTĄŻKA
                // cout << "Sent \"wstazki\" request from " << rank << " to " << i
                //<< " with lamport clock " << lamport_clock << endl;
            }
            pthread_mutex_unlock(&mutexSend);
            while (ACK < size)
                ;
            cout << rank << ": Waiting for \"wstazki\"" << endl;
            while (getWstazki(wstazkiQueue, wstazkiData) > W)
                ;
            cout << rank << ": Got  \"wstazki\"" << endl;
            // WSTAZKI END

            time = rand() % 10;
            cout << rank << ": doing stuff for " << time << "s" << endl;
            sleep(time); // DO STUFF
            cout << rank << ": finished" << endl;

            pthread_mutex_lock(&mutexSend);
            for (int i = 0; i < size; i++)
            {
                MPI_Send(&message, 3, MPI_INT, i, STATUS_SKRZAT_DONE, MPI_COMM_WORLD);
            }
            pthread_mutex_unlock(&mutexSend);
        } else {
            //KONIE START
            ACK = 0;
            pthread_mutex_lock(&mutexPsycho);
            psychoData.rank = rank;
            psychoData.lamport_clock = lamport_clock;
            pthread_mutex_unlock(&mutexPsycho);

            pthread_mutex_lock(&mutexSend);
            for (int i = 0; i < size; i++)
            {
                MPI_Send(&message, 3, MPI_INT, i, STATUS_PSYCHO, MPI_COMM_WORLD); // GET SALKA
            }
            pthread_mutex_unlock(&mutexSend);
            while (ACK < size);
            while (getPsycho(psychoQueue, psychoData) > traumaQueue.size());
            pthread_mutex_lock(&mutexTraumaQueue);
            cout << "size: " << traumaQueue.size() << endl;
            WstazkiData traumaData = traumaQueue[0];
            message[0] = traumaData.rank;
            cout << rank << ": got koń " << traumaData.rank << endl;
            pthread_mutex_lock(&mutexSend);
            for (int i = 0; i < size; i++)
            {
                MPI_Send(&message, 3, MPI_INT, i, STATUS_TRAUMA, MPI_COMM_WORLD);
            }
            pthread_mutex_unlock(&mutexSend);     
            pthread_mutex_unlock(&mutexTraumaQueue);       

            //KONIE END

            //SALKI START
            ACK = 0;
            pthread_mutex_lock(&mutexLamport);
            message[0] = rank;
            message[1] = lamport_clock;
            salkiData.lamport_clock = lamport_clock;
            salkiData.rank = rank;
            pthread_mutex_unlock(&mutexLamport);

            pthread_mutex_lock(&mutexSend);
            for (int i = 0; i < size; i++)
            {
                MPI_Send(&message, 3, MPI_INT, i, STATUS_SALKI, MPI_COMM_WORLD); // GET SALKA
            }
            pthread_mutex_unlock(&mutexSend);
            while (ACK < size);
            while (getSalki(salkiQueue, salkiData) > S);
            cout << rank << ": Got  \"salka\"" << endl;
            //SALKI END

            time = rand() % 10;
            cout << rank << ": fixing horse for " << time << "s" << endl;
            sleep(time); // DO STUFF
            cout << rank << ": finished" << endl;

            pthread_mutex_lock(&mutexSend);
            for (int i = 0; i < size; i++)
            {
                MPI_Send(&message, 3, MPI_INT, i, STATUS_RELEASE_SALKI, MPI_COMM_WORLD);
                MPI_Send(&message, 3, MPI_INT, i, STATUS_RELEASE_WSTAZKI, MPI_COMM_WORLD);
                MPI_Send(&message, 3, MPI_INT, i, STATUS_RELEASE_KONIE, MPI_COMM_WORLD);
            }
            pthread_mutex_unlock(&mutexSend);
        }
    }

    MPI_Finalize();
    return 0;
}
