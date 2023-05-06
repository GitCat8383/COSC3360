#include "huffmanTree.h"
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <pthread.h>

/*the arguments struct is designed for multithread to hold data for the decompression task*/
struct arguments
{
    /*Receives the information about the symbol to decompress (binary code and list of positions) from the main thread.*/
    string hostname; //server host name 
    int port; //port number
    string binaryCode; //binaryCode reads from STDIN
    vector<int> positions; //list of positions read from STDIN
    char *decompressedChars; //pointer to the decompressed string's memory location
};


/* The decompress function's task is connecting to the server, sending the binary code, have the server to convert it into 
character, then receiving the decompressed character from the server, and updating the decompressed string according to 
list of positions.*/
void *decompress(void *arg)
{
    /*Cast the void pointer to arguments pointer.*/
    arguments *args = (struct arguments *)arg;
    
    /*Variables use for create and connect socket.*/
    int sockfd;  //the server socket file descriptor.
    struct sockaddr_in serv_addr; //the server address's information.
    struct hostent *server; //pointer to hostent struct that store information about the server's host.

    /*Create a TCP socket to communicate with the server program.*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    /*Check if the socket creation was successful.*/
    if (sockfd < 0)
    {
        std::cerr << "ERROR opening socket" << std::endl;
        pthread_exit(NULL);
    }
    
    /*Resolve server's hostname.*/
    server = gethostbyname(args->hostname.c_str());
    /*Check if hosting is succesful.*/
    if (server == NULL)
    {
        std::cerr << "ERROR no such host" << std::endl;
        exit(0);
    }

    /*Initialize server address structure.*/
    bzero((char *)&serv_addr, sizeof(serv_addr)); //Clear the memory of the serv_addr structure.
    serv_addr.sin_family = AF_INET;//indicating that the server will use IPv4 addresses.
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length); //copies the first IP address associated with the resolved hostname into the server address structure.
    serv_addr.sin_port = htons(args->port); //assigns the server's port number.

    //Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "ERROR connecting" << std::endl;
        exit(1);
    }

    /*Send the binary code to the server using socket.*/
    string binaryCode = args->binaryCode; //create local string binary code.
    int binaryCodeSize = binaryCode.size()+1; //create the size of the binary code.
    int n; //variable used to store the return value of the read() and write() functions.
    n=write(sockfd, &binaryCodeSize, sizeof(int)); //writes the binaryCodeSize value to the socket to inform the server how many bytes will be sent for the binary code.
    /*Check if writing size of binary code to sever is successful*/
    if (n < 0) 
    {
        std::cerr << "ERROR writing binaryCodeSize to socket" << std::endl;
        pthread_exit(NULL);
    }
    n=write(sockfd, binaryCode.c_str(), binaryCodeSize);//writes binaryCode to the socket.
    /*Check if writing binary code to sever is successful*/
    if (n < 0) 
    {
        std::cerr << "ERROR writing binaryCodeSize to socket" << std::endl;
        pthread_exit(NULL);
    }

    /*Wait for the decoded representation of the binary code (character) from the server*/
    char decodedChar; 
    n=read(sockfd, &decodedChar, sizeof(char)); //Receive the decode character from the server.
    /*Check if receive the character from the sever is succesful*/
    if (n < 0)
    {
        std::cerr << "ERROR reading decoded character from socket" << std::endl;
        pthread_exit(NULL);
    }

    /*Write the received information into a memory location accessible by the main thread.*/
    for (int pos : args->positions)
    {
        args->decompressedChars[pos] = decodedChar;
    }

    // Close the socket
    close(sockfd);

    return NULL;
}

int main(int argc, char *argv[])
{
    /*check if the client provide enough command line arguments*/
    if (argc<3)
    {
        std::cerr<<"usage "<<argv[0]<<"hostname port"<<std::endl;
        exit(0);
    }

    /*Receive user input from STDIN*/
    std::string line; //Initiate the number of line.
    std::vector<std::string> binaryCodes; //Initiate an empty array of binaryCodes.
    std::vector<std::vector<int>> positions; //Initiate an empty 2D array of positions.

    /*Start reading input from compressed files*/
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string binaryCode;
        iss >> binaryCode;

        std::vector<int> pos;
        int p;
        while (iss >> p) {
            pos.push_back(p);
        }

        binaryCodes.push_back(binaryCode);
        positions.push_back(pos);
    }

    int m = binaryCodes.size(); //Assign m to the number of line for the use of POSIX threads.
    
    /*The size of the decompressed file, calculated by finding the maximum position value to calculate decompressed size.*/
    int decompressedSize = 0;
    for (const auto &pos : positions)
    {
        int maxPos = *max_element(pos.begin(), pos.end());
        decompressedSize = max(decompressedSize, maxPos + 1);
    }

    std::string decompressedString(decompressedSize, '\0'); //Initiate a string to store the decompressed data and fill it with null characters.
    std::vector<pthread_t> threads(m);//Initiate a vector to store the thread IDs for 'm' threads.
    std::vector<arguments> argsList(m);//Initiate a vector to store the argument structures for 'm' threads.

    /*Setting up arguments structure for each thread*/
    for (int i = 0; i < m; ++i)
    {
        argsList[i].hostname = argv[1]; //Assign the hostname.
        argsList[i].port = atoi(argv[2]); //Assign the port number.
        argsList[i].binaryCode = binaryCodes[i]; //Asssign the binary code.
        argsList[i].positions = positions[i]; //Assign the list of position.
        argsList[i].decompressedChars = decompressedString.data(); //Set the pointer to the decompressed string's data.
    
        /*Create a new thread with the decompress function and pass the arguments for the current thread*/
        if (pthread_create(&threads[i], NULL, decompress, (void *)&argsList[i]))
        {
            fprintf(stderr, "Error creating thread\n");
			    return 1;
        }
    }
    
    /*Joins m threads together*/
    for (int i = 0; i < m; ++i)
    {
        pthread_join(threads[i], NULL);
    }
        
    /*Output the message*/
    std::cout << "Original message: ";
    std::cout << decompressedString << std::endl;

    return 0;
}