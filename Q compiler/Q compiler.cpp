// Q compiler.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <unordered_map>
#include <stack>

typedef enum baseType {
    _void,
    _Bool,
    _char,
    _short,
    _int,
    _long,
    _longLong,
    _float,
    _double,
    _longDouble,
    _pointer,
    _function,
    _array,
    _struct,
    _union,
    _enum
};

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
    closeParenthesis, //close must always be 1 ahead of open
    openBracket,
    closeBracket,
    openCurlyBracket,
    closeCurlyBracket,
    eol, //end of line ';'
    ifStatement,
    elseStatement,
    comma,
    address,
    doubleAddress,
    asterisk,
    pointer,
    voidType,
    booleanType,
    charType,
    shortType,
    intType,
    longType,
    longLongType,
    floatType,
    doubleType,
    longDoubleType,
    structType,
    unionType,
    enumType,
    null
};

inline baseType tokenToBaseType(type input) { //for getting base types in declarations ie the int in int x or int** x[] etc
    switch (input) {
        case voidType:
            return _void;
        case booleanType:
            return _Bool;
        case charType:
            return _char;
        case shortType:
            return _short;
        case intType:
            return _int;
        case longType:
            return _long;
        case longLongType:
            return _longLong;
        case floatType:
            return _float;
        case doubleType:
            return _double;
        case longDoubleType:
            return _longDouble;
        case structType:
            return _struct;
        case unionType:
            return _union;
        case enumType:
            return _enum;
        default:
            std::cout << "error getting the type, input: " << input << std::endl;
            return _void;
    }
}

typedef enum syntax_type {
    translation_unit,
    function_declaration, 
    declaration_specifier, //the stuff before function name
    parameter_list,
    parameter_declaration,
    type_specifier,
    derefrence_operation,
    declarator
};

struct queue_token {
    size_t indexer; //stores information about where to find the token in the tokenVector
    char priority = 0; //used to sort tokens in the priority_queue //priority should only be over zero for things that need it (multiplication, declaration, etc)
};

struct token; //forward declaration

struct AST_NODE { //live on the heap(ptrs to them are in the branches vector) (will have to be freed during deconstruction of AST)
    AST_NODE* parent;
    unsigned int index; //defines its position on tokenVector //can be used to index with substring on an identifier
    unsigned int endIndex; //used for () and etc (for position across multi-tokens/range)
    bool marked = false; //set to true for terminals
    std::string key;
    syntax_type type;
    token* value = nullptr;
    std::vector<AST_NODE*> branches;
};

struct token {
    void* value = nullptr; //malloced, this will have to be freed during deconstruction of the AST 
    AST_NODE* parent = nullptr;
    AST_NODE* node = nullptr;
    type Type = null;
    size_t size; //this is the size of the data at value (allows for value to be variable length)
    char priority = -1;
};

struct bracket_information {
    size_t index; //NEEDS to be size_t in order to not be undefined behavior when i cast later
    type Type = null; 
};

class symbol_table_info { //struct for ease of use //all heap allocation is handled by the vector class
public:
    symbol_table_info() {
        createLocalScope();
    }
    ~symbol_table_info() {
        symbolTableVector.clear();
        tableStack.empty();
        labelTableVector.clear();
        labelTableStack.empty();
    }

    symbol_data& getIdentifier(std::string key) { //assumes it exists 
        try {
            return tableStack.top()->at(key);
        } catch(std::out_of_range err) {
            std::cout << "Couldnt find identifier: " << key << std::endl;
        }
    }
    symbol_data& getLabel(std::string key) {
        try {
            return labelTableStack.top()->at(key);
        }
        catch (std::out_of_range err) {
            std::cout << "Couldnt find label: " << key << std::endl; //do error checking for labels after assembling parse tree (since calls can precede declarations)
            declareLabel(key);
            return labelTableStack.top()->at(key);
        }
    }

    void declareIdentifier(std::string key) {
        symbol_data newSymbol;
        newSymbol.name = key;
        (*tableStack.top())[key] = newSymbol;
    }

    void declareLabel(std::string key) {
        symbol_data newSymbol;
        newSymbol.name = key;
        (*labelTableStack.top())[key] = newSymbol;
    }

    void createLocalScope() {
        std::unordered_map<std::string, symbol_data> newScope;
        symbolTableVector.push_back(newScope);
        tableStack.push(&newScope);
    }
    void createFunctionScope() {
        std::unordered_map<std::string, symbol_data> newScope;
        symbolTableVector.push_back(newScope);
        tableStack.push(&newScope);
        labelTableVector.push_back(newScope);
        labelTableStack.push(&newScope);
    }

    //public for direct access (for example when grabbing a ref to the top of a stack)
    std::vector<std::unordered_map<std::string, symbol_data>> symbolTableVector; //where the tables are actually stored in memory
    std::vector<std::unordered_map<std::string, symbol_data>> labelTableVector; //same but for symbol tables dedicated to labels(labelTables)
    std::stack<std::unordered_map<std::string, symbol_data>*> tableStack; //the keys are identifier names //stack contains ptrs to tables (its an organizational tool not where the data is held)
    std::stack<std::unordered_map<std::string, symbol_data>*> labelTableStack; //same but for labelTables
};

struct symbol_data {
    bool exists = true;
    std::string name; //also the key
    symbol_data* underlyingType; //the int in int* x, also used for structs and return type of function ptrs
    std::vector<baseType> typeObject; //contains info abt the type (doesnt include the identifier or underlying type)
    std::vector<AST_NODE*> arraySizes; //ptr to the part of the AST where the expression for the size(s) is
};

template<typename T>
void setValue(token& currentNode, size_t startOfSymbol, const T& value) { //allocates the value for a token //dont call on a null type
    currentNode.value = std::malloc(sizeof(T));
    std::cout << "set value for: " << currentNode.Type << " to " << value << std::endl;
    std::memcpy(currentNode.value, &value, sizeof(T)); //directly setting the memory
    currentNode.size = sizeof(T);
}
template<typename T>
void setValue(token& currentNode, size_t startOfSymbol, const T* ptr, const size_t length) { //allocates an array for a token //inputs are POINTERS to array and amount of elements
    currentNode.value = std::malloc(sizeof(T) * length);
    std::cout << "set value for: " << currentNode.Type << " to " << *ptr << std::endl;
    std::memcpy(currentNode.value, ptr, sizeof(T) * length); //directly setting the memory
    currentNode.size = sizeof(T) * length;
}


size_t lineCounter = 0;
void printNode(AST_NODE* node) {
    token* value = node->value;
    type valueType = value->Type;
    std::string valueName;
    switch (valueType) {
    case unsignedIntLiteral: // <= longLiteral means all number literals so dont put anything between or before them
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
        valueName = "parenthesis";
        break;
    case closeParenthesis:
        valueName = "closeParenthesis";
        break;
    case openBracket:
        valueName = "bracket";
        break;
    case closeBracket:
        valueName = "closeBracket";
        break;
    case openCurlyBracket:
        valueName = "curlyBracket";
        break;
    case closeCurlyBracket:
        valueName = "closeCurlyBracket";
        break;
    case eol:
        valueName = "eol";
        break;
    case ifStatement:
        valueName = "ifStatement";
        break;
    case elseStatement:
        valueName = "elseStatement";
        break;
    case comma:
        valueName = "comma";
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

void orderBrackets(std::vector<token>& tokenVector, std::vector<bracket_information>& bracketVector) {
    unsigned int bracketVectorIndex = 0;
    while (bracketVector.size() != 0) {
        unsigned int bracketsBetween = 0;
        for (size_t closeBracketIndex = 1; closeBracketIndex < bracketVector.size(); closeBracketIndex++) {
            if (bracketVector[closeBracketIndex].Type - 1 == bracketVector[0].Type) {
                if (bracketsBetween > 0) {
                    bracketsBetween--;
                } else {
                    //setting the close bracket node to a child of the open bracket node before assembling AST (this helps preserve priority)
                    token* openBracketToken = &tokenVector[bracketVector[0].index];
                    AST_NODE* openBracketNode = new AST_NODE;
                    openBracketNode->value = openBracketToken;
                    openBracketToken->node = openBracketNode;

                    token* closeBracketToken = &tokenVector[bracketVector[closeBracketIndex].index];
                    AST_NODE* closeBracketNode = new AST_NODE;
                    closeBracketNode->value = closeBracketToken;
                    closeBracketToken->node = closeBracketNode;

                    closeBracketToken->parent = openBracketNode;
                    openBracketNode->branches.push_back(closeBracketNode);

                    setValue(*closeBracketToken, 0, bracketVector[0].index);
                    setValue(*openBracketToken, 0, bracketVector[closeBracketIndex].index);

                    bracketVector.erase(bracketVector.begin() + closeBracketIndex);
                    bracketVector.erase(bracketVector.begin());
                    goto found_end_bracket;
                }
            }
            else if (bracketVector[closeBracketIndex].Type == bracketVector[0].Type) bracketsBetween++;
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

inline size_t getPriority(const size_t inputTokenIndex, std::vector<token>& tokenVector) { //for parsing expressions
    token& inputToken = tokenVector[inputTokenIndex];
    switch (inputToken.Type) {
    case asterisk:
        if (inputTokenIndex != 0) {
            const token& prevToken = tokenVector[inputTokenIndex];
            switch (prevToken.Type) {
            case addition:
            case subtraction:
            case multiplication:
            case division:
            case openCurlyBracket:
            case openParenthesis:
            case openBracket:
            case modulus:
            case increment:
            case decrement:
            case equalSign:
            case greaterSign:
            case lessSign:
            case and_logical:
            case or_logical:
            case not_logical:
            case and_bitwise:
            case or_bitwise:
            case not_bitwise:
            case xor_bitwise:
            case left_shift_bitwise:
            case right_shift_bitwise:
            case address:
            case pointer:
            case question_mark:
            case comma:
                inputToken.priority += 2;
                inputToken.Type = pointer;
                return inputToken.priority;
            default:
                inputToken.Type = multiplication;
                return inputToken.priority;
            }
        }
        else {
            inputToken.priority += 2;
            inputToken.Type = pointer;
            return inputToken.priority;
        }

    default:
        return inputToken.priority;
    }
}

AST_NODE* parse_expression(AST_NODE* inputNode, std::vector<token>& tokenVector, symbol_table_info& symbolTableInfo) { //assuming index is '(' and endindex is ')'
    size_t index = inputNode->index + 1;
    AST_NODE* lastNode = nullptr;

    while (index < inputNode->endIndex) {
        AST_NODE* currentNode = getTop(index, tokenVector);
        token* currentToken = currentNode->value;
        if (currentToken->priority != 0) { //ignore non operations
            index = pratt_parse_node(lastNode, currentNode, tokenVector, symbolTableInfo); //skip to the last parsed node
        }
        index++;
    }
    if (lastNode != nullptr) inputNode->branches.push_back(lastNode);
    return lastNode;
}

size_t pratt_parse_node(AST_NODE* lastNode, AST_NODE* inputNode, std::vector<token>& tokenVector, symbol_table_info& symbolTableInfo) {
    token* currentToken = inputNode->value;
    if (currentToken->priority != 0) { //ignore non operations
        size_t&& nextNodeIndex = inputNode->index + 2;
        AST_NODE* nextNode = getTop(nextNodeIndex, tokenVector);
        if (currentToken->priority < tokenVector[nextNodeIndex].priority) {
            //parse the next node then parse this one (recursively so wait for next node to finish)
            size_t outputIndex = pratt_parse_node(inputNode, nextNode, tokenVector, symbolTableInfo);
            parse_operation(inputNode, lastNode, nextNode, tokenVector, symbolTableInfo); //pass the left and right and etc as refs
            return outputIndex;
        } else {
            parse_operation(inputNode, lastNode, nullptr, tokenVector, symbolTableInfo); //pass the left and right and etc as refs
            return inputNode->index;
        }
    }
}

void parse_operation(AST_NODE* inputNode, AST_NODE* lastNode, AST_NODE* nextNode, std::vector<token>& tokenVector, symbol_table_info& symbolTableInfo) {
    token* inputToken = inputNode->value;
    switch (inputToken->Type) {
    case multiplication:
    case division:
    case subtraction:
    case addition:
        if (lastNode == nullptr) {
            nextNode = getTop(inputNode->index - 1, tokenVector);
        }
        if (nextNode == nullptr) {
            nextNode = getTop(inputNode->index + 1, tokenVector);
        }
        lastNode->parent = inputNode;
        inputNode->branches.push_back(lastNode);
        inputNode->branches.push_back(nextNode);
        break;
    case pointer: //pointer DEREFRENCE operation
        if (nextNode == nullptr) {
            nextNode = getTop(inputNode->index + 1, tokenVector);
        }
        inputNode->branches.push_back(nextNode);
        inputNode->type = derefrence_operation;
        break;
    case openParenthesis:
        inputNode->endIndex = (size_t)((*inputToken).value);
        parse_expression(inputNode, tokenVector, symbolTableInfo)
        break;
    }
}

AST_NODE* getTop(size_t inputTokenIndex, std::vector<token>& tokenVector) {
    token& inputToken = tokenVector.at(inputTokenIndex);
    if (inputToken.parent == nullptr) {
        AST_NODE* newParent = new AST_NODE();
        newParent->value = &inputToken;
        newParent->index = inputTokenIndex;
        return newParent; //no parent
    }
    AST_NODE* topNode = inputToken.parent;
    while (topNode != nullptr) {
        topNode = topNode->parent;
    }
    return topNode;
}


void recursive_parse(AST_NODE* inputNode, std::vector<token>& tokenVector, symbol_table_info& symbolTableInfo) {
    unsigned int position = inputNode->index;
    switch (inputNode->type) {
        case parameter_list:
            bool foundComma = false;
            unsigned int start = position + 1; //first token after (
            unsigned int end = position + 1;
            while (end < inputNode->endIndex) {
                if (tokenVector[end].Type == comma) { //make a new node for this parameter
                    foundComma = true;
                    AST_NODE* newNode = new AST_NODE;
                    newNode->parent = inputNode;
                    newNode->type = parameter_declaration; //output specfication for the function
                    newNode->index = start;
                    newNode->endIndex = end - 1; //token before the ,
                    recursive_parse(newNode, tokenVector, symbolTableInfo);
                    inputNode->branches.push_back(newNode);
                    start = end + 1;
                }
                end++;
            }
            if (start == position + 1) { //if there was only one argument
                AST_NODE* newNode = new AST_NODE;
                newNode->parent = inputNode;
                newNode->type = parameter_declaration; //output specfication for the function
                newNode->index = start;
                newNode->endIndex = inputNode->endIndex - 1;
                recursive_parse(newNode, tokenVector, symbolTableInfo);
                inputNode->branches.push_back(newNode);
            }
            break;
        case parenthesized_parameter: //change so that the identifier node is not remade when using parenthized parameter
        case parameter_declaration: //we are going to go to the identifier THEN check from its modifiers from there //currently assuming this HAS an identifier //redesign identifier modifier grouping *,(),[] to work with new type system
            token firstToken = tokenVector[position];
            size_t insertionLocation = 0; //position of the token to insert after
            bool is_abstractDeclarator = true;

            
            auto prefixCheck = [&tokenVector, &symbolTableInfo](size_t& prefixCounter, symbol_data& currentSymbol, AST_NODE* identifierNode, bool is_functionPtr) { //return true to run again
                prefixCounter--;
                token behindIdentifier = tokenVector[prefixCounter];
                if (behindIdentifier.Type == asterisk) { //* is the only prefix operator in function parameter declarations
                    currentSymbol.typeObject.push_back(_pointer); //arrays decay to ptr for declarationSpecifier
                    return true;
                }
                return false;
                };
            bool first_bracket = true;
            auto postfixCheck = [&tokenVector, &symbolTableInfo, &prefixCheck](size_t& prefixCounter, size_t& postfixCounter, symbol_data& currentSymbol, AST_NODE* identifierNode, bool& is_functionPtr, bool& first_bracket) { //return true to run again
                postfixCounter++;
                token frontOfIdentifier = tokenVector[postfixCounter];
                if (frontOfIdentifier.Type == openBracket) { //no parsing is done because the size is not stored for declarationSpecifier
                    if (first_bracket) {
                        currentSymbol.typeObject.push_back(_pointer); //arrays decay to ptr for declarationSpecifier
                        first_bracket = false;
                    }
                    else {
                        currentSymbol.typeObject.push_back(_array); //add logic for getting the size and etc of the array (for multi dim arrays)
                        AST_NODE* arraySizeExpression = new AST_NODE;
                        arraySizeExpression->parent = identifierNode;
                        arraySizeExpression->type = array_size_specifier;
                        arraySizeExpression->index = postfixCounter; //[ token
                        arraySizeExpression->endIndex = (size_t)frontOfIdentifier.value; //] token
                        recursive_parse(arraySizeExpression, tokenVector, symbolTableInfo);
                        currentSymbol.arraySizes.push_back(arraySizeExpression);
                    }
                    postfixCounter = (size_t)(frontOfIdentifier.value); //skip to outside of the bracket
                    return true;
                }
                else if (frontOfIdentifier.Type == closeParenthesis) {
                    while (prefixCheck(prefixCounter, currentSymbol, identifierNode, is_functionPtr)); //run on repeat untill it returns false
                    return true;
                }
                else if (frontOfIdentifier.Type == openParenthesis) { //this will decay to a pointer if it is not already since function args decay to pointer to function //add edgecase for multiple parameter_lists
                    is_functionPtr = true;
                    AST_NODE* parameterNode = new AST_NODE; //add more function ptr support 
                    parameterNode->parent = identifierNode;
                    parameterNode->type = parameter_list;
                    parameterNode->index = postfixCounter;
                    parameterNode->endIndex = (size_t)(frontOfIdentifier.value); //location of )
                    recursive_parse(parameterNode, tokenVector, symbolTableInfo);
                    identifierNode->branches[1] = parameterNode; //the declaration specifier is the return type, the first child is branches
                    postfixCounter = (size_t)(frontOfIdentifier.value);
                    return false; //exit early if a function ptr is found because parsing is different for function ptrs
                }
                return false;
            };
            
            //insert loop here |V| (is meant to process tokens left to right)
            switch (firstToken.Type) { //this first run assumes a declarator with a identifier
                case typeDescriptor: //expand this to include int, void, etc //this is the declarationSpecifier
                    AST_NODE* declarationSpecifierNode = new AST_NODE;
                    declarationSpecifierNode->type = declarationSpecifier;
                    declarationSpecifierNode->parent = inputNode;
                    declarationSpecifierNode->value = &firstToken;
                    inputNode->branches.push_back(declarationSpecifierNode);


                    break;
                case asterisk: //used for finding where to insert the 'placeholder identifier' for abstract declarators
                    insertionLocation = position; //change to be the position in the loop <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    break;
                case openBracket:
                    if (!insertionLocation) {
                        insertionLocation = position - 2;
                    }
                    break;
                case identifier: //this is the declarator
                    is_abstractDeclarator = false;
                    AST_NODE* identifierNode = new AST_NODE;
                    identifierNode->type = identifier;
                    std::string key = std::string((char*)firstToken.value, firstToken.size);
                    identifierNode->key = key; //get key for symbol table
                    symbol_data currentSymbol = symbolTableInfo.getIdentifier(key); //if it doesnt already exist its made                    
                    //insert identifier logic for symbol table
                    size_t postfixCounter = position; //use position of identifier <<<<<<<<<<<<<<<
                    size_t prefixCounter = position; //use position of identifier <<<<<<<<<<<<<<<

                    bool is_functionPtr = false;

                    bool first_bracket = true;

                    while (postfixCheck(prefixCounter, postfixCounter, currentSymbol, identifierNode, is_functionPtr, first_bracket));
                    while (prefixCheck(prefixCounter, currentSymbol, identifierNode, is_functionPtr));
                    identifierNode->parent = inputNode;
                    inputNode->branches.push_back(identifierNode);
                    
                    token* declarationSpecifierToken = inputNode->branches[0]->value;
                    
                    std::string declarationSpecifierKey(static_cast<char*>(declarationSpecifierToken->value), declarationSpecifierToken->size);
                    symbol_data metadata = symbolTableInfo.getIdentifier(key);
                    currentSymbol.underlyingType = &metadata;

                    if (is_functionPtr) {

                    } else {
                        if (declarationSpecifierToken->Type == variable) { //for unions/enums/structs aka any 'custom' type
                            
                        }
                        else {
                            currentSymbol.typeObject[0] = tokenToBaseType(inputNode->branches[0]->value->Type);
                        }
                    }
                    break;
            }
            //end loop here |^|
            if (is_abstractDeclarator) {
                bool first_bracket = true;
                bool is_functionPtr = false;
                size_t postfixCounter = insertionLocation; 
                size_t prefixCounter = insertionLocation + 1; 
                while (postfixCheck(prefixCounter, postfixCounter, currentSymbol, identifierNode, is_functionPtr, first_bracket));
                while (prefixCheck(prefixCounter, currentSymbol, identifierNode, is_functionPtr));
            }

            break;
        case array_size_specifier: //this will be parsed the same way expressions are
            
            break;
    }
}

void assembleAST(std::vector<token>& tokenVector, AST_NODE& translation_unit, symbol_table_info& symbolTableInfo) { //call AFTER the translation unit has all of its children
    AST_NODE* lastNode;
    AST_NODE* currentNode;
    for (unsigned int i = 0; i < translation_unit.branches.size(); i++) {
        currentNode = translation_unit.branches[i];
        switch (currentNode->type) {
            case function_declaration: //the functions are found via their specifiers (they already have a child type_specifier
                AST_NODE* newNode = new AST_NODE;
                newNode->parent = currentNode;
                newNode->type = parameter_list; //output specfication for the function
                newNode->index = currentNode->index + 1; //start (
                newNode->endIndex = (unsigned int)tokenVector[currentNode->index + 1].value; //end )
                recursive_parse(newNode, tokenVector, symbolTableInfo);
                currentNode->branches.push_back(newNode);
        }
    }
}




AST_NODE* assembleAST(std::vector<token>& tokenVector, TokenQueue& tokenQueue) { //return the root token? //use the priority_queue to get WHICH tokens to start on and the vector should essentially be their info
    AST_NODE* lastNode = nullptr;
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
            if (tokenPtr->node == nullptr) {
                AST_NODE* tempNode = new AST_NODE;
                tempNode->value = tokenPtr;
                tokenPtr->node = tempNode;
                naiveAddToken(*tokenPtr, tempNode);
                return;
            }
            else {
                naiveAddToken(*tokenPtr, tokenPtr->node);
                return;
            }
        } else while (tokenPtr->parent != nullptr) { //if the token is already in the tree structure, get the top of its tree
            tokenPtr2 = tokenPtr; //gets the 2nd to last in this tree (top of the tree instead of top of top of tree)
            tokenPtr = tokenPtr->parent->value;
        }
        naiveAddToken(*tokenPtr, tokenPtr2->parent);
    };
    
    size_t queueSize = tokenQueue.size();
    size_t i = 0;
    while (tokenQueue.size() > 0) {
        queue_token topOfQueue = tokenQueue.top();
        size_t vectorIndex = topOfQueue.indexer;
        tokenQueue.pop();
        currentToken = &tokenVector[topOfQueue.indexer];
        if (currentToken->node != nullptr) {
            currentNode = currentToken->node;
        } else {
            currentNode = new AST_NODE; //in the future optimize by allocating room for [tokens.size()] nodes all at once then getting one from that 
            currentNode->value = currentToken;
            currentToken->node = currentNode;
        }
        std::cout << "iteration: " << i << " type: " << currentToken->Type << std::endl;
        i++; //for debug purposes only (remove i later)

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
        case eol: {
            if (vectorIndex == 0) break;
            token* lastLine = nullptr;
            size_t locatorIndex = vectorIndex - 1;
            for (; locatorIndex != 0; locatorIndex--) {
                lastLine = &tokenVector[locatorIndex];
                if (lastLine->Type == eol or lastLine->Type == openCurlyBracket or lastLine->Type == closeCurlyBracket) {
                    std::cout << "eol_found: " << locatorIndex << std::endl;
                    goto eol_found;
                }
            }
            lastLine = &tokenVector[locatorIndex];
            if (lastLine->Type == eol or lastLine->Type == openCurlyBracket or lastLine->Type == closeCurlyBracket) {
                std::cout << "eol_found: " << locatorIndex << std::endl;
                goto eol_found;
            }
            locatorIndex = -1;
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
                        if (tokenPtr->node == nullptr) {
                            AST_NODE* tempNode = new AST_NODE;
                            tempNode->value = tokenPtr;
                            naiveAddToken(*tokenPtr, tempNode);
                            break;
                        }
                        else {
                            naiveAddToken(*tokenPtr, tokenPtr->node);
                            break;
                        }
                    }
                } else while (tokenPtr->parent != nullptr) { //if the token is already in the tree structure, get the top of its tree
                    tokenPtr2 = tokenPtr; //gets the 2nd to last in this tree (top of the tree instead of top of top of tree)
                    tokenPtr = tokenPtr->parent->value;
                }
                if (currentNode->branches.size() == 0 or currentNode->branches.back()->value != tokenPtr2) { //if the node isnt already in the vector
                    std::cout << "adding token!:  ";
                    naiveAddToken(*tokenPtr, tokenPtr2->parent);
                    break;
                }
            }
            break;
        }
        case openParenthesis:
            if (vectorIndex < tokenVector.size()) {
                if (vectorIndex + 1 == *static_cast<size_t*>(currentToken->value)) {
                    std::cout << "empty bracket: " << vectorIndex << std::endl;
                }
                else {

                    addToken(vectorIndex + 1);
                }
            }
        case openCurlyBracket:
            for (size_t indexer = vectorIndex + 1; indexer < *static_cast<size_t*>(currentToken->value); indexer++) {
                if (tokenVector[indexer].parent == nullptr) {
                    addToken(indexer);
                }
            }
            break;
        case comma: //wrong issue with grouping elements; need new system for that
            if (vectorIndex != 0) {
                token* prevToken = currentToken - 1;
                if (prevToken->parent) {
                    AST_NODE* commaParent = prevToken->parent; //std::find compares memory addresses directly but this is alright for our purpose because there is only one location of our elements
                    std::vector<AST_NODE*>::iterator it = std::find(commaParent->branches.begin(), commaParent->branches.end(), prevToken->node);
                    if (it != commaParent->branches.end() or *it == prevToken->node) { //find the ptr pointing to the child (prevToken->node in commaParent->branches) and swap it with our currentNode
                        *it = currentNode;
                    }
                }
                addToken(vectorIndex - 1);
            }
            if (vectorIndex < tokenVector.size()) {
                if (vectorIndex < tokenVector.size() - 1 and (currentToken + 2)->Type == comma) {
                    addToken(vectorIndex + 2);
                }
                else {
                    addToken(vectorIndex + 1);
                }
            }
            break;
        case ifStatement: {
            addToken(vectorIndex + 1); //condition
            size_t indexer = vectorIndex;
            while (indexer < tokenVector.size() and tokenVector[indexer].parent == nullptr) {
                if (tokenVector[indexer].parent != nullptr) goto found_execution_node;
                else indexer++;
            }
            std::cout << "couldnt find token for if statement" << std::endl;
            break;
            found_execution_node:
            addToken(indexer); //code to execute
            indexer++;
            while (indexer < tokenVector.size() and tokenVector[indexer].parent == nullptr) {
                if (tokenVector[indexer].parent != nullptr) goto found_potential_else;
                else indexer++;
            }
            std::cout << "couldnt find token for if statement" << std::endl;
            break;
            found_potential_else:
            if (tokenVector[indexer].Type == elseStatement) {
                addToken(indexer);
            }
            addToken(indexer); //code to execute
            indexer++;
            //add logic for else/elseif later
            break;
        }
        case elseStatement:
            addToken(vectorIndex + 1);
            break;
        }
    }
    //we dont delete tokenQueue because the objects on it are deallocated when popped off and the variable for it is on the stack
    while (currentToken->parent != nullptr) {
        currentToken = currentToken->parent->value;
    }
    return currentToken->node;
}

std::unordered_map<std::string, type> tokenKeywordTable;

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
            currentNode.priority = 2;
            queue_token currentQueueNode;
            currentQueueNode.priority = 2;
            currentQueueNode.indexer = tokenVector.size(); //its size not size-1 because nothing has been pushed back to the tokenVector yet
            tokenQueue.push(currentQueueNode);
            break;
        }
        case addition:
        case subtraction: {
            currentNode.priority = 1;
            queue_token currentQueueNode;
            currentQueueNode.priority = 1;
            currentQueueNode.indexer = tokenVector.size(); //its size not size-1 because nothing has been pushed back to the tokenVector yet
            tokenQueue.push(currentQueueNode);
            break;
        }
        case comma: {
            queue_token currentQueueNode;
            currentQueueNode.priority = 1;
            currentQueueNode.indexer = tokenVector.size(); //its size not size-1 because nothing has been pushed back to the tokenVector yet
            tokenQueue.push(currentQueueNode);
            break;
        }
        case eol: {
            queue_token currentQueueNode;
            currentQueueNode.priority = -2;
            currentQueueNode.indexer = tokenVector.size(); //its size not size-1 because nothing has been pushed back to the tokenVector yet
            tokenQueue.push(currentQueueNode);
            break;
        }
        case ifStatement: {
            std::cout << "adding if statment" << std::endl;
            queue_token currentQueueNode;
            currentQueueNode.priority = -3;
            currentQueueNode.indexer = tokenVector.size(); //its size not size-1 because nothing has been pushed back to the tokenVector yet
            tokenQueue.push(currentQueueNode);
            break;
        }
        case openParenthesis:
        case openBracket: {
            currentNode.priority = 20;
            queue_token currentQueueNode;
            currentQueueNode.priority = -1;
            currentQueueNode.indexer = tokenVector.size(); //its size not size-1 because nothing has been pushed back to the tokenVector yet
            tokenQueue.push(currentQueueNode);
            bracket_information currentBracket;
            currentBracket.Type = currentType;
            currentBracket.index = currentQueueNode.indexer;
            bracketVector.push_back(currentBracket);
            break;
        }
        case openCurlyBracket: {
            queue_token currentQueueNode;
            currentQueueNode.priority = -3;
            currentQueueNode.indexer = tokenVector.size(); //its size not size-1 because nothing has been pushed back to the tokenVector yet
            tokenQueue.push(currentQueueNode);
            bracket_information currentBracket;
            currentBracket.Type = currentType;
            currentBracket.index = currentQueueNode.indexer;
            bracketVector.push_back(currentBracket);
            break;
        }
        case closeParenthesis:
        case closeBracket:
        case closeCurlyBracket: {
            queue_token currentQueueNode;
            currentQueueNode.indexer = tokenVector.size(); //its size not size-1 because nothing has been pushed back to the tokenVector yet
            bracket_information currentBracket;
            currentBracket.Type = currentType;
            currentBracket.index = currentQueueNode.indexer;
            bracketVector.push_back(currentBracket);
            break;
        }
        case variable:
            std::cout << "variable added" << std::endl;
            setValue(currentNode, startOfSymbol, source.c_str() + startOfSymbol, length); //sets the value 
            break;
    }
    tokenVector.push_back(currentNode);
}

void tokenize(std::vector<token>& tokenVector, std::priority_queue<queue_token, std::vector<queue_token>, decltype(compareTokens)>& tokenQueue, std::string& source, std::vector<bracket_information>& bracketVector, std::unordered_map<std::string, type> tokenKeywordTable) { //fills the tokenVector and tokenQueue
    unsigned int startOfLine = 0; //not sure if this will matter (possible removal)
    unsigned int startOfSymbol = 0;
    unsigned int endOfSymbol = 0;
    type currentType = null;
    type lastType = null;
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
        case ',':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
            currentType = comma;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case ' ':
            if (currentType != null) {
                interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
                currentType = null;
            }
            startOfSymbol = i + 1;
            break;
        default: //change this when variables are added
            if (currentType != variable) {
                interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
                startOfSymbol = i;
                endOfSymbol = i + 1;
                currentType = variable;
            }
            else {
                endOfSymbol = i + 1;
                try {
                    type keyword = tokenKeywordTable.at(source.substr(startOfSymbol, endOfSymbol - startOfSymbol));
                    std::cout << "interpretting" << std::endl;
                    interpretSymbol(startOfSymbol, endOfSymbol, keyword, tokenVector, tokenQueue, source, bracketVector);
                    currentType = keyword;
                    startOfSymbol = i + 1;
                }
                catch (std::out_of_range) {
                    //break;
                }
            }
            break;
        }
    }
    if (currentType != null) interpretSymbol(startOfSymbol, endOfSymbol, currentType, tokenVector, tokenQueue, source, bracketVector);
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

    std::unordered_map<std::string, type> tokenKeywordTable;
    tokenKeywordTable["void"] = voidType;
    tokenKeywordTable["bool"] = booleanType;
    tokenKeywordTable["char"] = charType;
    tokenKeywordTable["short"] = shortType;
    tokenKeywordTable["long"] = longType;
    tokenKeywordTable["float"] = floatType;
    tokenKeywordTable["double"] = doubleType;
    tokenKeywordTable["if"] = ifStatement;
    tokenKeywordTable["else"] = elseStatement;

    tokenize(tokenVector, tokenQueue, sourceString, bracketVector, tokenKeywordTable);
    orderBrackets(tokenVector, bracketVector);
    std::cout << "tokenVector.size() "  << tokenVector.size() << std::endl;
    std::cout << "tokenQueue.size() " << tokenQueue.size() << std::endl;

    symbol_table_info* symbolTableInfo = new symbol_table_info;

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
