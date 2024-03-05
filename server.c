#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> 
#include <netinet/in.h>
#include <stdbool.h>

#define SERVER_UDP_PORT 5000 /* well-known port */
#define MAXLEN 4096 /* maximum data length */

bool checkEqual(const unsigned char *array1, const unsigned char *array2, int n);
int getSizeFileRecived(const unsigned char *array1);
bool isLastFrag(const unsigned char *array1);
int getPacketNumber( const unsigned char *array1);
void getNameOfFile(const unsigned char *array1, unsigned char *name);
double uniform_rand();

int main(int argc, char **argv)
{
	int sd, client_len, port, n;
	char buf[4];
	char rbuf[4];
	//note the RBUF2 is the recive buf. This is to compare between both to check errors
	unsigned char RBUF[MAXLEN], RBUF2[MAXLEN];
	unsigned char SBUF[MAXLEN];
	unsigned char ACKS[4];
	struct sockaddr_in server, client;
	const char input_string[4] = "ftp";
	bool flagOpenFile = false;  					//Check if file is open to write if not then it opens it 
	bool doneWithAllFrags = false;
	bool firstTimeReciving8 = true;
	switch(argc) {
		case 1:
			port = SERVER_UDP_PORT;
			//printf("Port 5000 default \n");
			break;
		case 2:
			port = atoi(argv[1]);
			//printf("Using other port %d \n", argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

	/* Create a datagram socket */	
	if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		fprintf(stderr, "Can’t create a socket\n");
		exit(1);
	}
	/* Bind an address to the socket */
	bzero((char *)&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *) &server,
	sizeof(server)) == -1) {
		fprintf(stderr, "Can’t bind name to socket\n");
		exit(1);
	}

	while (1) {
		client_len = sizeof(client);
		if ((n = recvfrom(sd, buf, 4, 0, (struct sockaddr *)&client, &client_len)) < 0) {
			fprintf(stderr, "Can’t receive datagram\n");
			exit(1);
		}
		else{
			printf("buf  %s \n", buf);
			//it means we recieved a message. Then we check if the message is ftp 
			if(strcmp(buf, "ftp") == 0){
				
				strcpy(rbuf, "yes");
				//here we send back the answer
				if ( sendto(sd, rbuf, 4, 0,(struct sockaddr *)&client, client_len) < 0) {
					fprintf(stderr, "Can’t send datagram\n");
					exit(1);
				}
				else{
					//before the shile we get the name of the file to open it MAYBE WE WILL SEND THE NAME OF FILE BEFORE
						//STARTING sending the files
					
					//Set the File destination 
					FILE* destination;
					
					

					while(!doneWithAllFrags){
						if ((n = recvfrom(sd, RBUF, MAXLEN, 0, (struct sockaddr *)&client, &client_len)) < 0) {
							fprintf(stderr, "Can’t receive datagram\n");
							exit(1);
						}
						// else{
						// 	//printf("we reciever the first packet :v and n is %d \n",n);
						// }
						//Here we recive the second packet :v 
						if ((n = recvfrom(sd, RBUF2, MAXLEN, 0, (struct sockaddr *)&client, &client_len)) < 0) {
							fprintf(stderr, "Can’t receive datagram\n");
							exit(1);
						}

						//for testing purposes I will not send an ack for the packet 8. to see how does the program behaves 
						// if ((getPacketNumber(RBUF) == 27424) && (firstTimeReciving8)){
						// 	//here we don't do anything because of the retransmission 
						// 	printf("We drop the ack for packet %d\n ",getPacketNumber(RBUF));
						// 	firstTimeReciving8 = false;
						// }
						//Here we drop some of the packets 
						if(uniform_rand() > .0001){
							//here we check if the files are equal if equal then ack okay if not then retransmits 
							if(checkEqual(RBUF, RBUF2, n)){
								//If they are equal then we send a signal that we recived the correct file and we put the bytes in the new file.
								strcpy(ACKS, "oki");
								if ( sendto(sd, ACKS, 4, 0,(struct sockaddr *)&client, client_len) < 0) {
									fprintf(stderr, "Can’t send datagram\n");
									exit(1);
								}
								//After making sure the file is correct then we open the file for the first time 
								if(!flagOpenFile){
									//get the name of the file
									unsigned char fileName[50];
									getNameOfFile(RBUF, fileName);
									//open the file to write 
									destination = fopen(fileName, "wb");
									//change the flag to indicate that is alredy open and ready to write in it
									flagOpenFile = true;
								}
								//FIRST I NEED TO EXTRACT THE BYTES OF THE FILE //will do using malloc 
								//then we write using fwrite 
								//then we free memeory allocation
								
								int sizeOfBytesFile = getSizeFileRecived(RBUF);
								
								// for(int i = 0; i<=(n-sizeOfBytesFile); i++){
								// 	printf("%c", RBUF2[i]);
								// }
								// printf("\n");

								unsigned char* L = (unsigned char*)malloc( sizeOfBytesFile * sizeof(unsigned char));
								if(L ==NULL){
									printf("Error alocating memory line 131 \n");
									exit(1);
								}

								for (int i= n-sizeOfBytesFile; i < (n) ; i++ ){
									L[i - (n-sizeOfBytesFile) ] = RBUF2[i];
								}
								fwrite(L,1,sizeOfBytesFile,destination);
								free(L); 
								
								if(isLastFrag(RBUF)){
									//here we send the signal to terminate the loop and close the file stream
									//otherwise it can continue reading
									//printf("LAST FRAG WE DONE \n");
									doneWithAllFrags = true;
									//here we close the file 
									fclose(destination);
									flagOpenFile = false;
								}


							}
							//Here is the ACK to show that no packet was recived. 
							else{
								//This mean that we need a retrnsmission of the same file
								//If they are equal then we send a signal that we recived the correct file and we put the bytes in the new file.
								strcpy(ACKS, "rpt");
								printf("Failed To Recive Packet #%d \n", getPacketNumber(RBUF));
								if ( sendto(sd, ACKS, 4, 0,(struct sockaddr *)&client, client_len) < 0) {
									fprintf(stderr, "Can’t send datagram\n");
									exit(1);
								}
							}
						}
						else{
							printf("Lost ACK of packet #%d \n",getPacketNumber(RBUF));
						}
						
						
					}
					//Here we need to clear and reset all previous values modify so a new transfer can start.
					doneWithAllFrags = false;

				}
			}
			else{
				strcpy(rbuf,"no");
				if ( sendto(sd, rbuf, 4, 0,(struct sockaddr *)&client, client_len) < 0) {
					fprintf(stderr, "Can’t send datagram\n");
					exit(1);
				}
			} 
		}
		
	}
	close(sd);
	return(0);
}

bool checkEqual(const unsigned char *array1, const unsigned char *array2, int iterval){
	bool check =true;
	for(int i =0; i< iterval; i++){
		if(array1[i] != array2[i]){
			check = false;
			i = iterval;
		}
	}
	return check;
}

int getSizeFileRecived( const unsigned char *array1){
	bool isTheLastFrag = false;
	int index[4];
	int iterIndex = 0;
	bool lookingFourColons=true;
	int iterGneral = 0;

	int size;


	while(lookingFourColons){
		if(array1[iterGneral] == ':'){
			index[iterIndex] = iterGneral;
			iterIndex++;
		}
		//we found the three colons
		if(iterIndex == 3){
			lookingFourColons = false;
		}
		iterGneral++;
	}

	//Because I alredy have the values in the array of the index I can iterate throught them 
	if( (index[2]-index[1] -1 ) > 1){ 
		size = array1[index[1]+1] - '0';
		for(int i=(index[1]+2); i<index[2]; i++ ){
			size = (size*10) + (array1[i] - '0');
		}
	}
	else{
		size = array1[(index[1]+1)] - '0';
	}
	//printf("The size number is %d /n",size);
	return size;
}	

bool isLastFrag(const unsigned char *array1){
	bool isTheLastFrag = false;
	int index[4];
	int iterIndex = 0;
	bool lookingFourColons=true;
	int iterGneral = 0;
	int totalFrag;
	int atFrag;

	while(lookingFourColons){
		if(array1[iterGneral] == ':'){
			index[iterIndex] = iterGneral;
			iterIndex++;
		}
		//we found the three colons
		if(iterIndex == 4){
			lookingFourColons = false;
		}
		iterGneral++;
	}

	//Because I alredy have the values in the array of the index I can iterate throught them 
	if( (index[1]-index[0] -1 ) > 1){ 
		atFrag = array1[index[0]+1] - '0';
		for(int i=(index[0]+2); i<index[1]; i++ ){
			atFrag = (atFrag*10) + (array1[i] - '0');
		}
	}
	else{
		atFrag = array1[(index[0]+1)] - '0';
	}

	//Then here we get the total frag
	if( (index[0]) > 1){ 
		totalFrag = array1[0] - '0';
		for(int i=(1); i<index[0]; i++ ){
			totalFrag = (totalFrag*10) + (array1[i] - '0');
		}
	}
	else{
		totalFrag = array1[0] - '0';
	}

	//printf("Frag %d - Total %d \n", atFrag, totalFrag);

	if(atFrag == totalFrag){
		isTheLastFrag= true;
	}
	return isTheLastFrag;
	
}


int getPacketNumber( const unsigned char *array1){
	bool isTheLastFrag = false;
	int index[4];
	int iterIndex = 0;
	bool lookingFourColons=true;
	int iterGneral = 0;
	int atFrag;

	int size;


	while(lookingFourColons){
		if(array1[iterGneral] == ':'){
			index[iterIndex] = iterGneral;
			iterIndex++;
		}
		//we found the three colons
		if(iterIndex == 4){
			lookingFourColons = false;
		}
		iterGneral++;
	}

	//here we get the number of the fragment 
	if( (index[1]-index[0] -1 ) > 1){ 
		atFrag = array1[index[0]+1] - '0';
		for(int i=(index[0]+2); i<index[1]; i++ ){
			atFrag = (atFrag*10) + (array1[i] - '0');
		}
	}
	else{
		atFrag = array1[(index[0]+1)] - '0';
	}

	return atFrag;
}	

void getNameOfFile(const unsigned char *array1, unsigned char *name){
	bool isTheLastFrag = false;
	int index[4];
	int iterIndex = 0;
	bool lookingFourColons=true;
	int iterGneral = 0;

	while(lookingFourColons){
		if(array1[iterGneral] == ':'){
			index[iterIndex] = iterGneral;
			iterIndex++;
		}
		//we found the three colons
		if(iterIndex == 4){
			lookingFourColons = false;
		}
		iterGneral++;
	}

	//here we get the number of the fragment 
	for(int i = (index[2]+1); i <= index[3]; i++ ){
		if( i == index[3]){
			name[i - (index[2]+1)] = '\0'; 
		}
		else{
			name[i - (index[2]+1)] = array1[i];
		}		
	}
	for(int i = (index[3]-index[2]) ; i < sizeof(name); i++){
		name[i] = '\0';
	}
	//printf("the NAME IS -> %s \n ", name);
	//

}	

double uniform_rand(){
	int randomNumber = rand();
	double randomDecimal = ((double)randomNumber)/RAND_MAX;
	return randomDecimal;
	//return 0.1;
}