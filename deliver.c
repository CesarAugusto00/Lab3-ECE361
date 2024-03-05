
/* A simple UDP client which measures round trip delay */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <string.h>
#include <time.h> 
#include <poll.h>
#include <errno.h>
#include <math.h>

//Note that the error comes from the first retransmission needed due to a lost ACK 


#define SERVER_UDP_PORT 5000
#define MAXLEN 4096 /* maximum data length */
#define DEFLEN 64 /* default length */
#define PACKET_SIZE 1000


long delay(struct timeval t1, struct timeval t2);
long delayMicroSeonds (struct timeval t1, struct timeval t2);
long long delayNanoseconds(struct timespec t1, struct timespec t2);
//calculates the time rtt and copy into a third value 
void delaycpynano(struct timespec t1, struct timespec t2, struct timespec t3);

bool file_exists(const char *filename);

size_t getFileSize(const char *fileName);

size_t getItinerationFileSize(const char *fileName);


int main(int argc, char **argv)
    {
    bool rrTest = true; 
    int data_size = DEFLEN, port = SERVER_UDP_PORT;
    int i, j, sd, server_len;
    char *pname, *host, rbuf[4], sbuf[4];
    struct hostent *hp;
    struct sockaddr_in server;

    struct timeval start, end, st, en;
    struct timeval startSampleRTT, endSampleRTT, tSampleRTT, tTotal;
    struct timeval timeout;
    //INITIALIZATION OF FIRST TIME OF TIMEOUT.
    timeout.tv_usec=0;
    timeout.tv_sec =1;

    long  sampleRTT;
    double testRTT; 
    double EstimatedRTT = 0.0;
    double DevRTT = 0.0;

    bool check;
    const char* inputString[256];
    unsigned char semicolon[2]= ":";
    unsigned char ACKR[4];
    char word[100];
    char word2[100];
    char temp[50];
    bool areAllTheFlagsSended = false;
    //stringWord will hold the string which will be copied to the sending buff
    unsigned char stringWord[100]; 
    unsigned char SBUF[MAXLEN], RBUF[MAXLEN];
    const char* confirming = "yes";
    //Here goes the information of the structure of the values.
    size_t total_frag;
    size_t frag_no;
    size_t size;
    int iterator = 0;

    bool firstTransmission = true;
    pname = argv[0];
    argc--;
    argv++;
    if (argc > 0 && (strcmp(*argv, "-s") == 0)) {
        if (--argc > 0 && (data_size = atoi(*++argv))) {
            argc--;
            argv++;
        }
        else {
            fprintf (stderr, "Usage: %s [-s data_size] host [port]\n", pname);
            exit(1);
        }
    }
    if (argc > 0) {
        host = *argv;
        if (--argc > 0)
            port = atoi(*++argv);
    }
    else {
        fprintf(stderr, "Usage: %s [-s data_size] host [port]\n", pname);
        exit(1);
    }
    /* Create a datagram socket */
    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Can’t create a socket\n");
        exit(1);
    }
    /* Store server’s information */
    bzero((char *)&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if ((hp = gethostbyname(host)) == NULL) {
        fprintf(stderr, "Can’t get server’s IP address\n");
        exit(1);
    }
    bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
    if (data_size > MAXLEN) {
        fprintf(stderr, "Data is too big\n");
        close(sd);
        exit(1);
    }

    // Here it will send the value
    // fgets(inputString, sizeof(inputString), stdin);
    // word = strtok(inputString, " ");
    // word2 = strtok(NULL, " ");
    // printf("%s\n", word2);
    // word2[strcspn(word2, "\n")] = '\0';
    scanf("%s", word);
    scanf("%s", word2);
    check = file_exists(word2); 
    if(check == true){
        strcpy(sbuf, word);
    }
    else{
        printf("File does not exist");
        exit(1);
    }


    gettimeofday(&start, NULL); /* start delay measure */
    /* transmit data */
    server_len = sizeof(server);
    if (sendto(sd, sbuf, 4, 0, (struct sockaddr *) &server, server_len) == -1) {
        fprintf(stderr, "sendto error\n");
        exit(1);
    }
    /* receive data */
    if (recvfrom(sd, rbuf, 4, 0, (struct sockaddr *) &server, &server_len) < 0) {
        fprintf(stderr, "recvfrom error\n");
        exit(1);
    }  
    else{
        printf("Server Replied %s \n", rbuf);
        if(strcmp(rbuf, "yes") == 0 ){
            printf("A file transfer can start.\n");
            //HERE WE START WITH THE TRANSMISSION 
            total_frag = getItinerationFileSize(word2);
            frag_no = 1;
            //note frag size is while getting the files out
            //The file name is alredy in word2 
            //and the data will be allocated at the end of the unsigned char array 

            FILE* source;
            unsigned char buffer[1000];
            int ch;

            source = fopen(word2, "rb");
        
            if (NULL == source) {
                printf("file can't be opened \n");
                exit(1);
            }

            size = fread(buffer, 1, sizeof(buffer), source);
            while (!areAllTheFlagsSended) {
                sprintf(temp, "%zu", total_frag);
                strcpy(stringWord, temp);
                strcat(stringWord, semicolon);
                sprintf(temp, "%zu", frag_no);
                strcat(stringWord, temp);
                strcat(stringWord, semicolon);
                sprintf(temp, "%zu", size);
                strcat(stringWord, temp);
                strcat(stringWord, semicolon);
                strcat(stringWord, word2);
                strcat(stringWord, semicolon);
                //printf(">>%s\n", stringWord);
                //Length contains the length of the file+999
                int length=0;
                iterator = 0;
                //Now stringWord contains the string of info. We need to append the buffer into before sending it 

                while(stringWord[iterator] != '\0'){
                    SBUF[iterator] = stringWord[iterator];
                    iterator++;
                    length ++;
                }

                for(int i=iterator; i <= (iterator + size -1) ; ++i){
                    SBUF[i] = buffer[i - iterator];
                }
                //BECAUSE HERE WE SEND OF FIRST PACK OF DATA IT WILL START THE TIMER HERE 
                //BECAUSE WE ARE SENDING THE DATA TWICE WE GET THE VALUE OF RTT UNTIL AFTER WE GET THE ACK 
                //clock_gettime(CLOCK_MONOTONIC,&startSampleRTT);
                gettimeofday(&startSampleRTT, NULL);
                gettimeofday(&st, NULL);
                //Here we send the first data pack
                if (sendto(sd, SBUF, (length+size), 0, (struct sockaddr *) &server, server_len) == -1) {
                    fprintf(stderr, "sendto error\n");
                    exit(1);
                }

                //Here we send the second packet
                if (sendto(sd, SBUF, (length+size), 0, (struct sockaddr *) &server, server_len) == -1) {
                    fprintf(stderr, "sendto error\n");
                    exit(1);
                }

                //HERE WE PUT THE timeout 
                // timeout.tv_nsec=0;
                // timeout.tv_sec =1;
                //printf("The timout is in here %ld and %ld micro packet %zu\n",timeout.tv_sec, timeout.tv_usec, frag_no);
                 if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                    perror("setsockopt");
                    exit(EXIT_FAILURE);
                }
                
                //Here get the  ACK 
                int output = recvfrom(sd, ACKR, 4, 0, (struct sockaddr *) &server, &server_len);
                //printf("There was an error %zu packet %zu\n",  output, frag_no);
                if (output < 0 ){
                    //printf("There was an error %zu packet %zu\n",  output, frag_no);
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        //Here it means we need a retransmission 
                        //printf("Retransmission needed on packet %zu \n",frag_no);
                        //printf("Timeout first Retransmission %ld packet %zu\n", timeout.tv_usec, frag_no);
                        printf("Timout. Packet %zu ACK not acknowledge \n", frag_no);
                        //We need to readjust the timeout for double the time set to sent the value
                        long currentTimeout = (timeout.tv_sec * 1000000) + timeout.tv_usec;
                        currentTimeout *= 2;
                        timeout.tv_sec = currentTimeout/1000000;
                        timeout.tv_usec = currentTimeout%1000000;
                        firstTransmission = false;
                    }
                    else{
                        fprintf(stderr, "recvfrom error\n");
                        exit(1);
                    }
                    
                }  
                else{
                    //Get the end in nano seconds 
                    //clock_gettime(CLOCK_MONOTONIC,&endSampleRTT);
                    gettimeofday(&endSampleRTT, NULL);
                    //get value in miliseconds 
                    gettimeofday(&en, NULL);

                    //Here we find the values of RTT and the transmission if only transmitted once 
                    if(firstTransmission && (delayMicroSeonds(startSampleRTT, endSampleRTT) > 0)){
                        
                        long sampleMicroSecondsRTT;
                        sampleMicroSecondsRTT = (((endSampleRTT.tv_sec - startSampleRTT.tv_sec)*1000000) + (endSampleRTT.tv_usec - startSampleRTT.tv_usec));
                        EstimatedRTT = (0.875*EstimatedRTT) + ((0.125)*sampleMicroSecondsRTT);
                        DevRTT = (0.75*DevRTT) +  (fabsl(sampleMicroSecondsRTT-EstimatedRTT));
                        long TimeoutInterval = (long)(EstimatedRTT + 4*DevRTT);
                        timeout.tv_sec = TimeoutInterval / 1000000;
                        timeout.tv_usec = TimeoutInterval%1000000;
                        //printf("Timeout first Retransmission %ld packet %zu\n", timeout.tv_usec, frag_no);
                        //printf("The time from the funciton is %ld and here is sample in microseconds %ld \n", delayMicroSeonds(startSampleRTT, endSampleRTT),sampleMicroSecondsRTT);
                        //printf("The value of timout is %ld seconds and %ld usec \n", TimeoutInterval/1000000, TimeoutInterval%1000000);
                    }
                    
                    if (strcmp(ACKR, "oki") == 0){
                        //Here it means that the transmission was correct so no retrasmission needed
                        //Here we also check if we are done transmiting or not
                        //printf("The number of this frag is %zu \n The total frag is %zu \n", frag_no, total_frag);
                        if(frag_no == total_frag){
                            //we are done 
                            fclose(source);
                            printf("All the data was transmitted \n");
                            areAllTheFlagsSended = true;
                        }
                        else{
                            frag_no++;
                            firstTransmission = true;
                            size = fread(buffer, 1, sizeof(buffer), source);
                        }
                        
                    }
                    else if (strcmp(ACKR, "rpt") == 0){
                        //Then it means that a retranmission is needed 
                        printf("Resending packet due makfunction #%zu \n", frag_no);
                        firstTransmission = false;
                    }
                    else{
                        //here it measn that there was an error while reciving ACK 
                        printf("Error reciving ACK \n");
                        exit(1);
                    }
                }
                    
                
                
                
            }   

        }

        else if(strcmp(rbuf, "no")  == 0){
            //printf("The sever responded with no \n");
            exit(1);
        }
        else{
            printf("No confirmation\n");
            exit(1);
        }
    }



    gettimeofday(&end, NULL); /* end delay measure */
    printf("Time program = %ld ms.\n", delay(start, end));
    close(sd);
    return(0);
    }

/*
* Compute the delay between t1 and t2 in milliseconds
*/
long delay (struct timeval t1, struct timeval t2)
{
    long d;
    d = (t2.tv_sec - t1.tv_sec) * 1000;
    d += ((t2.tv_usec - t1.tv_usec + 500) / 1000);
    return d;
}

long delayMicroSeonds (struct timeval t1, struct timeval t2)
{
    long d;
    d = (t2.tv_sec - t1.tv_sec) * 1000000;
    d += (t2.tv_usec - t1.tv_usec );
    return d;
}
//This function if correct lets try to do it directly 
long long delayNanoseconds(struct timespec t1, struct timespec t2){
    long long d;
    d = (t2.tv_sec - t1.tv_sec) * 1000000000LL;
    d += ((t2.tv_nsec - t1.tv_nsec ));
    return d;
}

void delaycpynano(struct timespec t1, struct timespec t2, struct timespec t3){
    t3.tv_sec = (t2.tv_sec - t1.tv_sec);
    t3.tv_nsec = ((t2.tv_nsec - t1.tv_nsec ));
}

bool file_exists(const char *filename)
{
    //function extracted from the web https://www.learnc.net/c-tutorial/c-file-exists/
    FILE *fp = fopen(filename, "r");
    bool doesExist = false;
    if (fp != NULL)
    {
        doesExist = true;
        fclose(fp); // close the file
    }
    return doesExist;
}


size_t getFileSize(const char *fileName) {
    FILE *file = fopen(fileName, "rb");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);  // Move the file pointer to the end
    size_t fileSize = ftell(file);  // Get the position, which is the file size
    fclose(file);

    return fileSize;
}

size_t getItinerationFileSize(const char *fileName) {
    //FOR A REASON i CAN'T USE CEIL. Might be the order.
    size_t fragmentsTotal;  
    FILE *file = fopen(fileName, "rb");

    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);  // Move the file pointer to the end
    size_t fileSize = ftell(file);  // Get the position, which is the file size
    fclose(file);
    if(fileSize <= 1000){
        fragmentsTotal = 1;
    }
    else{
        fragmentsTotal = fileSize/1000;
        if (fileSize % 1000 != 0){
            fragmentsTotal++;
        }
    }
    return (fragmentsTotal);

}