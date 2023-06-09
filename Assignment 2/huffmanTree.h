// Program is retrieved from assignment 1.
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
    int counter; //initiate the counter to keep track with the order of the added node.
    HuffmanTreeNode* left;
    HuffmanTreeNode* right;
    string labelLeft;
    string labelRigth;
    
    //initialize the Node 
    HuffmanTreeNode (char ch, int freq, string labelLeft, string labelRigth, int nodeCounter)
    {
        character=ch;
        frequency=freq;
        left=right=NULL;
        labelRigth="";
        labelLeft="";
        counter=nodeCounter;
    }
};

//Implementation of Huffman Tree function class
//Modify compare class for PQ
class Compare{
public: 
    /*Arrange the symbols based on their frequencies. If two or more symbols have the same frequency, they will be sorted based on their ASCII value.
    Insert internal node into the queue of nodes as the lowest node based on its frequency.*/
    bool operator() (HuffmanTreeNode* first, HuffmanTreeNode* second)
    {
        if (first->frequency==second->frequency) 
        {
            if (first->character==second->character)
            {
                return first->counter < second->counter; //using counter to determine the order.
            }
            return (int)(first->character) > (int)(second->character); //compare ascii value if frequency is equal
        }
        return first->frequency > second->frequency; //compare frequency 
    }
};

//initialize priority_queue
priority_queue<HuffmanTreeNode*, vector<HuffmanTreeNode*>, Compare> init_pq(char character[], int frequency[], int size, priority_queue<HuffmanTreeNode*, vector<HuffmanTreeNode*>, Compare>& pq,  int& nodeCounter)
{
    //initialize priority_queue
    for (int i = 0; i < size; i++)
    {
        //initialize new HuffmanTree Node, increment counter each time it added in the code.
        HuffmanTreeNode* newNode = new HuffmanTreeNode(character[i], frequency[i],"","",nodeCounter++);
        //push into priority_queue
        pq.push(newNode);
    }
    return pq;
}

//Function to build Huffman Tree using PQ
//when push into new node, label left edge as 1, right edge as 0.
HuffmanTreeNode* buildHuffmanTree(priority_queue<HuffmanTreeNode*, vector<HuffmanTreeNode*>, Compare> pq, int& nodeCounter)
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
        //increment the counter when each node is create.
        HuffmanTreeNode* i_node = new HuffmanTreeNode('\0', left->frequency+right->frequency,"0","1", nodeCounter++);
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

//Helper function to traverse the Huffman tree and determine the character
char getChar(HuffmanTreeNode* root, string binaryCode)
{
    HuffmanTreeNode* currentNode = root;
    for (char c : binaryCode) {
        if (c=='0') {
            currentNode = currentNode->left; //travel left if the code is 0.
        } 
        else 
        {
            currentNode = currentNode->right; //travel right if the code is 1.
        }
    }
    return currentNode->character; //return character after traverse the binaryCode.
}