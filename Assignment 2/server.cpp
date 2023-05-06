#include "huffmanTree.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h> 
#include <vector>

/*fireman function is used to handle the termination of child processes.*/
void fireman(int)
{
   while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[])
{   
    /*Declare integer variables for the server socket file descriptor (sockfd), new socket 
    file descriptor for the connected client (newsockfd), port number (portno), and client 
    address length (clilen).*/
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n; //variable used to store the return value of the read() and write() functions.
    /*Set up a signal handler for the SIGCHLD signal, which is sent when a child process terminates.*/ 
    signal(SIGCHLD, fireman); 
    /*Check if the user provided a port number as a command-line argument.*/
    if (argc < 2)
    {
        std::cerr << "ERROR, no port provided"<<std::endl;
        exit(1);
    }
    
    // Read the alphabet information from standard input
    std::vector<char> symbols;
    std::vector<int> frequencies;
    std::string line;
    
    while (std::getline(std::cin, line))
    {
        if (line.empty()) {
            break;
        }
        
        char symbol = line[0];
        int frequency = std::stoi(line.substr(2));
        symbols.push_back(symbol);
        frequencies.push_back(frequency);
    }
    
    int nodeCounter=0; //variable to help build the Huffman Tree
    // Create a priority queue using the symbols and frequencies
    priority_queue<HuffmanTreeNode*, vector<HuffmanTreeNode*>, Compare> pq;
    pq = init_pq(symbols.data(), frequencies.data(), symbols.size(), pq, nodeCounter);
    
    // Build the Huffman tree
    HuffmanTreeNode* huffman_tree = buildHuffmanTree(pq, nodeCounter);
    
    // Print the Huffman tree result
    encode(huffman_tree);
    
    /*Create a new TCP socket.*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    /*Check if the socket creation was successful*/
    if (sockfd < 0)
    {
        std::cerr << "ERROR opening socket"<<std::endl;
        exit(1);
    }
    
    /*Code to initialize server address structure*/
    bzero((char *)&serv_addr, sizeof(serv_addr)); //Clear the memory of the serv_addr structure
    portno = atoi(argv[1]); //Convert the port number provided as a command-line argument
    serv_addr.sin_family = AF_INET; //indicating that the server will use IPv4 addresses.
    serv_addr.sin_addr.s_addr = INADDR_ANY; //indicate that the server will bind to all available interfaces on the machine.
    serv_addr.sin_port = htons(portno); //Set the server's port number to the port number provided by the user
    /*Check if the socket binding was successful*/
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
        std::cerr << "ERROR on binding" << std::endl;
        exit(1);
    }
    
    listen(sockfd, 10); //Start listening for incoming connections on the socket. 20 is the maximum number of pending connection.
    clilen = sizeof(cli_addr); //Set the length of client address structure.
    while (true) //Set a loop to continuously accept the incoming connection.
    {
        /*Accept an incoming connection and create a new socket for the client.*/ 
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen); 
        if (fork() == 0) //new child process to handle the request from the client.
        {
            /*Check if there was an error on accepting incoming request*/
            if (newsockfd < 0)
            {
                std::cerr << "ERROR on accept" << std::endl;
                exit(1);
            }
    
            /*Receiving and decoding binary code, and sending decoded character*/
            int binary_code_length;
            n = read(newsockfd, &binary_code_length, sizeof(int));/* Receive the binary code length and the binary code itself from the client.*/
            if (n < 0)
            {
                std::cerr << "ERROR reading from socket" <<std::endl;
                exit(1);
            }
            /*Allocate memory for the binary_code_buffer to store the binary code from the client.*/
            char *binary_code_buffer = new char[binary_code_length]; //Buffer size is set to binary_code_length + 1 to include an extra space for the null character at the end of the string
            bzero(binary_code_buffer, binary_code_length + 1); //Clear the memory of the binary_code_buffer to avoid any garbage data.
            /*Read the actual binary code from the socket (newsockfd) into the binary_code_buffer.
            The number of bytes to read is determined by binary_code_length.*/
            n = read(newsockfd, binary_code_buffer, binary_code_length);
            /*Check if there was an error reading from the socket.*/
            if (n < 0)
            {
                std::cerr << "ERROR reading from socket" << std::endl;
                exit(1);
            }
            std::string binary_code(binary_code_buffer); //Convert the binary_code_buffer to string.

            char decoded_char = getChar(huffman_tree,binary_code); //Decode the binary code using the Huffman tree getChar method.
    
            //Send the decoded character back to the client.
            n = write(newsockfd, &decoded_char, sizeof(char));
            /*Check if there was an error sending decode char back to client*/
            if (n < 0)
            {
                std::cerr << "ERROR writing to socket" << std::endl;
                exit(1);
            }
    
            //Clear binary_code_buffer.
            delete[] binary_code_buffer;
    
            //End new process.
            close(newsockfd);
            _exit(0);
        }
    }
    close(sockfd); //Close socket.
    return 0;
}