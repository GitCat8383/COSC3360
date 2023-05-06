#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <map>
#include <queue>
#include <sstream>
#include <string>

using namespace std;

//define Huffman Tree
//Huffman tree node is define by its character, frequency, left node, right node, left edge, and right edge
struct HuffmanTreeNode
{
    char character;
    int frequency;
    HuffmanTreeNode* left;
    HuffmanTreeNode* right;
    string labelLeft;
    string labelRigth;
    
    //initialize the Node
    HuffmanTreeNode (char ch, int freq, string labelLeft, string labelRigth)
    {
        character=ch;
        frequency=freq;
        left=right=NULL;
        labelRigth="";
        labelLeft="";
    }
};

//Implementation of Huffman Tree function class
//Modify compare class for PQ
class Compare{
public: 
    //Arrange the symbols based on their frequencies. If two or more symbols have the same frequency, they will be sorted based on their ASCII value.
    //Insert internal node into the queue of nodes as the lowest node based on its frequency.
    bool operator() (HuffmanTreeNode* first, HuffmanTreeNode* second)
    {
        if (first->frequency==second->frequency) 
        {
            if (first->character==second->character)
            {
                return first < second;//compare the order if frequency of internal node is equal
            }
            return (int)(first->character)>(int)(second->character); //compare ascii value if frequency is equal
        }
        return first->frequency > second->frequency; //compare frequency 
    }
};

//initialize priority_queue
priority_queue<HuffmanTreeNode*, vector<HuffmanTreeNode*>, Compare> init_pq(char character[], int frequency[], int size, priority_queue<HuffmanTreeNode*, vector<HuffmanTreeNode*>, Compare>& pq)
{
    //initialize priority_queue
    for (int i = 0; i < size; i++)
    {
        //initialize new HuffmanTree Node
        HuffmanTreeNode* newNode = new HuffmanTreeNode(character[i], frequency[i],"","");
        //push into priority_queue
        pq.push(newNode);
    }
    return pq;
}

//Function to build Huffman Tree using PQ
//when push into new node, label left edge as 1, right edge as 0.
HuffmanTreeNode* buildHuffmanTree(priority_queue<HuffmanTreeNode*, vector<HuffmanTreeNode*>, Compare> pq)
{
    //using while loop to generate the Tree
    while (pq.size()>1)
    {
        //extract node with least frequency or ASCII value if equal
        HuffmanTreeNode* left = pq.top();
        //remove that node from priority_queue
        pq.pop();

        //extract node with second least frequency or ASCII if equal
        HuffmanTreeNode* right = pq.top();
        //remove that node from priority_queue
        pq.pop();

        //declare internal node which the value is the sum of its child frequency. Label left node as "0",right node as "1"
        HuffmanTreeNode* i_node = new HuffmanTreeNode('\0', left->frequency+right->frequency,"0","1");
        //build new branch of tree
        i_node->left = left;
        i_node->right = right;

        //push internal node into our priority_queue
        pq.push(i_node);
    }
    return pq.top(); //return the tree
}

//helper function to traverse tree to print binary code and store value in arr
void traverse(HuffmanTreeNode* root, int arr[], int pos)
{
    //left traverse is 0, using recursion to achieve value
    if (root->left)
    {
        arr[pos] = 0;
        traverse(root->left, arr, pos+1);
    }
    
    //right traverse is 1, using recursion to achieve value
    if (root->right)
    {
        arr[pos]=1;
        traverse(root->right,arr, pos+1);
    }
    
    //print chararacter and its code when we reach leaf node
    if (!root->left && !root->right)
        {
            //print binary code
            std::cout<<"Symbol: "<<root->character<<", Frequency: "<<root->frequency<<", Code: ";
            for (int i=0; i<pos; i++)
            {
                cout<<arr[i];
            }
            std::cout<<std::endl;
        }
}


//print result from generating HuffmanTree
void encode(HuffmanTreeNode* root)
{
    //traverse huffman tree and print result. Initiate empty array to store result
    int arr[100], position=0;
    traverse(root,arr,position);
}

//Define the thread arguments using to decompress the file
//Include root, binaryCode, positions, and char to decompress
struct arguments
{
    HuffmanTreeNode* root;
    string binaryCode;
    vector<int> positions;
    char* decompressedChars;
};

//Helper function to traverse the Huffman tree and determine the character
char getChar(HuffmanTreeNode* root, string binaryCode)
{
    HuffmanTreeNode* currentNode = root;
    for (char c : binaryCode) {
        if (c=='0') {
            currentNode = currentNode->left;
        } 
        else 
        {
            currentNode = currentNode->right;
        }
    }
    return currentNode->character;
}

//Thread function to decompress a symbol using void pointer arg
void* decompress(void* arg)
{
    arguments *args = (struct arguments*)arg;

    //Traverse the Huffman tree and get the character from the binary code
    char ch = getChar(args->root, args->binaryCode);

    //Store the decompressed character
    for (int pos : args->positions) {
        args->decompressedChars[pos] = ch;
    }
    pthread_exit(NULL);
}


//Driver code
int main()
{   //initilze empty list of character and frequency
    char character[100];
    int frequency[100];

    //Read input from filename.txt
    int i=0;
    int sum_freq=0;
    string filename;
    //input filename
    std::cin>>filename;
    ifstream infile(filename);
    if (!infile) {
        cerr << "Error: could not open file" << endl;
        return 1;
    }
    string line;
    while (getline(infile, line)) {
        character[i] = line[0];
        frequency[i] = stoi(line.substr(2));
        sum_freq+=frequency[i];
        i++;
    }
    infile.close(); //finish reading filename.
    int size=i; //size of the arrays is equal to the number of elements

    //initialize priority_queue
    priority_queue<HuffmanTreeNode*, vector<HuffmanTreeNode*>, Compare> pq;
    init_pq(character, frequency, size, pq);

    //buildHuffmanTree
    HuffmanTreeNode* root=buildHuffmanTree(pq);
    //Output the huffman tree
    encode(root);

    //read compressedfile
    vector<string> binaryCodes;
    vector<vector<int>> positions;
    string filename2;
    cin >> filename2;
    ifstream infile2(filename2);
    if (!infile2) {
        cerr << "Error: could not open file" << endl;
        return 1;
    }

    string line2;
    while (getline(infile2, line2)) {
        istringstream iss(line2);
        string binaryCode;
        iss >> binaryCode;

        vector<int> pos;
        int p;
        while (iss >> p) {
            pos.push_back(p);
        }
        binaryCodes.push_back(binaryCode);
        positions.push_back(pos);
    }
    infile2.close(); //finish reading compressed file
    
    // Create the thread arguments and POSIX threads
    int nthreads=size; //initialize n threads which size equal to number of line
    pthread_t *tid = new pthread_t[nthreads]; //create m thread id
    arguments *args=new arguments[nthreads]; //create m arguments thread
    char decompressedChars[sum_freq]; //initialize the message when we decompressed
    for (int i = 0; i < nthreads; i++) {
        //assign threads 
        args[i].root = root;
        args[i].binaryCode = binaryCodes[i];
        args[i].positions = positions[i];
        args[i].decompressedChars = decompressedChars;
        //Call pthread_create
        if (pthread_create(&tid[i], NULL, decompress, &args[i]))
        {
            fprintf(stderr, "Error creating thread\n");
			    return 1;
        }
    }

    // Wait for the threads to finish
    for (int i = 0; i < nthreads; i++) {
        pthread_join(tid[i], NULL);
    }

    // Print the original message
    cout<<"Original message: ";
    for (int i=0;i<sum_freq;i++) {
        cout<<decompressedChars[i];
    }
    cout << endl;
    return 0;
}