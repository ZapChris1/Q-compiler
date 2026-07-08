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


char constexpr BINDING_POWER_PARENTHESIS = 2;
char constexpr BINDING_POWER_BRACKET = 1;
char constexpr BINDING_POWER_DEREFRENCE_POINTER = -1;
char constexpr BINDING_POWER_AMPERSAND = -1;
char constexpr BINDING_POWER_MULTIPLICATION = -2;
char constexpr BINDING_POWER_DIVISION = -2;
char constexpr BINDING_POWER_ADDITION = -3;
char constexpr BINDING_POWER_SUBTRACTION = -3;
char constexpr BINDING_POWER_EQUALITY_SIGN = -31;
char constexpr BINDING_POWER_COMMA = -32;

struct token; //forward declaration

std::vector<token> tokenVector;

enum baseType {
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

enum type {
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
    colon,
    ifStatement,
    elseStatement,
    switchStatement,
    caseStatement,
    defaultStatement,
    gotoStatement,
    breakStatement,
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
    signedType,
    unsignedType,
    constType,
    volatileType,
    restrictType,
    staticType,
    equals,
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

enum syntax_type {
    null_type,
    translation_unit,
    function_call,
    function_arguments,
    function_declaration,
    function_definition,
    declaration_specifier, //the stuff before function name
    parameter_list,
    parameter_declaration,
    type_specifier,
    derefrence_operation,
    if_statement,
    else_statement,
    condition,
    switch_statement,
    case_statement,
    goto_statement,
    goto_destination,
    break_statement,
    declarator,
    multiplication_operation,
    division_operation,
    addition_operation,
    subtraction_operation,
    comma_seperator,
    array_size_specifier,
    grammar_statement,
    scope_block,
    single_line,
    single_line_with_scope,
    assignment,
    identifier
};

struct AST_NODE { //live on the heap(ptrs to them are in the branches vector) (will have to be freed during deconstruction of AST)
    AST_NODE* parent = nullptr;
    size_t index = 0; //defines its position on tokenVector //can be used to index with substring on an identifier
    size_t endIndex = 0; //used for () and etc (for position across multi-tokens/range)
    bool marked = false; //set to true for terminals
    std::string key;
    syntax_type type = null_type;
    void* value = nullptr;
    std::vector<AST_NODE*> branches;
};

void recursive_parse(AST_NODE* inputNode); //prototype

struct token {
    void* value = nullptr; //malloced, this will have to be freed during deconstruction of the AST 
    AST_NODE* parent = nullptr;
    AST_NODE* node = nullptr;
    type Type = type::null;
    size_t size = 0; //this is the size of the data at value (allows for value to be variable length)
    char priority = -1;
};

struct bracket_information {
    size_t index = 0; //NEEDS to be size_t in order to not be undefined behavior when i cast later
    type Type = type::null; 
};

struct symbol_data {
    bool exists = true;
    std::string name; //also the key
    AST_NODE* destinationNode = nullptr; //used for linking functions and labels 
    symbol_data* underlyingType = nullptr; //the int in int* x, also used for structs and return type of function ptrs
    std::vector<baseType> typeObject; //contains info abt the type (doesnt include the identifier or underlying type)
    std::vector<AST_NODE*> arraySizes; //ptr to the part of the AST where the expression for the size(s) is
};

class scope_stack : public std::stack<std::unordered_map<std::string, symbol_data>*> {
public:
    using std::stack<std::unordered_map<std::string, symbol_data>*>::c; //expose the container to allow traversing through the stack
    
    void clear() {
        while (!this->empty()) {
            this->pop();
        }
    }
};

class symbol_table_info { //struct for ease of use //all heap allocation is handled by the vector class
public:
    symbol_table_info() {
        createLocalScope();
        declareIdentifier("int");
        createLocalScope();
    }
    ~symbol_table_info() {
        symbolTableVector.clear();
        tableStack.clear();
        labelTableVector.clear();
        labelTableStack.clear();
    }
    //hardcode in the types for int, float, char, etc
    symbol_data* getIdentifier(std::string key) { //assumes it exists 
        std::cout << "<<<<<<<<<<<<<>>>>>>>>>>>>>" << key << std::endl;
        for (size_t i = 1; i <= tableStack.size(); i++) {
            std::unordered_map<std::string, symbol_data>* currentTable = tableStack.c[tableStack.size() - i];
            std::cout << "<><> " << tableStack.size() - i << " : " << (currentTable == &symbolTableVector[tableStack.size() - i]) << std::endl;
            if (currentTable->find(key) != currentTable->end()) {
                std::cout << i << std::endl;
                return &(tableStack.c[tableStack.size() - i]->at(key));
            }
        }
        std::cout << "Couldnt find identifier: " << key << std::endl;
        return nullptr;
    }
    symbol_data* getLabel(std::string key) {
        for (size_t i = 1; i <= labelTableStack.size(); i++) {
            std::unordered_map<std::string, symbol_data>* currentTable = labelTableStack.c[labelTableStack.size() - i];
            if (currentTable->find(key) != currentTable->end()) {
                return &(labelTableStack.c[labelTableStack.size() - i]->at(key));
            }
        }
        std::cout << "Couldnt find label: " << key << std::endl;
        return nullptr;
    }

    symbol_data* declareIdentifier(std::string key) {
        symbol_data newSymbol;
        newSymbol.name = key;
        if (tableStack.empty()) {
            std::cout << "empty table stack error" << std::endl;
            return nullptr;
        } else {
            if (tableStack.top()->find(key) == tableStack.top()->end()) {
                (*tableStack.top()).insert({ key, newSymbol });
                std::cout << "declared identifier :" << key << ": " << std::endl;
            }
            else {
                std::cout << "attempted to redeclare identifier in the same scope :" << key << ": " << std::endl;
                return nullptr;
            }
        }
        return &(*tableStack.top())[key];
    }

    symbol_data* declareIdentifier(std::string key, symbol_data& newSymbol) { //allows for the use of a premade structure
        newSymbol.name = key;
        if (tableStack.empty()) {
            std::cout << "empty table stack error" << std::endl;
            return nullptr;
        }
        else {
            if (tableStack.top()->find(key) == tableStack.top()->end()) {
                (*tableStack.top()).insert({ key, newSymbol });
                std::cout << "declared identifier :" << key << ": " << std::endl;
            }
            else {
                std::cout << "attempted to redeclare identifier in the same scope :" << key << ": " << std::endl;
                return nullptr;
            }
        }
        return &(*tableStack.top())[key];
    }

    symbol_data* declareLabel(std::string key) {        
        symbol_data newSymbol;
        newSymbol.name = key;
        if (labelTableStack.empty()) {
            std::cout << "empty label table stack error" << std::endl;
            return nullptr;
        }
        else {
            if (labelTableStack.top()->find(key) == labelTableStack.top()->end()) {
                (*labelTableStack.top()).insert({ key, newSymbol });
                std::cout << "declared label :" << key << ": " << std::endl;
            }
            else {
                std::cout << "attempted to redeclare label in the same scope :" << key << ": " << std::endl;
                return nullptr;
            }
        }
        return &(*labelTableStack.top())[key];
    }

    std::unordered_map<std::string, symbol_data>* createLocalScope() {
        std::unordered_map<std::string, symbol_data> newScope;
        symbolTableVector.push_back(newScope);
        tableStack.push(&(symbolTableVector.back()));
        return &newScope;
    }
    std::unordered_map<std::string, symbol_data>* createFunctionScope() {
        std::unordered_map<std::string, symbol_data> newScope;
        symbolTableVector.push_back(newScope);
        tableStack.push(&(symbolTableVector.back()));
        labelTableVector.push_back(newScope);
        labelTableStack.push(&(labelTableVector.back()));
        return &newScope;
    }

    void exitScope() {
        if (tableStack.top() == labelTableStack.top()) {
            labelTableStack.pop();
        }
        tableStack.pop();
    }

    std::string generateUnnamedKey(void) {
        std::string uniqueKey;
        if (lastUniqueKey != nullptr) {
            uniqueKey = *lastUniqueKey;
            if (lastUniqueKey->length() == 1) {
                if (static_cast<signed char>((*lastUniqueKey)[0]) < (signed char)("9")) {
                    (*lastUniqueKey)[0] = (*lastUniqueKey)[0] + 1;
                }
                else {
                    lastUniqueKey->push_back(SCHAR_MIN);
                }
            }
            else {
                if (static_cast<signed char>(lastUniqueKey->back()) < SCHAR_MAX) {
                    lastUniqueKey->back() = lastUniqueKey->back() + 1;
                }
                else {
                    lastUniqueKey->push_back(SCHAR_MIN);
                }
            }
        }
        else {
            lastUniqueKey = new std::string("0");
        }
        std::cout << "key generated" << *lastUniqueKey << std::endl;
        return *lastUniqueKey;
    }

    //public for direct access (for example when grabbing a ref to the top of a stack)
    std::deque<std::unordered_map<std::string, symbol_data>> symbolTableVector; //where the tables are actually stored in memory
    std::deque<std::unordered_map<std::string, symbol_data>> labelTableVector; //same but for symbol tables dedicated to labels(labelTables)
    scope_stack tableStack; //the keys are identifier names //stack contains ptrs to tables (its an organizational tool not where the data is held)
    scope_stack labelTableStack; //same but for labelTables
    std::stack<AST_NODE*> switchStack;
    std::deque<symbol_data*> unnamedKeys; //for memory management purposes

private:
    std::string* lastUniqueKey = nullptr; //used for generating unique keys
};

symbol_table_info* symbolTableInfo = new symbol_table_info;

template<typename T>
void setValue(token& currentNode, size_t startOfSymbol, const T& value) { //allocates the value for a token //dont call on a null type
    currentNode.value = std::malloc(sizeof(T));
    if (currentNode.value == 0) {
        std::cout << "failed to set value for: " << currentNode.Type << " to " << value << std::endl;
        return;
    }
    else {
        std::cout << "set value for: " << currentNode.Type << " to " << value << std::endl;
        std::memcpy(currentNode.value, &value, sizeof(T)); //directly setting the memory
        currentNode.size = sizeof(T);
    }
}
template<typename T>
void setValue(token& currentNode, size_t startOfSymbol, const T* ptr, const size_t length) { //allocates an array for a token //inputs are POINTERS to array and amount of elements
    currentNode.value = std::malloc(sizeof(T) * length);
    
    if (currentNode.value == 0) {
        std::cout << "failed to set value for: " << currentNode.Type << " to " << *ptr << std::endl;
        return;
    }
    else {
        std::cout << "set value for: " << currentNode.Type << " to " << *ptr << std::endl;
        std::memcpy(currentNode.value, ptr, sizeof(T) * length); //directly setting the memory
        currentNode.size = sizeof(T) * length;
    }
}


size_t lineCounter = 0;
void printNode(AST_NODE* node) {
    std::cout << "\n" << std::endl;
    for (unsigned int i = 0; i < lineCounter; i++) std::cout << "\t";
    if (node->endIndex != 0) std::cout << "syntax_type: " << node->type << " children: " << node->branches.size() << " index: " << node->index << " end index: " << node->endIndex << std::endl;
    else std::cout << "syntax_type: " << node->type << " children: " << node->branches.size() << " index: " << node->index << std::endl;
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

void orderBrackets(std::vector<bracket_information>& bracketVector) {
    unsigned int bracketVectorIndex = 0;
    while (bracketVector.size() != 0) {
        unsigned int bracketsBetween = 0;
        for (size_t closeBracketIndex = 1; closeBracketIndex < bracketVector.size(); closeBracketIndex++) {
            if (bracketVector[closeBracketIndex].Type - 1 == bracketVector[0].Type) {
                if (bracketsBetween > 0) {
                    bracketsBetween--;
                } else {
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

AST_NODE* parse_token(size_t inputIndex, AST_NODE* lastNode, bool nud); //protoype

AST_NODE* pratt_parse_node(AST_NODE* lastNode, size_t inputNodeIndex, bool nud) {
    size_t rightNodeIndex = inputNodeIndex + 1;
    token* rightToken = &tokenVector[rightNodeIndex];
    token* lastToken = &tokenVector[lastNode->index];
    if (lastToken->priority >= rightToken->priority) {
        return parse_token(inputNodeIndex, lastNode, nud); //return parsed self
    } else { //if rightPriority is greater //add a while loop to continue this recursion untill the right priority is no longer higher aka the right tree is complete
        AST_NODE* selfNode = parse_token(inputNodeIndex, lastNode, nud);
        AST_NODE* rightData = nullptr;
        if (selfNode->endIndex < rightNodeIndex) rightData = pratt_parse_node(selfNode, rightNodeIndex, false);
        else rightData = selfNode;
        while (lastToken->priority < tokenVector[rightData->endIndex + 1].priority) { //if this still greater then continue building the rightNode tree
            rightData = pratt_parse_node(rightData, rightData->endIndex + 1, false);
        }
        return rightData;
    }
}

//parse expression and automatically child it to an input parentnode bracket
AST_NODE* parse_expression(AST_NODE* inputNode) { //assuming index is '(' and endindex is ')'
    AST_NODE* leftSide = nullptr;
    
    if (inputNode->index + 1 < inputNode->endIndex) {
        leftSide = pratt_parse_node(leftSide, inputNode->index + 1, true); //skip to the last parsed node    
        for (size_t index = leftSide->endIndex + 1; index < inputNode->endIndex; index++) {
            if (tokenVector[index].Type == eol) goto end;
            leftSide = pratt_parse_node(leftSide, index, false); //skip to the last parsed node
            index = leftSide->endIndex;
        }
    }
    end:
    if (leftSide != nullptr) inputNode->branches.push_back(leftSide);
    return leftSide; //returns top node of the expression
}
//parent node independent
AST_NODE* parse_expression(size_t start, size_t end) { //assuming start is '(' and end is ')'
    AST_NODE* leftSide = nullptr;
    if (start + 1 < end) {
        leftSide = pratt_parse_node(leftSide, start + 1, true); //skip to the last parsed node    
        for (size_t index = leftSide->endIndex + 1; index < end; index++) {
            if (tokenVector[index].Type == eol) return leftSide;
            leftSide = pratt_parse_node(leftSide, index, false); //skip to the last parsed node
            index = leftSide->endIndex;
        }
    }
    return leftSide; //returns top node of the expression //doesnt automatically add as a child
}

void parse_arguments(AST_NODE* inputNode) { //assuming index is '(' and endindex is ')'
    unsigned int bracketCounter = 0;
    size_t lastCommaIndex = inputNode->index + 1;
    for (size_t index = inputNode->index + 1; index < inputNode->endIndex; inputNode++) {
        token* currentToken = &tokenVector[index];
        switch (currentToken->Type) {
        case openBracket:
        case openCurlyBracket:
        case openParenthesis:
            bracketCounter++;
            break;
        case closeBracket:
        case closeCurlyBracket:
        case closeParenthesis:
            bracketCounter--;
            break;
        case comma:
            if (bracketCounter == 0) {
                AST_NODE* currentExpression = parse_expression(lastCommaIndex - 1, index);
                currentExpression->parent = inputNode;
                inputNode->branches.push_back(currentExpression);
                lastCommaIndex = index;
            }
            break;
        }
    }
    if (inputNode->endIndex - lastCommaIndex > 1) {
        AST_NODE* currentExpression = parse_expression(lastCommaIndex - 1, inputNode->endIndex);
        currentExpression->parent = inputNode;
        inputNode->branches.push_back(currentExpression);
    }
    else {
        std::cout << "Comma with nothing infront of it syntax error" << std::endl;
    }
}

AST_NODE* parse_token(size_t inputIndex, AST_NODE* lastNode, bool nud) {
    token* inputToken = &tokenVector[inputIndex];
    AST_NODE* inputNode = inputToken->node;
    if (inputNode) return inputNode; //if theres already a tree built no need to rebuild
    inputNode = new AST_NODE();
    inputNode->index = inputIndex;
    inputToken->node = inputNode;


    auto unary_prefix_operation = [inputNode, &inputIndex]() { //uses right hand side ie (symbol)token
        AST_NODE* nextNode = pratt_parse_node(inputNode, inputIndex + 1, true);
        nextNode->parent = inputNode;
        inputNode->branches.push_back(nextNode);
        inputNode->endIndex = nextNode->endIndex;
    };

    auto unary_postfix_operation = [inputNode, lastNode, &inputIndex]() { //uses left hand side ie token(symbol)
        lastNode->parent = inputNode;
        inputNode->branches.push_back(lastNode);
    };

    auto binary_operation = [inputNode, lastNode, &inputIndex, unary_prefix_operation, unary_postfix_operation]() {
        unary_prefix_operation();
        unary_postfix_operation();
    };
    
    switch (inputToken->Type) {
    case multiplication:
        if (nud) {
            unary_prefix_operation();
            inputNode->type = syntax_type::derefrence_operation;
            break;
        }
        inputNode->type = syntax_type::multiplication_operation;
        binary_operation();
        break;
    case division:
        inputNode->type = syntax_type::division_operation;
        binary_operation();
        break;
    case subtraction:
        inputNode->type = syntax_type::subtraction_operation;
        binary_operation();
        break;
    case addition:
        inputNode->type = syntax_type::addition_operation;
        binary_operation();
        break;
    case equals:
        inputNode->type = syntax_type::assignment;
        binary_operation();
        break;
    case comma:
        if (!nud) std::cout << "err comma cannot be led at: " << inputNode->index << std::endl;
        unary_postfix_operation();
        inputNode->type = syntax_type::comma_seperator;
        break;
    case openParenthesis:
        inputNode->endIndex = *(size_t*)((*inputToken).value);
        parse_expression(inputNode);
        break;
    case variable: {
        if (inputNode->index + 1 < tokenVector.size() and tokenVector[inputNode->index + 1].Type == openParenthesis) { //if it is a function
            inputNode->type = syntax_type::function_call;
            std::string key = std::string((char*)inputToken->value, inputToken->size);
            inputNode->key = key; //get key for symbol table

            token* nextToken = &tokenVector[inputNode->index + 1];
            size_t start = inputNode->index + 1;
            size_t end = *(size_t*)nextToken->value;
            inputNode->endIndex = end;

            AST_NODE* functionArgs = parse_expression(start, end);
            functionArgs->parent = inputNode;
            inputNode->branches.push_back(functionArgs);
            if (functionArgs->type == comma) functionArgs->type = syntax_type::function_arguments;
        } else {
            inputNode->type = syntax_type::identifier;
            std::string key = std::string((char*)inputToken->value, inputToken->size);
            inputNode->key = key; //get key for symbol table
            inputNode->endIndex = inputIndex;
        }
        break;
    }
    case eol:
        std::cout << "err: attempting to parse ; at: " << inputIndex << std::endl;
        delete inputNode;
        return nullptr;
    default:
        std::cout << "err: unknown token: " << inputToken->Type << "at: " << inputIndex << std::endl;
        inputNode->endIndex = inputIndex;
    }
    return inputNode;
}

void parse_block(AST_NODE* inputNode);


AST_NODE* parse_declaration_specifier(size_t position, size_t endIndex) { //index is the first index to check and endIndex is where to stop
    std::cout << "parsing declaration specifiers!" << std::endl;
    AST_NODE* declarationSpecifierNode = new AST_NODE;
    declarationSpecifierNode->type = syntax_type::declaration_specifier;
    declarationSpecifierNode->index = position;

    std::string key = symbolTableInfo->generateUnnamedKey();
    declarationSpecifierNode->key = key;
    symbol_data declarationSpecifierData = *symbolTableInfo->declareIdentifier(key);
    symbol_data* underlyingType = nullptr;
    for (; position < endIndex; position++) {
        token* currentToken = &tokenVector[position];
        switch (currentToken->Type) {
        case variable:
            underlyingType = (symbolTableInfo->getIdentifier(std::string((char*)currentToken->value, currentToken->size))); goto end;
        case booleanType:
            underlyingType = (symbolTableInfo->getIdentifier("boolean")); goto end;
        case charType:
            underlyingType = (symbolTableInfo->getIdentifier("char")); goto end;
        case intType:
            underlyingType = (symbolTableInfo->getIdentifier("int")); goto end;
        case floatType:
            underlyingType = (symbolTableInfo->getIdentifier("float")); goto end;
        case doubleType:
            underlyingType = (symbolTableInfo->getIdentifier("double")); goto end;
        case shortType:
        case longType:
        case unsignedType:
        case signedType:
            if (tokenVector[position + 1].Type == variable or tokenVector[position + 1].Type == asterisk) {
                declarationSpecifierData.typeObject.push_back(tokenToBaseType(currentToken->Type));
                underlyingType = symbolTableInfo->getIdentifier("int"); 
                goto end;
            }
        default:
            declarationSpecifierData.typeObject.push_back(tokenToBaseType(currentToken->Type));
            break;
        }
    }
    end:
    if (underlyingType == nullptr) std::cout << "error: no underlying type found at: " << position << std::endl;
    declarationSpecifierData.underlyingType = underlyingType;
    declarationSpecifierNode->endIndex = position;
    return declarationSpecifierNode;
}

void parse_declaration_identifiers(size_t endIndex, AST_NODE* declaration_node) { //inParameters refers to whether this declaration is parameter declaration
    std::cout << "parsing declaration identifiers!: " << declaration_node->index << std::endl;
    bool typeObjectReady = false;
    size_t startIndex = declaration_node->endIndex + 1;
    size_t start = startIndex;

    auto parse_prefix_modifiers = [](AST_NODE* identifierNode, size_t inputPosition, size_t start, symbol_data& currentSymbol) {
        for (size_t position = inputPosition; position-- >= start;) { //prefix operators
            if (tokenVector[position].Type == asterisk) currentSymbol.typeObject.push_back(baseType::_pointer);
        }
    };
    
    auto parse_postfix_modifiers = [&parse_prefix_modifiers, &declaration_node, endIndex](AST_NODE* identifierNode, size_t start) {
                
        size_t identifierPosition = identifierNode->index;
        size_t prefixPosition = identifierPosition;

        symbol_data currentSymbol;      
        
        size_t position = identifierPosition + 1;
        for (; position < endIndex; position++) { //postfix operators
            if (tokenVector[position].Type == openBracket) {
                currentSymbol.typeObject.push_back(_array);
                AST_NODE* arraySizeExpression = new AST_NODE;
                arraySizeExpression->parent = identifierNode;
                arraySizeExpression->type = syntax_type::array_size_specifier;
                arraySizeExpression->index = position; //[ token
                arraySizeExpression->endIndex = *(size_t*)tokenVector[position].value; //] token
                position = arraySizeExpression->endIndex;
                recursive_parse(arraySizeExpression); //add logic for getting the size and etc of the array (for multi dim arrays)
                currentSymbol.arraySizes.push_back(arraySizeExpression);
            } else if (tokenVector[position].Type == closeParenthesis) {
                parse_prefix_modifiers(identifierNode, prefixPosition, *(size_t*)tokenVector[position].value, *currentSymbol);
                prefixPosition = position;
            } else if (tokenVector[position].Type == openParenthesis) {
                if (!declaration_node->branches.empty()) {
                    std::cout << "err, cannot have multiple declarations with a function declaraton at: " << position << std::endl;
                }
                identifierNode->index = declaration_node->index;
                currentSymbol.typeObject.push_back(_function);
                AST_NODE* functionParameters = new AST_NODE;
                functionParameters->parent = identifierNode;
                functionParameters->type = syntax_type::parameter_declaration; //output specfication for the function
                functionParameters->index = position; //start (
                functionParameters->endIndex = *(size_t*)tokenVector[functionParameters->index].value; //end )
                identifierNode->branches.push_back(functionParameters);
                recursive_parse(functionParameters);

                if (functionParameters->endIndex + 1 < tokenVector.size() and tokenVector[functionParameters->endIndex + 1].Type == openCurlyBracket) {
                    std::cout << "function definition" << std::endl;

                    identifierNode->type = syntax_type::function_definition;
                    parse_prefix_modifiers(identifierNode, prefixPosition, start, *currentSymbol);

                    currentSymbol.destinationNode = identifierNode;
                    AST_NODE* functionBody = new AST_NODE;

                    functionBody->parent = identifierNode;
                    functionBody->type = syntax_type::scope_block;
                    identifierNode->branches.push_back(functionBody);
                    functionBody->index = functionParameters->endIndex + 1; //start {
                    functionBody->endIndex = *(size_t*)tokenVector[functionBody->index].value; //end }
                    identifierNode->endIndex = functionBody->endIndex;
                    
                    functionBody->value = symbolTableInfo->createFunctionScope();
                    parse_block(functionBody);
                    symbolTableInfo->exitScope();
                    
                    position = identifierNode->endIndex;
                    
                    symbol_data* underlyingTypeData = symbolTableInfo->getIdentifier(declaration_node->key);
                    underlyingTypeData->typeObject.insert(underlyingTypeData->typeObject.end(), currentSymbol.typeObject.begin(), currentSymbol.typeObject.end());
                    
                    symbol_data* prototypeDeclaration = symbolTableInfo->getIdentifier(identifierNode->key);
                    if (prototypeDeclaration != nullptr) { //if there was a previous forwards declaration
                        if (!(prototypeDeclaration->underlyingType->typeObject == underlyingTypeData->typeObject and
                        prototypeDeclaration->underlyingType->underlyingType == underlyingTypeData->underlyingType)) {
                            std::cout << "err, discrepancy between return type of function definition and declaration at: " << position << std::endl;
                        }
                    }

                    return position;
                } else {
                    std::cout << "function declaration" << std::endl;
                    identifierNode->type = syntax_type::function_declaration;
                    identifierNode->endIndex = functionParameters->endIndex;
                    position = identifierNode->endIndex;
                }
            }
            else if (tokenVector[position].Type == comma) {
                parse_prefix_modifiers(identifierNode, prefixPosition, start, currentSymbol);
                currentSymbol.underlyingType = symbolTableInfo->getIdentifier(declaration_node->key);
                symbolTableInfo->declareIdentifier(identifierNode->key, currentSymbol);
                return position;
            }
            else if (tokenVector[position].Type == equals) {
                parse_prefix_modifiers(identifierNode, prefixPosition, start, currentSymbol);
                currentSymbol.underlyingType = symbolTableInfo->getIdentifier(declaration_node->key);
                symbolTableInfo->declareIdentifier(identifierNode->key, currentSymbol);
                return position;
            }
        }
        parse_prefix_modifiers(identifierNode, prefixPosition, start, currentSymbol);
        currentSymbol.underlyingType = symbolTableInfo->getIdentifier(declaration_node->key);
        symbolTableInfo->declareIdentifier(identifierNode->key, currentSymbol);
        return (size_t)0;
    };
    bool lastIsAssignment = false;
    for (size_t position = startIndex; position < endIndex; position++) { //change the equals to be per identifier
        token* currentToken = &tokenVector[position];
        switch (currentToken->Type) {
        case comma:
            if (lastIsAssignment) {
                if (declaration_node->branches.size() > 0) {
                    AST_NODE* identifierNode = declaration_node->branches.back();
                    AST_NODE* value = parse_expression(start - 1, position);
                    identifierNode->branches.push_back(value);
                    value->parent = identifierNode;
                    lastIsAssignment = false;
                }
                else {
                    std::cout << "err missing identifier at: " << position << std::endl;
                }
            }
            else {
                std::cout << "err missing identifier at: " << position << std::endl;
            }
            start = position + 1;
            break;
        case variable: {
            AST_NODE* identifierNode = new AST_NODE;
            token* identifierToken = &tokenVector[position];
            
            declaration_node->branches.push_back(identifierNode);
            identifierNode->parent = declaration_node;
            identifierNode->key = std::string((char*)identifierToken->value, identifierToken->size);
            identifierNode->index = position;
            size_t newPosition = parse_postfix_modifiers(identifierNode, start);
            std::cout << identifierNode->key << " : " << start << " : " << newPosition << std::endl;
            
            if (identifierNode->type == syntax_type::null_type) {
                identifierNode->type = syntax_type::identifier;
            }
            
            if (newPosition == 0) {
                declaration_node->endIndex = position;
                return;
            }
            else {
                identifierNode->endIndex = newPosition;
                
                position = newPosition;
                start = position + 1;
                
                if (tokenVector[newPosition].Type == equals) {
                    lastIsAssignment = true;
                }
            }
        }
            break;
        case equals: //supposed to be found in the postfix parser
            std::cout << "err missing identifier at: " << position << std::endl;
            break;
        }
    }
    if (declaration_node->branches.empty()) {
        std::cout << "err empty declaration at: " << declaration_node->index << std::endl;
    }
    else {
        declaration_node->endIndex = declaration_node->branches.back()->endIndex;
    }
}



size_t parse_parameter_declaration_identifier(size_t endIndex, AST_NODE* declarationSpecifierNode) { //declaration_node->endIndex should be the end of the underlying type
        
        auto parse_prefix_modifiers = [](AST_NODE* identifierNode, size_t inputPosition, size_t start, symbol_data& currentSymbol) {
            for (size_t position = inputPosition; --position > start - 1;) { //prefix operators
                if (tokenVector[position].Type == asterisk) currentSymbol.typeObject.push_back(baseType::_pointer);
            }
        };

        auto parse_postfix_modifiers = [&parse_prefix_modifiers, &declarationSpecifierNode, &endIndex](AST_NODE* identifierNode, size_t start) {
            size_t identifierPosition = identifierNode->index;
            size_t prefixPosition = identifierPosition;
            symbol_data currentSymbol = *symbolTableInfo->declareIdentifier(identifierNode->key);
            for (size_t position = identifierPosition + 1; position < endIndex; position++) { //postfix operators
                if (tokenVector[position].Type == openBracket) {
                    currentSymbol.typeObject.push_back(_array); //add logic for getting the size and etc of the array (for multi dim arrays)
                    AST_NODE* arraySizeExpression = new AST_NODE;
                    arraySizeExpression->parent = identifierNode;
                    arraySizeExpression->type = syntax_type::array_size_specifier;
                    arraySizeExpression->index = position; //[ token
                    arraySizeExpression->endIndex = *(size_t*)tokenVector[position].value; //] token
                    position = arraySizeExpression->endIndex;
                    recursive_parse(arraySizeExpression);
                    currentSymbol.arraySizes.push_back(arraySizeExpression);
                }
                else if (tokenVector[position].Type == closeParenthesis) {
                    parse_prefix_modifiers(identifierNode, prefixPosition, *(size_t*)tokenVector[position].value, currentSymbol);
                    prefixPosition = position;
                }
                else if (tokenVector[position].Type == comma) {
                    parse_prefix_modifiers(identifierNode, prefixPosition, start, currentSymbol);
                    return position;
                }
                else if (tokenVector[position].Type == equals) {
                    std::cout << "err parameter declaration cannot have initializations: " << position << std::endl;
                }
            }
            parse_prefix_modifiers(identifierNode, prefixPosition, start, currentSymbol);
            return (size_t)0;
        };
        bool lastIsAssignment = false;
        size_t start = declarationSpecifierNode->endIndex + 1;
        for (size_t position = start; position < endIndex; position++) { //change the equals to be per identifier
            token* currentToken = &tokenVector[position];
            switch (currentToken->Type) {
            case comma:
                return position;
            case variable: {
                AST_NODE* identifierNode = new AST_NODE;
                declarationSpecifierNode->branches.push_back(identifierNode);
                identifierNode->parent = declarationSpecifierNode;
                identifierNode->type = syntax_type::identifier;
                identifierNode->index = position;
                token* identifierToken = &tokenVector[position];
                identifierNode->key = std::string((char*)identifierToken->value, identifierToken->size);
                return parse_postfix_modifiers(identifierNode, start);
            }
            case equals: //supposed to be found in the postfix parser
                std::cout << "err parameter declaration cannot have initializations: " << position << std::endl;
                break;
            }
        }
        return endIndex;
    }

void parse_line(AST_NODE* parentNode, size_t firstIndex, size_t end) { //end is for bounds checking

    auto parse_as_declaration = [&parentNode, firstIndex, end]() {
        AST_NODE* declarationSpecifierNode = parse_declaration_specifier(firstIndex, end);
        declarationSpecifierNode->parent = parentNode;
        parentNode->branches.push_back(declarationSpecifierNode);
        parse_declaration_identifiers(end, declarationSpecifierNode);
    };

    auto parse_as_expression = [&parentNode, firstIndex, end]() {
        AST_NODE* topNode = parse_expression(firstIndex - 1, end);
        topNode->parent = parentNode;
        parentNode->branches.push_back(topNode);
    };

    token* firstToken = &tokenVector[firstIndex];
    switch (firstToken->Type) {
        case variable: {//fix identifier vs variable later
            token* secondToken = &tokenVector[firstIndex + 1];
            std::string key((char*)firstToken->value, firstToken->size);
            if (secondToken->Type == colon) {
                symbol_data* data = symbolTableInfo->declareLabel(key);
                
                AST_NODE* labelNode = new AST_NODE;
                labelNode->index = firstIndex;
                labelNode->key = key;
                labelNode->endIndex = firstIndex + 1;
                labelNode->type = syntax_type::goto_destination;
                labelNode->parent = parentNode;
                parentNode->branches.push_back(labelNode);

                AST_NODE* destinationNode = new AST_NODE;
                destinationNode->index = labelNode->endIndex + 1;
                destinationNode->parent = labelNode;
                destinationNode->type = syntax_type::grammar_statement;
                labelNode->branches.push_back(destinationNode);
                recursive_parse(destinationNode);

                data->destinationNode = labelNode;
            }
            else {
                symbol_data* data = symbolTableInfo->getIdentifier(key);
                if (data) {
                    if (data->typeObject.size() > 0) {
                        parse_as_expression();
                    }
                    else {
                        parse_as_declaration();
                    }
                } else {
                    std::cout << "attempted to parse unknown identifier:" << key << ": " << std::endl;
                }
            }
        }
        case booleanType:
        case charType:
        case intType:
        case floatType:
        case doubleType:
        case shortType:
        case longType:
        case unsignedType:
        case signedType: //include all the type qualifiers etc here
            parse_as_declaration();
            break;
        case type::defaultStatement:
        case type::caseStatement:
            //parse seperately
            break;
        case gotoStatement:
            //parse as goto
            break;
        default:
            parse_as_expression();
            break;
    }
}

void parse_block(AST_NODE* inputNode) { //lastStatement is the first index checked
    std::cout << "parsing block!" << std::endl;

    size_t lastStatement = inputNode->index + 1;
    
    auto parse_control_flow_statement = [&lastStatement, &inputNode](const syntax_type statementType, size_t& i) {
        AST_NODE* newNode = new AST_NODE();
        newNode->type = statementType;
        newNode->parent = inputNode;
        newNode->index = i;
        inputNode->branches.push_back(newNode);
        recursive_parse(newNode);
        lastStatement = i;
        i = newNode->endIndex;
        };

    for (size_t i = lastStatement; i < inputNode->endIndex; i++) { //endIndex is the }
        token* currentToken = &tokenVector[i];
        std::cout << "parsing token: " << i << "  with type: " << currentToken->Type << std::endl;
        switch (currentToken->Type) {
        case eol:
            if (i - lastStatement < 2) { //empty
                lastStatement = i;
            }
            else { //pure expression, use parse_expression()
                parse_line(inputNode, lastStatement, i);
                lastStatement = i;
            }
            break;
        case ifStatement:
            parse_control_flow_statement(syntax_type::if_statement, i);
            break;
        case switchStatement:
            parse_control_flow_statement(syntax_type::switch_statement, i);
            break;
        case breakStatement:
            parse_control_flow_statement(syntax_type::break_statement, i);
            break;
        case gotoStatement:
            parse_control_flow_statement(syntax_type::goto_statement, i);
            break;
        case caseStatement:
            parse_control_flow_statement(syntax_type::case_statement, i);
            break;
        case colon:
            if (i > lastStatement) {
                token* identifierToken = &tokenVector[i - 1];

                std::string key((char*)identifierToken->value, identifierToken->size);
                symbol_data data = *symbolTableInfo->getLabel(key);

                AST_NODE* labelNode = new AST_NODE;
                labelNode->index = i - 1;
                labelNode->key = key;
                labelNode->endIndex = i;
                labelNode->type = syntax_type::goto_destination;
                labelNode->parent = inputNode;
                inputNode->branches.push_back(labelNode);

                AST_NODE* destinationNode = new AST_NODE;
                destinationNode->index = labelNode->endIndex + 1;
                destinationNode->parent = labelNode;
                destinationNode->type = syntax_type::grammar_statement;
                labelNode->branches.push_back(destinationNode);
                recursive_parse(destinationNode);

                data.destinationNode = labelNode;
                i = destinationNode->endIndex;
            }
        case variable: {
            std::string key((char*)currentToken->value, currentToken->size);
            std::cout << "currentToken size: " << key << std::endl;
            symbol_data* data = symbolTableInfo->getIdentifier(key);
            std::cout << "currentToken size: " << key << std::endl;
            if (data) {
                std::cout << "parsing!:" << key << ": " << std::endl;
                if (data->typeObject.size() == 0) { //if the variable is a type
                    AST_NODE* declarationSpecifierNode = parse_declaration_specifier(i, inputNode->endIndex);
                    declarationSpecifierNode->parent = inputNode;
                    inputNode->branches.push_back(declarationSpecifierNode);
                    parse_declaration_identifiers(inputNode->endIndex, declarationSpecifierNode);
                    if (declarationSpecifierNode->parent == inputNode) { //if it was a variable skip to the end of the declaration
                        i = declarationSpecifierNode->endIndex;
                        if (i + 1 > tokenVector.size() and tokenVector[i + 1].Type == eol) {
                            i++;
                        }
                    }
                    else {
                        i = declarationSpecifierNode->branches[0]->endIndex; //if it was a function skip to the end of the function
                    }
                    lastStatement = i;
                }
            } else { //first time seeing data, must be a declaration
                std::cout << "attempted to parse unknown identifier:" << key << ": " << std::endl;
            }
            break;
            }
        }
    }
}

void recursive_parse(AST_NODE* inputNode) {
    size_t position = inputNode->index;
    switch (inputNode->type) {
    case parameter_declaration:
            for (size_t i = position + 1; i < inputNode->endIndex; i++) {
                AST_NODE* declarationSpecifierNode = parse_declaration_specifier(i, inputNode->endIndex);
                declarationSpecifierNode->parent = inputNode;
                inputNode->branches.push_back(declarationSpecifierNode);
                i = declarationSpecifierNode->endIndex + 1;
                if (i < inputNode->endIndex) {
                    i = parse_parameter_declaration_identifier(inputNode->endIndex, declarationSpecifierNode);
                }
                if (i == 0) break;
            }
            break;
        case condition:
        case array_size_specifier: //this will be parsed the same way expressions are
            parse_expression(inputNode);
            break;
        case switch_statement: {
            token* conditionToken = &tokenVector[position + 1];
            AST_NODE* conditionNode = new AST_NODE;
            conditionNode->type = syntax_type::condition;
            conditionNode->index = position + 1;
            conditionNode->endIndex = *(size_t*)conditionToken->value;
            conditionNode->parent = inputNode;
            parse_expression(conditionNode);
            inputNode->branches.push_back(conditionNode);

            AST_NODE* bodyNode = new AST_NODE;
            bodyNode->index = conditionNode->endIndex + 1;
            bodyNode->parent = inputNode;
            bodyNode->type = syntax_type::grammar_statement;
            inputNode->branches.push_back(bodyNode);
            recursive_parse(bodyNode);

            symbolTableInfo->switchStack.push(inputNode);
        }
            break;
        case if_statement: {
            token* conditionToken = &tokenVector[position + 1];
            AST_NODE* conditionNode = new AST_NODE;
            conditionNode->type = syntax_type::condition;
            conditionNode->index = position + 1;
            conditionNode->endIndex = *(size_t*)conditionToken->value;
            conditionNode->parent = inputNode;
            parse_expression(conditionNode);
            inputNode->branches.push_back(conditionNode);

            AST_NODE* bodyNode = new AST_NODE;
            bodyNode->index = conditionNode->endIndex + 1;
            bodyNode->parent = inputNode;
            bodyNode->type = syntax_type::grammar_statement;
            inputNode->branches.push_back(bodyNode);
            recursive_parse(bodyNode);

            token* elseToken = &tokenVector[bodyNode->endIndex + 1];
            if (elseToken->Type == elseStatement) {
                AST_NODE* elseNode = new AST_NODE;
                elseNode->index = bodyNode->endIndex + 1;
                elseNode->type = syntax_type::else_statement;
                elseNode->parent = inputNode;
                inputNode->branches.push_back(elseNode);
                recursive_parse(elseNode);
                inputNode->endIndex = elseNode->endIndex;
            }
            else {
                inputNode->endIndex = bodyNode->endIndex;
            }
        }
            break;
        case grammar_statement: {
            token* currentToken = &tokenVector[position];
            if (currentToken->Type == openBracket) {
                inputNode->endIndex = *(size_t*)currentToken->value;
                inputNode->type = syntax_type::scope_block;
                inputNode->value = symbolTableInfo->createLocalScope();
                parse_block(inputNode);
                symbolTableInfo->exitScope();
            }
            else { //single statement
                inputNode->endIndex = inputNode->parent->parent->endIndex;
                inputNode->type = syntax_type::single_line;
                parse_line(inputNode, inputNode->index, inputNode->parent->endIndex);
            }
            break;
        }
        case scope_block:
            parse_block(inputNode);
            symbolTableInfo->exitScope();
            //implement later
            break;
        case single_line:
            parse_line(inputNode, inputNode->index, inputNode->parent->endIndex);
            break;
        case goto_statement: {
            token* labelToken = &tokenVector[position + 1];
            inputNode->key = std::string((char*)labelToken->value, labelToken->size);
            inputNode->endIndex = position + 1;

            AST_NODE* bodyNode = new AST_NODE;
            bodyNode->index = inputNode->endIndex + 1;
            bodyNode->parent = inputNode;
            bodyNode->type = syntax_type::grammar_statement;
            inputNode->branches.push_back(bodyNode);
            recursive_parse(bodyNode);
        }
            break;
        case case_statement: {
            AST_NODE* switchNode = symbolTableInfo->switchStack.top();
            switchNode->branches.push_back(inputNode);
            inputNode->parent = switchNode;
            for (size_t i = position + 1; i < inputNode->parent->endIndex; i++) {
                if (tokenVector[i].Type == colon) {
                    inputNode->endIndex = i;
                    parse_expression(inputNode);
                    goto out;
                }
            }
            std::cout << "expected colon at: " << position << std::endl;
        out:
            AST_NODE* bodyNode = new AST_NODE;
            bodyNode->index = inputNode->endIndex;
            bodyNode->parent = inputNode;
            bodyNode->type = syntax_type::grammar_statement;
            inputNode->branches.push_back(bodyNode);
            recursive_parse(bodyNode);
        }
            break;
    }
}

//lexer logic

std::unordered_map<std::string, type> tokenKeywordTable;

void interpretSymbol(size_t startOfSymbol, size_t endOfSymbol, type currentType, std::string& source, std::vector<bracket_information>& bracketVector) {
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
            currentNode.priority = BINDING_POWER_MULTIPLICATION;
            break;
        }
        case addition:
        case subtraction: {
            currentNode.priority = BINDING_POWER_SUBTRACTION;
            break;
        }
        case comma: {
            currentNode.priority = BINDING_POWER_COMMA;
            break;
        }
        case openParenthesis:
            currentNode.priority = BINDING_POWER_PARENTHESIS;
            goto generic_bracket_logic;
        case openBracket: {
            currentNode.priority = BINDING_POWER_BRACKET;
            goto generic_bracket_logic;
        generic_bracket_logic:
            bracket_information currentBracket;
            currentBracket.Type = currentType;
            currentBracket.index = tokenVector.size();
            bracketVector.push_back(currentBracket);
            break;
        }
        case openCurlyBracket: {
            bracket_information currentBracket;
            currentBracket.Type = currentType;
            currentBracket.index = tokenVector.size();
            bracketVector.push_back(currentBracket);
            break;
        }
        case closeParenthesis:
        case closeBracket:
        case closeCurlyBracket: {
            bracket_information currentBracket;
            currentBracket.Type = currentType;
            currentBracket.index = tokenVector.size();
            bracketVector.push_back(currentBracket);
            break;
        }
        case variable:
            setValue(currentNode, startOfSymbol, source.c_str() + startOfSymbol, length); //sets the value 
            break;
    }
    tokenVector.push_back(currentNode);
}

void tokenize(std::string& source, std::vector<bracket_information>& bracketVector, std::unordered_map<std::string, type> tokenKeywordTable) { //fills the tokenVector
    size_t startOfLine = 0; //not sure if this will matter (possible removal)
    size_t startOfSymbol = 0;
    size_t endOfSymbol = 0;
    type currentType = null;
    type lastType = null;
    int currentNum = 0;
    for (size_t i = 0; i < source.length(); i++) {
        switch (source[i]) {
        case ';':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
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
                interpretSymbol(startOfSymbol, i, currentType, source, bracketVector);
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
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
            currentType = subtraction;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case '+': //add increment operator with variables in the future
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
            currentType = addition;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case '*': //add struct operator with variables in the future
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
            currentType = multiplication;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case '(':
            std::cout << ">>>>>>>>>>>OPEN PARENTHESIS FOUND<<<<<<<<<<<: " << i << std::endl;
            
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
            currentType = openParenthesis;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case ')':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
            currentType = closeParenthesis;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case '[':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
            currentType = openBracket;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case ']':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
            currentType = closeBracket;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case '{':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
            currentType = openCurlyBracket;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case '}':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
            currentType = closeCurlyBracket;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case ',':
            interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
            currentType = comma;
            startOfSymbol = i;
            endOfSymbol = i + 1;
            break;
        case '\n':
        case '\t':
        case '\v':
        case '\r':
        case '\f':
        case ' ':
            if (currentType != null) {
                interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
                currentType = null;
            }
            startOfSymbol = i + 1;
            break;
        default: //change this when variables are added
            if (currentType != variable) {
                interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
                startOfSymbol = i;
                endOfSymbol = i + 1;
                currentType = variable;
            }
            else {
                endOfSymbol = i + 1;
                try {
                    type keyword = tokenKeywordTable.at(source.substr(startOfSymbol, endOfSymbol - startOfSymbol));
                    std::cout << "interpretting" << std::endl;
                    interpretSymbol(startOfSymbol, endOfSymbol, keyword, source, bracketVector);
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
    if (currentType != null) interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
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
    tokenize(sourceString, bracketVector, tokenKeywordTable);
    orderBrackets(bracketVector);
    std::cout << "tokenVector.size() "  << tokenVector.size() << std::endl;
    
    AST_NODE* translation_unit = new AST_NODE;
    translation_unit->index = -1; //this is just set to -1 so that parse_block starts at 0
    translation_unit->endIndex = tokenVector.size();
    
    translation_unit->value = symbolTableInfo->tableStack.c.front();
    std::cout << "parse block" << std::endl;
    parse_block(translation_unit);

    
    printAST(translation_unit);
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
