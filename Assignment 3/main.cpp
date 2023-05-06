#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <pthread.h>
#include "huffmanTree.h"

/*struct arguments to hold information among each threads*/
struct arguments {
    HuffmanTreeNode* root; //Pointer to root of the Huffman Tree.
    std::vector<std::string>* binaryCodes; //Pointer to vector of binary code of each character.
    int index; //Index of current character being processed by Thread.
    int n; //number of line or unique characters. 
    int* current_index; //Pointer to an integer that represents the current index of the character.
    int* thread_counter; //Pointer to an integer that representing the number of threads that have completed.
    std::vector<char>* decompressed_message; //Pointer to the vector representing the decompressed message.
    std::vector<std::vector<int>>* positions; //Pointer to the vector of positions representing the positions of each character in the decompressed message.
    pthread_mutex_t* mutex; //Pointer to the mutex used for synchronization in each critical section.
    pthread_cond_t* cond; //Pointer to condition variable used for synchronization in each critical section.
    pthread_mutex_t* printMutex; //Pointer to mutex used for synchronization for printing symbol, frequency, and code.
    pthread_cond_t* printCond; //Pointer to conditional variable used for synchronization for printing symbol, frequency, and code.
};

/*main Thread function for printing in order and store original message by using synchronization.*/
void* mainThread(void* arg) {
    arguments* main_args = (arguments*)arg; //Cast the input argument to a arguments pointer.

    // Create local args object
    arguments args;
    {
        /*CRITICAL SECTION 1
        Copy main_args values into local args and signal the main thread to continue. 
        Guarantee that each thread has its own copy of the arguments, which prevents any race conditions*/
        pthread_mutex_lock(main_args->mutex); //Lock the mutex therefore no other thread can access main_args.
        args = *main_args; //Copy main_args into local args object
        pthread_cond_signal(main_args->cond); //Signal the main thread to continue creating new threads.
        pthread_mutex_unlock(main_args->mutex); //Unlock the mutex, allowing other threads to access main_args.
    }
    
    /*Use Huffman Tree method to find the binary code and symbol*/
    std::string binaryCode = (*(args.binaryCodes))[args.index];
    char symbol = getChar(args.root, binaryCode);

    /*CRITICAL SECTION 2
    Waiting for the thread's turn to print translation info that only one thread can access the shared current_index variable at a time.
    Guarantee that printing is done in the correct order based on the order we read the compressed file.*/
    pthread_mutex_lock(args.printMutex); //Lock the printMutex therefore no other thread can access current_index.
    while (args.index != *args.current_index) //Wait for it is this thread turn to print.
    { 
        pthread_cond_wait(args.printCond, args.printMutex); //Wait for a signal from another thread.
    }

    // Print the symbol, frequency, and code
    int arr[100]; //Helper array to print.
    traverse(args.root, symbol, arr, 0); //Print the symbol, frequency, and code

    // Write the decoded character to the output array
    for (int pos : (*(args.positions))[args.index]) {
        (*(args.decompressed_message))[pos] = symbol;
    }

    /*CRITICAL SECTION 3:
    Incremnting the shared current_index value to signal that it's the next thread's turn to print.*/
    (*args.current_index)++; //Increment the shared current_index.
    pthread_cond_signal(args.printCond); //Signal the next waiting thread to print.
    pthread_mutex_unlock(args.printMutex); //Unlock the printMutex and allow other threads to access current_index.

    // Increment the thread_counter and signal the main thread if all threads are done
    pthread_mutex_lock(args.mutex); //Lock the mutex to therefore no other thread can access thread_counter.
    (*args.thread_counter)++;//Increment the thread_counter.
    
    if (*args.thread_counter == args.n) //Check if all threads are completed.
    {
        pthread_cond_signal(args.cond); //Signal the main thread that all thread are done.
    }
    pthread_mutex_unlock(args.mutex); //Unlock the mutex and allow other threads to access thread_counter.

    return nullptr;
}

// Driver code
int main() {
    /* Reading input */
    int n;
    std::cin >> n;
    std::cin.ignore();

    // Initialize empty list of characters and frequencies
    std::vector<char> characters(n);
    std::vector<int> frequencies(n);
    int total_characters = 0; // Add a variable to store the total number of characters in the original message

    // Read input for characters and frequencies
    string line;
    for (int i = 0; i < n; ++i) {
        getline(cin, line);
        istringstream iss(line);
        characters[i] = iss.get(); // Use get() to read the first character including whitespace
        iss >> frequencies[i];
        total_characters += frequencies[i]; // Update the total number of characters
    }

    // Initialize priority_queue
    priority_queue<HuffmanTreeNode*, vector<HuffmanTreeNode*>, Compare> pq;
    int nodeCounter = 0; // Add a nodeCounter variable to keep track of node order
    init_pq(characters.data(), frequencies.data(), n, pq, nodeCounter);

    // Build HuffmanTree
    HuffmanTreeNode* root = buildHuffmanTree(pq, nodeCounter);

    // Read input for binary codes and positions
    std::vector<string> binaryCodes(n);
    std::vector<vector<int> > positions(n);

    for (int i = 0; i < n; ++i) {
        getline(cin, line);
        istringstream iss(line);
        iss >> binaryCodes[i];

        int p;
        while (iss >> p) {
            positions[i].push_back(p);
        }
    }

    // Initialize shared decompressed_message vector
    std::vector<char> decompressed_message(total_characters, '\0');
    
    // Initialize mutex and condition variable
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_t cond;
    pthread_cond_init(&cond, nullptr);
    
    // Initialize printMutex and printCond
    pthread_mutex_t printMutex;
    pthread_mutex_init(&printMutex, nullptr);
    pthread_cond_t printCond;
    pthread_cond_init(&printCond, nullptr);
    
    int current_index = 0; //variable to keep track of the order of synchronization among threads when they are printing.
    int thread_counter = 0; //variable to keep track of the number of threads that have completed their tasks.
    
    /*Initialize the arguments object*/
    arguments main_args = {root, &binaryCodes, 0, n, &current_index, &thread_counter, &decompressed_message, &positions, &mutex, &cond, &printMutex, &printCond};
    pthread_t threads[n]; //Create an array of pthread_t threads with the size n.
    for (int i = 0; i < n; ++i) //Loop through each thread.
    { 
        main_args.index = i; //Set the index of the current thread in the main_args.

        /*Critical section to ensure synchronization between the parent thread and the child threads during their creation.*/
        pthread_mutex_lock(&mutex); //Lock the mutex to protect shared resources during thread creation
        if (pthread_create(&threads[i], nullptr, mainThread, &main_args)) //Create a new thread and start the mainThread function with the main_args as an argument.
        {
            fprintf(stderr, "Error creating thread\n");
			return 1;
        }
        pthread_cond_wait(&cond, &mutex); //Wait for the signal from the newly created thread, indicating that it has copied the main_args data.
        pthread_mutex_unlock(&mutex); //Unlock the mutex after the thread has copied the main_args data.
    }

    /*Wait for all threads to finish using pthread_join.
    Guarantee that the parent thread waits for all its child threads to end before ending its execution*/
    for (int i = 0; i < n; ++i) {
        pthread_join(threads[i], nullptr);
    }

    //Clean up mutex and condition variable
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&printMutex);
    pthread_cond_destroy(&printCond);
    
    //Delete Huffman Tree
    delete(root);

    // Print the original message
    cout << "Original message: ";
    for (char ch : decompressed_message) {
        std::cout << ch;
    }
    std::cout << endl;

    return 0;
}