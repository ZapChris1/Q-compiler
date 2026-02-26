// Q compiler.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <queue>
#include <algorithm>

typedef enum type {
    unsignedIntLiteral,
    intLiteral,
    floatLiteral,
    doubleLiteral,
    longLiteral,
    subtraction,
    addition,
    multiplication,
    division,
    operation,
    variable,
    openParenthesis,
    closeParenthesis,
    openBracket,
    closeBracket,
    openCurlyBracket,
    closeCurlyBracket,
    eol, //end of line ';'
    null
};

struct queue_token {
    size_t indexer; //stores information about where to find the token in the tokenVector
    char priority = 0; //used to sort tokens in the priority_queue //priority should only be over zero for things that need it (multiplication, declaration, etc)
};

struct token; //forward declaration

struct AST_NODE { //live on the heap(ptrs to them are in the branches vector) (will have to be freed during deconstruction of AST)
    bool marked = false;
    token* value = nullptr;
    std::vector<AST_NODE*> branches;
};

struct token {
    void* value = nullptr; //malloced, this will have to be freed during deconstruction of the AST 
    AST_NODE* parent = nullptr;
    type Type = null;
    size_t size; //this is the size of the data at value (allows for value to be variable length)
};

struct bracket_information {
    size_t index;
    type Type = null;
};

template<typename T>
void setValue(token& currentNode, size_t startOfSymbol, const T& value) { //allocates the value for a token //dont call on a null type
    currentNode.value = std::malloc(sizeof(T));
    std::memcpy(currentNode.value, &value, sizeof(T)); //directly setting the memory
    currentNode.size = sizeof(T);
}

size_t lineCounter = 0;
void printNode(AST_NODE* node) {
    token* value = node->value;
    type valueType = value->Type;
    std::string valueName;
    switch (valueType) {
    case unsignedIntLiteral:
        valueName = "unsignedIntLiteral";
        break;
    case intLiteral:
        valueName = "intLiteral";
        break;
    case floatLiteral:
        valueName = "floatLiteral";
        break;
    case doubleLiteral:
        valueName = "doubleLiteral";
        break;
    case longLiteral:
        valueName = "longLiteral";
        break;
    case subtraction:
        valueName = "subtraction";
        break;
    case addition:
        valueName = "addition";
        break;
    case multiplication:
        valueName = "multiplication";
        break;
    case division:
        valueName = "division";
        break;
    case operation:
        valueName = "operation";
        break;
    case variable:
        valueName = "variable";
        break;
    case openParenthesis:
        valueName = "openParenthesis";
        break;
    case closeParenthesis:
        valueName = "closeParenthesis";
        break;
    case eol:
        valueName = "eol";
        break;
    case null:
        valueName = "null";
        break;
    }
    std::cout << "\n" << std::endl;
    for (unsigned int i = 0; i < lineCounter; i++) std::cout << "\t";
    std::cout << valueName;
}

void printAST(AST_NODE* currentNode) {
    currentNode->marked = true;
    printNode(currentNode);
    lineCounter++;
    for (unsigned int i = 0; i < currentNode->branches.size(); i++) {
        if (currentNode->branches[i] == nullptr) std::cout << "\ninvalid ptr" << std::endl;
        if (!currentNode->branches[i]->marked) {
            printAST(currentNode->branches[i]);
        }
    }
    lineCounter--;
    return;
}

auto compareTokens = [](const queue_token& tokenA, const queue_token& tokenB) -> bool {
    if (tokenA.priority == tokenB.priority) {
        return tokenA.indexer > tokenB.indexer;
    }
    else {
        return tokenA.priority < tokenB.priority;
    }
};

//finish tomorrow
void orderBrackets(std::vector<token>& tokenVector, std::vector<bracket_information>& bracketVector) {
    for (size_t frontBracketIndex = 0; frontBracketIndex < bracketVector.size(); frontBracketIndex++) {
        for (size_t backBracketIndex = bracketVector.size() - 1; backBracketIndex > 0; backBracketIndex--) {
            if (bracketVector[backBracketIndex].Type == bracketVector[frontBracketIndex].Type) {
                //set the values to the indexes of eachother
            }
        }
    }
    unsigned int bracketVectorIndex = 0;
    while (bracketVector.size() != 0) {
        for (size_t backBracketIndex = bracketVector.size() - 1; backBracketIndex > 0; backBracketIndex--) {
            if (bracketVector[backBracketIndex].Type == bracketVector[0].Type) {
                size_t temp = bracketVector[backBracketIndex].index;
                bracketVector[backBracketIndex].index = bracketVector[0].index;
                bracketVector[0].index = temp;
                bracketVector.erase(bracketVector.begin() + backBracketIndex);
                bracketVector.erase(bracketVector.begin());
                goto found_end_bracket;
            }
        }
        //if nothing was found there was an error
        std::cout << "error couldnt find end bracket for " << bracketVector[0].index << std::endl;
        break;
        found_end_bracket:
            bracketVectorIndex++;
            continue;
    }
}

using TokenQueue = std::priority_queue<queue_token, std::vector<queue_token>, decltype(compareTokens)>; //just a macro because the typeName is very long

AST_NODE* assembleAST(std::vector<token>& tokenVector, TokenQueue& tokenQueue) { //return the root token? //use the priority_queue to get WHICH tokens to start on and the vector should essentially be their info
    AST_NODE* currentNode = nullptr;
    token* currentToken = nullptr;
    auto naiveAddToken = [&currentNode, &currentToken](token& tokenToAdd, AST_NODE* nodeToAdd) { //adds a token without checking for its parent
        std::cout << "token added, type: " << tokenToAdd.Type << " branches: " << nodeToAdd->branches.size() << std::endl;
        currentNode->branches.push_back(nodeToAdd); //the value is cast to a AST_NODE 
        tokenToAdd.parent = currentNode; //set the parent refrence (already allocated)
    };
    auto addToken = [&currentNode, &currentToken, &tokenVector, naiveAddToken](size_t tokenIndex) { //gets the top parent of whatever token THEN adds it //had bounds checking
        if (tokenIndex >= tokenVector.size() or tokenIndex < 0) {
            std::cout << "Tried to add token outside of bounds, index: " << tokenIndex <<  std::endl;
            return;
        }
        std::cout << "adding token!:  ";
        token* tokenPtr = &tokenVector[tokenIndex];
        token* tokenPtr2 = nullptr;
        if (tokenPtr->parent == nullptr) {
            AST_NODE* tempNode = new AST_NODE;
            tempNode->value = tokenPtr;
            naiveAddToken(*tokenPtr, tempNode);
            return;
        } else while (tokenPtr->parent != nullptr) { //if the token is already in the tree structure, get the top of its tree
            tokenPtr2 = tokenPtr; //gets the 2nd to last in this tree (top of the tree instead of top of top of tree)
            tokenPtr = tokenPtr->parent->value;
        }
        naiveAddToken(*tokenPtr, tokenPtr2->parent);
    };
    
    size_t queueSize = tokenQueue.size();
    size_t i = 0;
    while (tokenQueue.size() > 0) {
        std::cout << "iteration: " << i << std::endl;
        i++; //for debug purposes only (remove i later)
        queue_token topOfQueue = tokenQueue.top();
        size_t vectorIndex = topOfQueue.indexer;
        tokenQueue.pop();
        currentNode = new AST_NODE; //in the future optimize by allocating room for [tokens.size()] nodes all at once then getting one from that 
        currentNode->value = &tokenVector[topOfQueue.indexer];
        currentToken = &tokenVector[topOfQueue.indexer];
        
        switch (currentToken->Type) {
        case subtraction:
            if (vectorIndex == 0) { //is unary
                addToken(vectorIndex + 1);
                break;
            } else {
                token* tokenPtr = &tokenVector[vectorIndex - 1];
                while (tokenPtr->parent != nullptr) { //loop to get the top token in tree
                    tokenPtr = tokenPtr->parent->value;
                }
                if (tokenPtr->Type == closeParenthesis or tokenPtr->Type == variable or tokenPtr->Type <= longLiteral) { //is binary subtraction symbol
                    addToken(vectorIndex - 1);
                    addToken(vectorIndex + 1);
                    break;
                } else { //is unary
                    addToken(vectorIndex + 1);
                    break;
                }
            }

        case addition:
        case multiplication:
            addToken(vectorIndex - 1);
            addToken(vectorIndex + 1);
            break;
        case eol: //remember to give this negative priority
            if (vectorIndex == 0) break;
            token* lastLine = nullptr;
            size_t locatorIndex = vectorIndex - 1;
            for (; locatorIndex != 0; locatorIndex--) {
                lastLine = &tokenVector[locatorIndex];
                if (lastLine->Type == eol) goto eol_found;
            }
            locatorIndex = -1;
            goto eol_found;

            eol_found: //add every token between the last line and current line to the branches of this line
            locatorIndex++;
            for (; locatorIndex < vectorIndex; locatorIndex++) {
                if (locatorIndex >= tokenVector.size() or locatorIndex < 0) {
                    std::cout << "Tried to add token outside of bounds, index: " << locatorIndex << std::endl;
                    continue;
                }
                token* tokenPtr = &tokenVector[locatorIndex];
                token* tokenPtr2 = nullptr;
                if (tokenPtr->parent == nullptr) {
                    if (currentNode->branches.size() == 0 or currentNode->branches.back()->value != tokenPtr) { //if the node isnt already in the vector
                        AST_NODE* tempNode = new AST_NODE;
                        tempNode->value = tokenPtr;
                        std::cout << "adding token!:  ";
                        naiveAddToken(*tokenPtr, tempNode);
                        continue;
                    }
                }
                else while (tokenPtr->parent != nullptr) { //if the token is already in the tree structure, get the top of its tree
                    tokenPtr2 = tokenPtr; //gets the 2nd to last in this tree (top of the tree instead of top of top of tree)
                    tokenPtr = tokenPtr->parent->value;
                }
                if (currentNode->branches.size() == 0 or currentNode->branches.back()->value != tokenPtr2) { //if the node isnt already in the vector
                    std::cout << "adding token!:  ";
                    naiveAddToken(*tokenPtr, tokenPtr2->parent);
                }
            }
            break;
        }
    }
    //we dont delete tokenQueue because the objects on it are deallocated when popped off and the variable for it is on the stack
    return currentNode;
}

void interpretSymbol(size_t startOfSymbol, size_t endOfSymbol, type currentType, std::vector<token>& tokenVector, TokenQueue& tokenQueue, std::string& source, std::vector<bracket_information>& bracketVector) {
    if (startOfSymbol >= endOfSymbol) return; //exit if there isnt a symbol (endOfSymbol is exclusive)
    size_t length = endOfSymbol - startOfSymbol;
    std::cout << "token: " << source.substr(startOfSymbol, length) << ": Start of Symbol " << startOfSymbol << " length " << length << " type: "  << currentType << std::endl;
    token currentNode;
    currentNode.Type = currentType;
    switch (currentType) {
        case unsignedIntLiteral: {
            unsigned int value = static_cast<unsigned int>(std::stoi(source.substr(startOfSymbol, length))); //casting to unsigned int
            setValue(currentNode, startOfSymbol, value);
            break;
        }
        case intLiteral: {
            int value = std::stoi(source.substr(startOfSymbol, length));
            setValue(currentNode, startOfSymbol, value);
            break;
        }
        case longLiteral: {
            long value = std::stol(source.substr(startOfSymbol, length));
            setValue(currentNode, startOfSymbol, value);
            break;
        }
        case floatLiteral: {
            float value = std::stof(source.substr(startOfSymbol, length));
            setValue(currentNode, startOfSymbol, value);
            break;
        }
        case doubleLiteral: {
            double value = std::stod(source.substr(startOfSymbol, length));
            setValue(currentNode, startOfSymbol, value);
            break;
        }
        case multiplication: {
            queue_token currentQueueNode;
            currentQueueNode.priority = 2;
            currentQueueNode.indexer = tokenVector.size(); //its size not size-1 because nothing has been pushed back to the tokenVector yet
            tokenQueue.push(currentQueueNode);
            break;
        }
        case addition:
        case subtraction: {
            queue_token currentQueueNode;
            currentQueueNode.priority = 1;
            currentQueueNode.indexer = tokenVector.size(); //its size not size-1 because nothing has been pushed back to the tokenVector yet
            tokenQueue.push(currentQueueNode);
            break;
        }
        case eol: {
            queue_token currentQueueNode;
            currentQueueNode.priority = -1;
            currentQueueNode.indexer = tokenVector.size(); //its size not size-1 because nothing has been pushed back to the tokenVector yet
            tokenQueue.push(currentQueueNode);
            break;
        }
        case openParenthesis:
        case closeParenthesis:
        case openBracket:
        case closeBracket:
        case openCurlyBracket:
        case closeCurlyBracket: {
            queue_token currentQueueNode;
            currentQueueNode.priority = 10; //decide later
            currentQueueNode.indexer = tokenVector.size(); //its size not size-1 because nothing has been pushed back to the tokenVector yet
            tokenQueue.push(currentQueueNode);
            break;
        }
    }
    tokenVector.push_back(currentNode);
}

void tokenize(std::vector<token>& tokenVector, std::priority_queue<queue_token, std::vector<queue_token>, decltype(compareTokens)>& tokenQueue, std::string& source, std::vector<bracket_information>& bracketVector) { //fills the tokenVector and tokenQueue
    unsigned int startOfLine = 0; //not sure if this will matter (possible removal)
    unsigned int startOfSymbol = 0;
    unsigned int endOfSymbol = 0;
    type currentType = null;
    int currentNum = 0;
    for (unsigned int i = 0; i < source.length(); i++) {
        switch (source[i]) {
        case ';':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
            startOfLine = i + 1; //not sure if this will matter (possible removal)
            startOfSymbol = i;
            endOfSymbol = i + 1;
            currentType = eol;
            break;
        case '0': //if its a number
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (!(currentType == unsignedIntLiteral or currentType == intLiteral or currentType == floatLiteral or currentType == doubleLiteral)) { //if its not already a number type
                interpretSymbol(startOfSymbol, i, currentType, tokenVector, tokenQueue, source, bracketVector);
                currentType = intLiteral;
                startOfSymbol = i;
            }
            endOfSymbol = i + 1;
            break;
        case '.': //add support for structs in the future
            if (i == 0) break;
            if ((currentType == unsignedIntLiteral or currentType == intLiteral) and (source[i - 1] == '0' or source[i - 1] == '1' or source[i - 1] == '2' or source[i - 1] == '3' or source[i - 1] == '4' or source[i - 1] == '5' or source[i - 1] == '6' or source[i - 1] == '7' or source[i - 1] == '8' or source[i - 1] == '9')) {
                currentType = floatLiteral;
                endOfSymbol = i + 1;
            }
            break;
        case '-': //add decrement operator with variables in the future
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
            currentType = subtraction;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case '+': //add increment operator with variables in the future
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
            currentType = addition;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case '*': //add struct operator with variables in the future
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
            currentType = multiplication;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case '(':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
            currentType = openParenthesis;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case ')':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
            currentType = closeParenthesis;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case '[':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
            currentType = openBracket;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case ']':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
            currentType = closeBracket;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case '{':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
            currentType = openCurlyBracket;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case '}':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
            currentType = closeCurlyBracket;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        default: //change this when variables are added
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
            startOfSymbol = i + 1;
            break;
        }
    }
    if (currentType != null) interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source);
}
void fillString(std::string& returnString, std::ifstream& inputFile) {
    returnString.reserve(inputFile.tellg());
    char buffer[4096];
    while (inputFile.read(buffer, sizeof(buffer))) {
        returnString.append(buffer);
    }
    returnString.append(buffer, inputFile.gcount());
}

int main() {
    std::ifstream inFile("inputFile.txt");
    if (!inFile.is_open()) std::cout << "Couldnt open file" << std::endl;
    //inFile.seekg(0);
    std::string sourceString;
    fillString(sourceString, inFile);
    std::cout << "----: " << sourceString << std::endl;
    std::vector<token> tokenVector;

    std::priority_queue<queue_token, std::vector<queue_token>, decltype(compareTokens)> tokenQueue(compareTokens);
    std::vector<bracket_information> bracketVector;
    tokenize(tokenVector, tokenQueue, sourceString, bracketVector);
    orderBrackets(tokenVector, bracketVector);
    std::cout << "tokenVector.size() "  << tokenVector.size() << std::endl;
    std::cout << "tokenQueue.size() " << tokenQueue.size() << std::endl;
    AST_NODE* rootNode = assembleAST(tokenVector, tokenQueue);
    std::cout << "rootNode type: " << rootNode->value->Type << std::endl;
    printAST(rootNode);
    std::cout << std::endl;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
