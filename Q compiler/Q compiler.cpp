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
#include <cassert>

bool constexpr ALLOW_FUNCTION_DECLARATIONS_FOLLOWING_STRUCT_DEFINITIONS = false; //disable to adhere to standard, enabling allows struct foo {} func () {}; to create a function returning a foo obj

char constexpr BINDING_POWER_PARENTHESIS = 15; //( //led
char constexpr BINDING_POWER_BRACKET = 15; //[ //led
char constexpr BINDING_POWER_STRUCT_ACCESS = 15; //same for . and -> //led
char constexpr BINDING_POWER_INCREMENT_POSTFIX = 15; //++ //led
char constexpr BINDING_POWER_DECREMENT_POSTFIX = 15; //-- //led

char constexpr BINDING_POWER_INCREMENT_PREFIX = 14; //++ //nud
char constexpr BINDING_POWER_DECREMENT_PREFIX = 14; //-- //nud
char constexpr BINDING_POWER_UNARY_POSITIVE = 14; //+ //nud
char constexpr BINDING_POWER_UNARY_NEGATION = 14; //- //nud
char constexpr BINDING_POWER_LOGICAL_NOT = 14; //! //nud
char constexpr BINDING_POWER_BITWISE_NOT = 14; //~ //nud
char constexpr BINDING_POWER_DEREFRENCE_POINTER = 14; //* //nud
char constexpr BINDING_POWER_AMPERSAND = 14; //& //nud
char constexpr BINDING_POWER_SIZEOF = 14; //sizeof //nud
char constexpr BINDING_POWER_ALIGNOF = 14; //alignof //nud
char constexpr BINDING_POWER_TYPE_CAST = 14; //(type)x, priority is reset for the ( token after being identified as a typecast

char constexpr BINDING_POWER_MULTIPLICATION = 13; //* //led
char constexpr BINDING_POWER_DIVISION = 13; // / //led
char constexpr BINDING_POWER_MODULUS = 13; //% //led

char constexpr BINDING_POWER_BITWISE_SHIFT_LEFT = 11; //<< //led
char constexpr BINDING_POWER_BITWISE_SHIFT_RIGHT = 11; //>> //led

char constexpr BINDING_POWER_GREATER_THAN = 10; //> //led
char constexpr BINDING_POWER_LESS_THAN = 10; //< //led
char constexpr BINDING_POWER_GREATER_THAN_OR_EQUAL = 10; //>= //led
char constexpr BINDING_POWER_LESS_THAN_OR_EQUAL = 10; //<= //led

char constexpr BINDING_POWER_EQUALS_CHECK = 9; //== //led
char constexpr BINDING_POWER_NOT_EQUALS_CHECK = 9; //!= //led

char constexpr BINDING_POWER_BITWISE_AND = 8; //& //led

char constexpr BINDING_POWER_BITWISE_XOR = 7; //^ //led

char constexpr BINDING_POWER_BITWISE_OR = 6; //| //led

char constexpr BINDING_POWER_LOGICAL_AND = 5; //&&, and //led

char constexpr BINDING_POWER_LOGICAL_OR = 4; //||, or //led

char constexpr BINDING_POWER_CONDITIONAL = 3; //? //led //implement its parsing seperately

//led
char constexpr BINDING_POWER_ASSIGNMENT = 2; //=
char constexpr BINDING_POWER_ADDITION_ASSIGNMENT = 2; //+=
char constexpr BINDING_POWER_SUBTRACTION_ASSIGNMENT = 2; //-=
char constexpr BINDING_POWER_MULTIPLICATION_ASSIGNMENT = 2; //*=
char constexpr BINDING_POWER_MODULUS_ASSIGNMENT = 2; //%=
char constexpr BINDING_POWER_DIVISION_ASSIGNMENT = 2; // /=
char constexpr BINDING_POWER_BITWISE_SHIFT_LEFT_ASSIGNMENT = 2; //<<=
char constexpr BINDING_POWER_BITWISE_SHIFT_RIGHT_ASSIGNMENT = 2; //>>=
char constexpr BINDING_POWER_BITWISE_AND_ASSIGNMENT = 2; //&=
char constexpr BINDING_POWER_BITWISE_XOR_ASSIGNMENT = 2; //^=
char constexpr BINDING_POWER_BITWISE_OR_ASSIGNMENT = 2; //|=

char constexpr BINDING_POWER_COMMA = 1; //,

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
    _enum,
};

enum symbol_type {
    Void,
    Variable,
    Function,
    Label,
    Type //int, structs, classes etc go here
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
    modulus,
    shiftLeft,
    shiftRight,
    greaterThan,
    lessThan,
    greaterThanEqual,
    lessThanEqual,
    equalsComp,
    notEquals,
    xOr,
    bitwiseOr,
    And,
    Or,
    logicalNot,
    bitwiseNot,
    sizeOf,
    alignOf,
    conditional,
    increment,
    decrement,
    structAccess,
    structPointerAccess,
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
    forStatement,
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
    plusEquals,
    minusEquals,
    multEquals,
    modEquals,
    divEquals,
    shiftLeftEquals,
    shiftRightEquals,
    andEquals,
    xorEquals,
    orEquals,
    null,
};


char getPriority(type inputType, bool nud) {
    if (nud) {
        switch (inputType) { //nud
        case type::increment:
            return BINDING_POWER_INCREMENT_PREFIX;
        case type::decrement:
            return BINDING_POWER_DECREMENT_PREFIX;
        case type::addition:
            return BINDING_POWER_UNARY_POSITIVE;
        case type::subtraction:
            return BINDING_POWER_UNARY_NEGATION;
        case type::logicalNot:
            return BINDING_POWER_LOGICAL_NOT;
        case type::bitwiseNot:
            return BINDING_POWER_BITWISE_NOT;
        case type::multiplication:
            return BINDING_POWER_DEREFRENCE_POINTER;
        case type::address:
            return BINDING_POWER_AMPERSAND;
        case type::sizeOf:
            return BINDING_POWER_SIZEOF;
        case type::alignOf:
            return BINDING_POWER_ALIGNOF;
        case type::comma:
            return BINDING_POWER_COMMA;
        default:
            return 0;
        }
    }
    else {
        switch (inputType) { //led
        case type::openParenthesis: //function call
            return BINDING_POWER_PARENTHESIS;
        case type::openBracket: //indexing
            return BINDING_POWER_BRACKET;
        case type::increment:
            return BINDING_POWER_INCREMENT_POSTFIX;
        case type::decrement:
            return BINDING_POWER_DECREMENT_POSTFIX;
        case type::structAccess:
        case type::structPointerAccess:
            return BINDING_POWER_STRUCT_ACCESS;
        case type::multiplication:
            return BINDING_POWER_MULTIPLICATION;
        case type::division:
            return BINDING_POWER_DIVISION;
        case type::modulus:
            return BINDING_POWER_MODULUS;
        case type::shiftLeft:
            return BINDING_POWER_BITWISE_SHIFT_LEFT;
        case type::shiftRight:
            return BINDING_POWER_BITWISE_SHIFT_RIGHT;
        case type::greaterThan:
            return BINDING_POWER_GREATER_THAN;
        case type::lessThan:
            return BINDING_POWER_LESS_THAN;
        case type::greaterThanEqual:
            return BINDING_POWER_GREATER_THAN_OR_EQUAL;
        case type::lessThanEqual:
            return BINDING_POWER_GREATER_THAN_OR_EQUAL;
        case type::equalsComp:
            return BINDING_POWER_EQUALS_CHECK;
        case type::notEquals:
            return BINDING_POWER_NOT_EQUALS_CHECK;
        case type::address:
            return BINDING_POWER_BITWISE_AND;
        case type::xOr:
            return BINDING_POWER_BITWISE_XOR;
        case type::bitwiseOr:
            return BINDING_POWER_BITWISE_OR;
        case type::And:
            return BINDING_POWER_LOGICAL_AND;
        case type::Or:
            return BINDING_POWER_LOGICAL_OR;
        case type::conditional:
            return BINDING_POWER_CONDITIONAL;
        case type::equals:
            return BINDING_POWER_ASSIGNMENT;
        case type::plusEquals:
            return BINDING_POWER_ADDITION_ASSIGNMENT;
        case type::minusEquals:
            return BINDING_POWER_ADDITION_ASSIGNMENT;
        case type::multEquals:
            return BINDING_POWER_MULTIPLICATION_ASSIGNMENT;
        case type::modEquals:
            return BINDING_POWER_MODULUS_ASSIGNMENT;
        case type::divEquals:
            return BINDING_POWER_DIVISION_ASSIGNMENT;
        case type::shiftLeftEquals:
            return BINDING_POWER_BITWISE_SHIFT_LEFT_ASSIGNMENT;
        case type::shiftRightEquals:
            return BINDING_POWER_BITWISE_SHIFT_RIGHT_ASSIGNMENT;
        case type::andEquals:
            return BINDING_POWER_BITWISE_AND_ASSIGNMENT;
        case type::xorEquals:
            return BINDING_POWER_BITWISE_XOR_ASSIGNMENT;
        case type::orEquals:
            return BINDING_POWER_BITWISE_XOR_ASSIGNMENT;
        case type::comma:
            return BINDING_POWER_COMMA;
        default:
            return 0;
        }
    }
}


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
    parenthesized_expression,
    type_cast,
    parenthesized_type,
    compound_literal,
    compound_literal_parameters,
    index_operation,
    function_call,
    function_arguments,
    function_declaration,
    function_definition,
    declaration_specifier, //the stuff before function name
    parameter_list,
    parameter_declaration,
    type_specifier,
    derefrence_operation,
    conditional_operation,
    conditional_operator_false_case,
    for_loop,
    for_loop_parenthesis,
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
    member_access,
    pointer_member_access,
    modulus_operation,
    shift_left_operation,
    shift_right_operation,
    greater_than_comparison,
    less_than_comparison,
    equal_comparison,
    not_equal_comparison,
    less_than_equal_comparison,
    greater_than_equal_comparison,
    address_operation,
    bitwise_and_operation,
    bitwise_not_operation,
    bitwise_xor_operation,
    bitwise_or_operation,
    logical_or_operation,
    logical_and_operation,
    logical_not_operation,
    size_of_operation,
    align_of_operation,
    unary_positive,
    unary_negation,
    prefix_increment,
    postfix_increment,
    prefix_decrement,
    postfix_decrement,
    assignment,
    addition_assignment,
    subtraction_assignment,
    multiplication_assignment,
    division_assignment,
    modulus_assignment,
    shift_left_assignment,
    shift_right_assignment,
    and_assignment,
    xor_assignment,
    or_assignment,
    comma_seperator,
    array_size_specifier,
    grammar_statement,
    scope_block,
    identifier,
    struct_definition,
    struct_declaration,
};

std::string syntax_type_to_string(syntax_type type)
{
    switch (type)
    {
    case syntax_type::null_type: return "null_type";
    case syntax_type::translation_unit: return "translation_unit";
    case syntax_type::parenthesized_expression: return "parenthesized_expression";
    case syntax_type::type_cast: return "type_cast";
    case syntax_type::parenthesized_type: return "parenthesized_type";
    case syntax_type::compound_literal: return "compound_literal";
    case syntax_type::compound_literal_parameters: return "compound_literal_parameters";
    case syntax_type::index_operation: return "index_operation";
    case syntax_type::function_call: return "function_call";
    case syntax_type::function_arguments: return "function_arguments";
    case syntax_type::function_declaration: return "function_declaration";
    case syntax_type::function_definition: return "function_definition";
    case syntax_type::declaration_specifier: return "declaration_specifier";
    case syntax_type::parameter_list: return "parameter_list";
    case syntax_type::parameter_declaration: return "parameter_declaration";
    case syntax_type::type_specifier: return "type_specifier";
    case syntax_type::derefrence_operation: return "derefrence_operation";
    case syntax_type::conditional_operation: return "conditional_operation";
    case syntax_type::conditional_operator_false_case: return "conditional_operator_false_case";
    case syntax_type::for_loop: return "for_loop";
    case syntax_type::for_loop_parenthesis: return "for_loop_parenthesis";
    case syntax_type::if_statement: return "if_statement";
    case syntax_type::else_statement: return "else_statement";
    case syntax_type::condition: return "condition";
    case syntax_type::switch_statement: return "switch_statement";
    case syntax_type::case_statement: return "case_statement";
    case syntax_type::goto_statement: return "goto_statement";
    case syntax_type::goto_destination: return "goto_destination";
    case syntax_type::break_statement: return "break_statement";
    case syntax_type::declarator: return "declarator";
    case syntax_type::multiplication_operation: return "multiplication_operation";
    case syntax_type::division_operation: return "division_operation";
    case syntax_type::addition_operation: return "addition_operation";
    case syntax_type::subtraction_operation: return "subtraction_operation";
    case syntax_type::member_access: return "member_access";
    case syntax_type::pointer_member_access: return "pointer_member_access";
    case syntax_type::modulus_operation: return "modulus_operation";
    case syntax_type::shift_left_operation: return "shift_left_operation";
    case syntax_type::shift_right_operation: return "shift_right_operation";
    case syntax_type::greater_than_comparison: return "greater_than_comparison";
    case syntax_type::less_than_comparison: return "less_than_comparison";
    case syntax_type::equal_comparison: return "equal_comparison";
    case syntax_type::not_equal_comparison: return "not_equal_comparison";
    case syntax_type::less_than_equal_comparison: return "less_than_equal_comparison";
    case syntax_type::greater_than_equal_comparison: return "greater_than_equal_comparison";
    case syntax_type::address_operation: return "address_operation";
    case syntax_type::bitwise_and_operation: return "bitwise_and_operation";
    case syntax_type::bitwise_not_operation: return "bitwise_not_operation";
    case syntax_type::bitwise_xor_operation: return "bitwise_xor_operation";
    case syntax_type::bitwise_or_operation: return "bitwise_or_operation";
    case syntax_type::logical_or_operation: return "logical_or_operation";
    case syntax_type::logical_and_operation: return "logical_and_operation";
    case syntax_type::logical_not_operation: return "logical_not_operation";
    case syntax_type::size_of_operation: return "size_of_operation";
    case syntax_type::align_of_operation: return "align_of_operation";
    case syntax_type::unary_positive: return "unary_positive";
    case syntax_type::unary_negation: return "unary_negation";
    case syntax_type::prefix_increment: return "prefix_increment";
    case syntax_type::postfix_increment: return "postfix_increment";
    case syntax_type::prefix_decrement: return "prefix_decrement";
    case syntax_type::postfix_decrement: return "postfix_decrement";
    case syntax_type::assignment: return "assignment";
    case syntax_type::addition_assignment: return "addition_assignment";
    case syntax_type::subtraction_assignment: return "subtraction_assignment";
    case syntax_type::multiplication_assignment: return "multiplication_assignment";
    case syntax_type::division_assignment: return "division_assignment";
    case syntax_type::modulus_assignment: return "modulus_assignment";
    case syntax_type::shift_left_assignment: return "shift_left_assignment";
    case syntax_type::shift_right_assignment: return "shift_right_assignment";
    case syntax_type::and_assignment: return "and_assignment";
    case syntax_type::xor_assignment: return "xor_assignment";
    case syntax_type::or_assignment: return "or_assignment";
    case syntax_type::comma_seperator: return "comma_seperator";
    case syntax_type::array_size_specifier: return "array_size_specifier";
    case syntax_type::grammar_statement: return "grammar_statement";
    case syntax_type::scope_block: return "scope_block";
    case syntax_type::identifier: return "identifier";
    case syntax_type::struct_definition: return "struct_definition";
    case syntax_type::struct_declaration: return "struct_declaration";
    default: return "unknown";
    }
}

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
    char priority = 0;
};

struct bracket_information {
    size_t index = 0; //NEEDS to be size_t in order to not be undefined behavior when i cast later
    type Type = type::null; 
};

struct key_pair {
    std::string unnamedKey;
    std::string memberName;
};

struct symbol_data {
    std::string name; //also the key
    AST_NODE* destinationNode = nullptr; //used for linking functions, labels, and structs
    symbol_data* underlyingType = nullptr; //the int in int* x, also used for structs and return type of function ptrs
    std::vector<baseType> typeObject; //contains info abt the type (doesnt include the identifier or underlying type)
    std::vector<AST_NODE*> arraySizes; //ptr to the part of the AST where the expression for the size(s) is (also used for function parameters by pointing to the parameter_definition)
    std::unordered_map<std::string, std::string> memberKeyTable; //used for structs and possibly unions
    symbol_type type = symbol_type::Void;
    bool defined = false;
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
        declareType("int");
        declareVariable("a"); declareVariable("b"); declareVariable("c");
        declareVariable("d"); declareVariable("e"); declareVariable("f");
        declareVariable("g"); declareVariable("h"); declareVariable("i");
        createFunctionScope();
    }
    ~symbol_table_info() {
        symbolTableVector.clear();
        tableStack.clear();
        labelTableVector.clear();
        labelTableStack.clear();
    }
    //hardcode in the types for int, float, char, etc
    symbol_data* getIdentifier(std::string key) { //assumes it exists 
        for (size_t i = 1; i <= tableStack.size(); i++) {
            std::unordered_map<std::string, symbol_data>* currentTable = tableStack.c[tableStack.size() - i];
            if (currentTable->find(key) != currentTable->end()) {
                std::cout << i << std::endl;
                return &(tableStack.c[tableStack.size() - i]->at(key));
            }
        }
        std::cout << "Couldnt find identifier: " << key << std::endl;
        return nullptr;
    }

    symbol_data* checkIdentifier(std::string key) { //checks for an identifier in the same scope (for determining whether it can be shadowed)
        std::unordered_map<std::string, symbol_data>* currentTable = tableStack.c[tableStack.size() - 1];
        if (currentTable->find(key) != currentTable->end()) {
            return &(tableStack.c[tableStack.size() - 1]->at(key));
        }
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

    symbol_data* declareType(std::string key) {
        symbol_data* data = declareIdentifier(key);
        data->type = symbol_type::Type;
        return data;
    }

    symbol_data* declareFunction(std::string key) {
        symbol_data* data = declareIdentifier(key);
        data->type = symbol_type::Function;
        return data;
    }

    symbol_data* declareVariable(std::string key) {
        symbol_data* data = declareIdentifier(key);
        data->type = symbol_type::Variable;
        return data;
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
        if (!labelTableStack.empty() and tableStack.top() == labelTableStack.top()) {
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
    
    if (!(node->key.empty())) std::cout << "key: " << node->key;
    std::cout << " syntax_type: " << syntax_type_to_string(node->type) << " children: " << node->branches.size() << " index: " << node->index;
    if (node->endIndex != 0) std::cout << " : "  << node->endIndex;
    
    std::cout << std::endl;
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

                    token* closeBracketToken = &tokenVector[bracketVector[closeBracketIndex].index];

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

AST_NODE* parse_token(size_t inputIndex, AST_NODE* lastNode, bool nud, size_t end); //protoype

AST_NODE* pratt_parse_node(AST_NODE* lastNode, size_t inputNodeIndex, bool nud, size_t end = 0) {
    static size_t endIndex = 0;
    if (end) endIndex = end;
    token* selfToken = &tokenVector[inputNodeIndex];


    size_t rightNodeIndex = inputNodeIndex + 1;
    switch (selfToken->Type) {
    case type::openParenthesis:
    case type::openBracket:
    case type::openCurlyBracket:
        rightNodeIndex = *(size_t*)selfToken->value + 1;
    }
    if (rightNodeIndex < endIndex) {
        token* rightToken = &tokenVector[rightNodeIndex];
        char rightPriority = getPriority(rightToken->Type, false); //we are assuming the rightToken to be led, this assumption can be corrected later
        if (lastNode != nullptr) {
            token* lastToken = &tokenVector[lastNode->index];
            if (lastToken->priority >= rightPriority) {
                return parse_token(inputNodeIndex, lastNode, nud, endIndex); //return parsed self
            }
            else { //if rightPriority is greater //add a while loop to continue this recursion untill the right priority is no longer higher aka the right tree is complete
                AST_NODE* selfNode = parse_token(inputNodeIndex, lastNode, nud, endIndex);
                AST_NODE* rightData = nullptr;
                if (selfNode and selfNode->endIndex < rightNodeIndex) rightData = pratt_parse_node(selfNode, rightNodeIndex, false);
                else rightData = selfNode;
                while (lastToken->priority < tokenVector[rightData->endIndex + 1].priority and rightData->endIndex + 1 < endIndex) { //if this still greater then continue building the rightNode tree
                    rightData = pratt_parse_node(rightData, rightData->endIndex + 1, false);
                }
                return rightData;
            }
        }
        else {
            if (selfToken->priority >= rightPriority) {
                return parse_token(inputNodeIndex, lastNode, nud, endIndex); //return parsed self
            }
            else {
                AST_NODE* selfNode = parse_token(inputNodeIndex, lastNode, nud, endIndex);
                AST_NODE* rightData = nullptr;
                if (selfNode and selfNode->endIndex < rightNodeIndex) rightData = pratt_parse_node(selfNode, rightNodeIndex, false);
                else rightData = selfNode;
                return rightData;
            }
        }
    } else {
        return parse_token(inputNodeIndex, lastNode, nud, end); //return parsed self
    }
}


//parent node independent
AST_NODE* parse_expression(size_t start, size_t end) { //assuming start is '(' and end is ')'
    AST_NODE* leftSide = nullptr;
    size_t index = start + 1;
    if (start + 1 < end) {
        leftSide = pratt_parse_node(leftSide, start + 1, true, end); //skip to the last parsed node    
        assert(leftSide);
        if (leftSide != nullptr) {
            for (index = leftSide->endIndex + 1; index < end; index++) {
                if (tokenVector[index].Type == eol or tokenVector[index].Type == colon) { //stops parsing at eol
                    return leftSide;
                }
                AST_NODE* temp = pratt_parse_node(leftSide, index, false, end);
                if (temp != nullptr) leftSide = temp; //skip to the last parsed node    
                index = leftSide->endIndex;
            }
        }
    }
    return leftSide; //returns top node of the expression //doesnt automatically add as a child
}

//parse expression and automatically child it to an input parentnode bracket
AST_NODE* parse_expression(AST_NODE* inputNode) { //assuming index is '(' and endindex is ')'
    AST_NODE* leftSide = parse_expression(inputNode->index, inputNode->endIndex);
    if (leftSide != nullptr) inputNode->branches.push_back(leftSide);
    return leftSide; //returns top node of the expression
}

AST_NODE* parse_declaration_specifier(size_t position, size_t endIndex); //forward dec

void parse_parenthesis_in_expression(AST_NODE* parenthesisNode) {
    size_t index = parenthesisNode->index + 1;
    if (index >= parenthesisNode->endIndex) {
        std::cout << "err expected content in () at: " << parenthesisNode->index << std::endl;
        assert(index < parenthesisNode->endIndex);
        return;
    }

    auto parse_as_expression = [&parenthesisNode]() {
        parenthesisNode->type = syntax_type::parenthesized_expression;
        AST_NODE* expressionNode = parse_expression(parenthesisNode);
        if (expressionNode == nullptr) {
            std::cout << "err expected expression at: " << parenthesisNode->index + 1 << std::endl;
            assert(expressionNode != nullptr);
        }
        };

    auto parse_as_type = [&parenthesisNode, index]() {
        parenthesisNode->type = syntax_type::parenthesized_type;
        AST_NODE* typeNode = parse_declaration_specifier(index, parenthesisNode->endIndex);
        typeNode->parent = parenthesisNode;
        parenthesisNode->branches.push_back(typeNode);
        };

    token* currentToken = &tokenVector[parenthesisNode->index + 1];

    switch (currentToken->Type) {
    case variable: {
        std::string key((char*)currentToken->value, currentToken->size);
        symbol_data* data = symbolTableInfo->getIdentifier(key);

        assert(data != nullptr);
        if (data != nullptr) {
            std::cout << "parsing!:" << key << ": " << std::endl;
            if (data->type == symbol_type::Type) { //if the variable is a type aka declaration
                parse_as_type();
                break;
            }
            else {
                parse_as_expression();
                break;
            }
        }
        else {
            std::cout << "err attempted to parse unknown identifier:" << key << ": at: " << parenthesisNode->index + 1 << std::endl;
            break;
        }
        //if neither a type nor label then treat it as an expression
        parse_as_expression();
        break;
    }
    case type::booleanType:
    case type::charType:
    case type::intType:
    case type::floatType:
    case type::doubleType:
    case type::shortType:
    case type::longType:
    case type::unsignedType:
    case type::signedType: //include all the type qualifiers etc here
        parse_as_type();
        break;
    default:
        parse_as_expression();
        break;
    }
}


void parse_arguments(AST_NODE* inputNode) { //assuming index is '(' and endindex is ')'
    size_t lastCommaIndex = inputNode->index + 1;
    for (size_t index = inputNode->index + 1; index < inputNode->endIndex; inputNode++) {
        token* currentToken = &tokenVector[index];
        switch (currentToken->Type) {
        case openParenthesis:
            index = *(size_t*)currentToken->value;
            break;
        case comma:
            AST_NODE* currentExpression = parse_expression(lastCommaIndex - 1, index);
            currentExpression->parent = inputNode;
            inputNode->branches.push_back(currentExpression);
            lastCommaIndex = index;
            break;
        }
    }
    if (inputNode->endIndex - lastCommaIndex > 1) {
        AST_NODE* currentExpression = parse_expression(lastCommaIndex - 1, inputNode->endIndex);
        currentExpression->parent = inputNode;
        inputNode->branches.push_back(currentExpression);
    } else {
        std::cout << "Comma with nothing infront of it syntax error" << std::endl;
    }
}

AST_NODE* parse_token(size_t inputIndex, AST_NODE* lastNode, bool nud, size_t end) {
    std::cout << "parsing token" << inputIndex << std::endl;
    token* inputToken = &tokenVector[inputIndex];
    AST_NODE* inputNode = inputToken->node;
    assert(inputNode == nullptr);
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
        inputNode->endIndex = inputNode->index;
    };

    auto binary_operation = [nud, inputNode, lastNode, &inputIndex, unary_prefix_operation, unary_postfix_operation]() {
        assert(not nud);
        if (nud) std::cout << "err attempting a binary operation on a nud token:" << tokenVector[inputIndex].Type << "  at: " << inputIndex << std::endl;
        unary_postfix_operation();
        unary_prefix_operation();
    };
    
    switch (inputToken->Type) {
    case type::multiplication:
        if (nud) {
            inputNode->type = syntax_type::derefrence_operation;
            unary_prefix_operation();
        }
        else {
            inputNode->type = syntax_type::multiplication_operation;
            binary_operation();
        }
        break;
    case type::division:
        inputNode->type = syntax_type::division_operation;
        binary_operation();
        break;
    case type::structAccess:
        inputNode->type = syntax_type::member_access;
        binary_operation();
        break;
    case type::structPointerAccess:
        inputNode->type = syntax_type::pointer_member_access;
        binary_operation();
        break;
    case type::modulus:
        inputNode->type = syntax_type::modulus_operation;
        binary_operation();
        break;
    case type::shiftLeft:
        inputNode->type = syntax_type::shift_left_operation;
        binary_operation();
        break;
    case type::shiftRight:
        inputNode->type = syntax_type::shift_right_operation;
        binary_operation();
        break;
    case type::greaterThan:
        inputNode->type = syntax_type::greater_than_comparison;
        binary_operation();
        break;
    case type::lessThan:
        inputNode->type = syntax_type::less_than_comparison;
        binary_operation();
        break;
    case type::greaterThanEqual:
        inputNode->type = syntax_type::greater_than_equal_comparison;
        binary_operation();
        break;
    case type::equalsComp:
        inputNode->type = syntax_type::equal_comparison;
        binary_operation();
        break;
    case type::notEquals:
        inputNode->type = syntax_type::not_equal_comparison;
        binary_operation();
        break;
    case type::lessThanEqual:
        inputNode->type = syntax_type::less_than_equal_comparison;
        binary_operation();
        break;
    case type::address:
        if (nud) {
            inputNode->type = syntax_type::address_operation;
            unary_prefix_operation();
        }
        else {
            inputNode->type = syntax_type::bitwise_and_operation;
            binary_operation();
        }
        break;
    case type::bitwiseNot:
        if (nud) {
            inputNode->type = syntax_type::bitwise_not_operation;
            unary_prefix_operation();
        }
        else std::cout << "attempted to parse nud operation as led at" << inputIndex << std::endl;
        break;
    case type::logicalNot:
        if (nud) {
            inputNode->type = syntax_type::logical_not_operation;
            unary_prefix_operation();
        }
        else std::cout << "attempted to parse nud operation as led at" << inputIndex << std::endl;
        break;
    case type::xOr:
        inputNode->type = syntax_type::bitwise_xor_operation;
        binary_operation();
        break;
    case type::bitwiseOr:
        inputNode->type = syntax_type::bitwise_or_operation;
        binary_operation();
        break;
    case type::Or:
        inputNode->type = syntax_type::logical_or_operation;
        binary_operation();
        break;
    case type::And:
        inputNode->type = syntax_type::logical_and_operation;
        binary_operation();
        break;
    case type::sizeOf:
        if (nud) {
            inputNode->type = syntax_type::size_of_operation;
            unary_prefix_operation();
        }
        else std::cout << "attempted to parse nud operation as led at" << inputIndex << std::endl;
        break;
    case type::alignOf: //add support for alignof in the future
        if (nud) {
            inputNode->type = syntax_type::align_of_operation;
            unary_prefix_operation();
        }
        else std::cout << "attempted to parse nud operation as led at" << inputIndex << std::endl;
        break;
    case type::conditional: //add earlier termination in the future when semantic analysis is improved/implemented
        assert(!nud);
        if (nud) std::cout << "err attempted to parse ? as nud at: " << inputNode->index << std::endl;
        else {
            inputNode->type = syntax_type::conditional_operation;
            unary_postfix_operation();
            AST_NODE* trueCase = parse_expression(inputNode->index, end);
            trueCase->parent = inputNode;
            inputNode->branches.push_back(trueCase);
            inputNode->endIndex = trueCase->endIndex;

            token* colonToken = nullptr;
            if (inputNode->endIndex + 1 < tokenVector.size()) {
                colonToken = &tokenVector[inputNode->endIndex + 1];
            }

            if (colonToken == nullptr or colonToken->Type != type::colon) std::cout << "err parsing the false case for conditional expected : at: " << inputNode->endIndex + 1 << std::endl;
            else {
                inputToken->priority = BINDING_POWER_CONDITIONAL;
                colonToken->priority = BINDING_POWER_CONDITIONAL;
                inputIndex = inputNode->endIndex + 1;
                unary_prefix_operation();
            }
        }
        break; 
    case type::addition:
        if (nud) {
            inputNode->type = syntax_type::unary_positive;
            unary_prefix_operation();
        }
        else {
            inputNode->type = syntax_type::addition_operation;
            binary_operation();
        }
        break;
    case type::subtraction:
        if (nud) {
            inputNode->type = syntax_type::unary_negation;
            unary_prefix_operation();
        } else {
            inputNode->type = syntax_type::subtraction_operation;
            binary_operation();
        }
        break;
    case type::increment:
        if (nud) {
            inputNode->type = syntax_type::prefix_increment;
            unary_prefix_operation();
        }
        else {
            inputNode->type = syntax_type::postfix_increment;
            unary_postfix_operation();
        }
        break;
    case type::decrement:
        if (nud) {
            inputNode->type = syntax_type::prefix_decrement;
            unary_prefix_operation();
        }
        else {
            inputNode->type = syntax_type::postfix_decrement;
            unary_postfix_operation();
        }
        break;
    case type::equals:
        inputNode->type = syntax_type::assignment;
        binary_operation();
        break;
    case type::plusEquals:
        inputNode->type = syntax_type::addition_assignment;
        binary_operation();
        break;
    case type::minusEquals:
        inputNode->type = syntax_type::subtraction_assignment;
        binary_operation();
        break;
    case type::multEquals:
        inputNode->type = syntax_type::multiplication_assignment;
        binary_operation();
        break;
    case type::divEquals:
        inputNode->type = syntax_type::division_assignment;
        binary_operation();
        break;
    case type::modEquals:
        inputNode->type = syntax_type::modulus_assignment;
        binary_operation();
        break;
    case type::shiftLeftEquals:
        inputNode->type = syntax_type::shift_left_assignment;
        binary_operation();
        break;
    case type::shiftRightEquals:
        inputNode->type = syntax_type::shift_right_assignment;
        binary_operation();
        break;
    case type::andEquals: //bitwise and not logical and
        inputNode->type = syntax_type::and_assignment;
        binary_operation();
        break;
    case type::xorEquals:
        inputNode->type = syntax_type::xor_assignment;
        binary_operation();
        break;
    case type::orEquals: //bitwise and not logical or
        inputNode->type = syntax_type::or_assignment;
        binary_operation();
        break;
    case type::comma:
        if (nud) std::cout << "err comma cannot be nud at: " << inputNode->index << std::endl;
        binary_operation();
        inputNode->type = syntax_type::comma_seperator;
        break;
    case type::openParenthesis:
        inputNode->endIndex = *(size_t*)((*inputToken).value);
        parse_parenthesis_in_expression(inputNode);

        if (inputNode->type == syntax_type::parenthesized_type) {
            inputNode->type = syntax_type::type_cast;
            if (nud) {
                inputIndex = inputNode->endIndex;
                unary_prefix_operation();
                if (inputNode->branches[0]->type == syntax_type::compound_literal_parameters) {
                    inputNode->type = syntax_type::compound_literal;
                }
            }
            else {
                std::cout << "err passing a compile-time type into a function call at: " << inputNode->index << std::endl;
                unary_postfix_operation();
            }
        }
        else {
            if (!nud) {
                inputNode->type = syntax_type::function_call; //implement semantic check in the future
                unary_postfix_operation();
                inputNode->endIndex = *(size_t*)((*inputToken).value);
            }
            //if its nud then its just for grouping so nothing else required
        }
        break;
    case type::openBracket:
        if (!nud) {
            inputNode->endIndex = *(size_t*)((*inputToken).value);
            inputNode->type = syntax_type::index_operation;
            parse_expression(inputNode);
            unary_postfix_operation();
            inputNode->endIndex = *(size_t*)((*inputToken).value);
        }
        else std::cout << "err attempted to parse [ as nud at: " << inputIndex << std::endl;
        break;
    case type::openCurlyBracket:
        if (nud) {
            inputNode->endIndex = *(size_t*)((*inputToken).value);
            inputNode->type = syntax_type::compound_literal_parameters;
            parse_expression(inputNode);
        }
        else std::cout << "err attempted to parse { as led at: " << inputIndex << std::endl;
        break;
    case type::variable: {
        inputNode->type = syntax_type::identifier;
        std::string key = std::string((char*)inputToken->value, inputToken->size);
        inputNode->key = key; //get key for symbol table
        inputNode->endIndex = inputIndex;
        break;
    }
    case type::colon:
    case type::eol:
        assert(nud);
        if (!nud) {
            std::cout << "err: attempting to parse ; or : as led at: " << inputIndex << std::endl;
        }
        delete inputNode;
        return nullptr;
    default:
        std::cout << "err: unknown token: " << inputToken->Type << "at: " << inputIndex << std::endl;
        inputNode->endIndex = inputIndex;
    }
    assert(inputNode != nullptr);
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

            if (tokenVector[position + 1].Type == variable or tokenVector[position + 1].Type == multiplication) {
                declarationSpecifierData.typeObject.push_back(tokenToBaseType(currentToken->Type));
                underlyingType = symbolTableInfo->getIdentifier("int");
                if (tokenVector[position + 1].Type == variable) position++;
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

void parse_declaration_identifiers(size_t endIndex, AST_NODE* declaration_node, std::vector<key_pair>* outVector = nullptr, bool use_unnamed_keys = false) { //inParameters refers to whether this declaration is parameter declaration
    std::cout << "parsing declaration identifiers at!: " << declaration_node->index << std::endl;
    bool typeObjectReady = false;
    size_t startIndex = declaration_node->endIndex + 1;
    size_t start = startIndex;

    auto parse_prefix_modifiers = [declaration_node](AST_NODE* identifierNode, size_t inputPosition, size_t start, symbol_data& currentSymbol) {
        for (size_t position = inputPosition; position-- >= start;) { //prefix operators
            if (tokenVector[position].Type == multiplication) {
                currentSymbol.typeObject.push_back(baseType::_pointer);
                if (start > declaration_node->endIndex) return true;
            }
        }
        return false;
    };
    
    auto parse_postfix_modifiers = [&parse_prefix_modifiers, &declaration_node, endIndex, outVector, use_unnamed_keys](AST_NODE* identifierNode, size_t start) {
                
        size_t identifierPosition = identifierNode->index;
        size_t prefixPosition = identifierPosition;

        symbol_data currentSymbol;

        bool enclosed_ptr = false;
        bool valid_function = true;

        size_t position = identifierPosition + 1;
        for (; position < endIndex; position++) { //postfix operators
            if (tokenVector[position].Type == openBracket) {
                valid_function = false;
                currentSymbol.typeObject.push_back(_array);
                AST_NODE* arraySizeExpression = new AST_NODE;
                arraySizeExpression->parent = identifierNode;
                arraySizeExpression->type = syntax_type::array_size_specifier;
                arraySizeExpression->index = position; //[ token
                arraySizeExpression->endIndex = *(size_t*)tokenVector[position].value; //] token
                position = arraySizeExpression->endIndex;
                recursive_parse(arraySizeExpression); //add logic for getting the size and etc of the array (for multi dim arrays)
                currentSymbol.arraySizes.push_back(arraySizeExpression);
                identifierNode->endIndex = arraySizeExpression->endIndex;
            } else if (tokenVector[position].Type == closeParenthesis) {
                if (parse_prefix_modifiers(identifierNode, prefixPosition, *(size_t*)tokenVector[position].value, currentSymbol)) enclosed_ptr = true;
                prefixPosition = position;
                identifierNode->endIndex = position;
            } else if (tokenVector[position].Type == openParenthesis) {

                if (enclosed_ptr) {
                    valid_function = false;
                    currentSymbol.typeObject.push_back(_function);
                    AST_NODE* functionParameters = new AST_NODE;
                    functionParameters->parent = identifierNode;
                    functionParameters->type = syntax_type::parameter_declaration; //output specfication for the function
                    functionParameters->index = position; //start (
                    functionParameters->endIndex = *(size_t*)tokenVector[functionParameters->index].value; //end )
                    identifierNode->branches.push_back(functionParameters);
                    recursive_parse(functionParameters);
                    identifierNode->endIndex = functionParameters->endIndex;
                    position = functionParameters->endIndex;
                    continue;
                }
                else if (not valid_function) {
                    std::cout << "err, invalid function at: " << position << std::endl;
                }
                assert(valid_function);
                valid_function = false;
                if (not declaration_node->branches.empty()) {
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

                symbol_data* underlyingTypeData = symbolTableInfo->getIdentifier(declaration_node->key);
                underlyingTypeData->typeObject.insert(underlyingTypeData->typeObject.end(), currentSymbol.typeObject.begin(), currentSymbol.typeObject.end());

                if (functionParameters->endIndex + 1 < tokenVector.size() and tokenVector[functionParameters->endIndex + 1].Type == openCurlyBracket) {
                    std::cout << "function definition" << std::endl;
                    
                    identifierNode->type = syntax_type::function_definition;
                    parse_prefix_modifiers(identifierNode, prefixPosition, start, currentSymbol);

                    currentSymbol.destinationNode = identifierNode;
                    currentSymbol.type = symbol_type::Function;
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
                    
                    std::string key;

                    symbol_data* prototypeDeclaration = nullptr;
                    if (outVector == nullptr or not use_unnamed_keys) { //there is no need to check for a previous declaration if using unnamed keys
                        prototypeDeclaration = symbolTableInfo->getIdentifier(identifierNode->key);
                    }
                    if (prototypeDeclaration != nullptr) { //if there was a previous forwards declaration
                        if (!(prototypeDeclaration->underlyingType->typeObject == underlyingTypeData->typeObject and
                        prototypeDeclaration->underlyingType->underlyingType == underlyingTypeData->underlyingType)) {
                            std::cout << "err, discrepancy between return type of function definition and declaration at: " << position << std::endl;
                        } else {
                            AST_NODE* prototypeParameters = prototypeDeclaration->arraySizes[0];
                            
                            if (prototypeParameters->branches.size() == functionParameters->branches.size()) {
                                bool discrepancy = false;
                                for (size_t i = 0; i < prototypeParameters->branches.size() and !discrepancy; i++) {
                                    AST_NODE* prototypeBranch = prototypeParameters->branches[i];
                                    AST_NODE* definitionBranch = functionParameters->branches[i];

                                    symbol_data* prototypeDeclaratorData = symbolTableInfo->getIdentifier(prototypeBranch->key);
                                    symbol_data* definitionDeclaratorData = symbolTableInfo->getIdentifier(definitionBranch->key);
                                    
                                    symbol_data* definitionDeclaratandData = symbolTableInfo->getIdentifier(definitionBranch->branches[0]->key);
                                    symbol_data* prototypeDeclaratandData = symbolTableInfo->getIdentifier(prototypeBranch->branches[0]->key);

                                    if (not ((prototypeDeclaratorData->typeObject == definitionDeclaratorData->typeObject //checks if the types for the parameters are all the same
                                        and prototypeDeclaratorData->underlyingType == definitionDeclaratorData->underlyingType) //if there is a descrepancy then the flag is set to true
                                        and (definitionDeclaratandData->typeObject == prototypeDeclaratandData->typeObject
                                            and definitionDeclaratandData->underlyingType == prototypeDeclaratandData->underlyingType))) {
                                        discrepancy = true;
                                    }
                                }
                                if (discrepancy) std::cout << "err, discrepancy between return type of function definition and declaration at: " << position << std::endl;
                            }
                            else {
                                std::cout << "err, discrepancy between return type of function definition and declaration at: " << position << std::endl;
                            }
                        }
                    }
                    else {
                        if (outVector != nullptr and use_unnamed_keys) {
                            key_pair pair;
                            pair.memberName = identifierNode->key;
                            pair.unnamedKey = symbolTableInfo->generateUnnamedKey();
                            outVector->push_back(pair);

                            symbol_data* definitonData = symbolTableInfo->declareFunction(pair.unnamedKey);
                            definitonData->underlyingType = underlyingTypeData;
                            definitonData->arraySizes.push_back(functionParameters);
                        }
                        else {
                            symbol_data* definitonData = symbolTableInfo->declareFunction(identifierNode->key);
                            definitonData->underlyingType = underlyingTypeData;
                            definitonData->arraySizes.push_back(functionParameters);
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
            else if (tokenVector[position].Type == equals or tokenVector[position].Type == comma or tokenVector[position].Type == eol) {
                if (outVector != nullptr and use_unnamed_keys) {
                    key_pair pair;
                    pair.memberName = identifierNode->key;
                    pair.unnamedKey = symbolTableInfo->generateUnnamedKey();
                    outVector->push_back(pair);

                    parse_prefix_modifiers(identifierNode, prefixPosition, start, currentSymbol);
                    currentSymbol.underlyingType = symbolTableInfo->getIdentifier(declaration_node->key);
                    currentSymbol.type = symbol_type::Variable;
                    symbolTableInfo->declareIdentifier(pair.unnamedKey, currentSymbol);
                }
                else {
                    parse_prefix_modifiers(identifierNode, prefixPosition, start, currentSymbol);
                    currentSymbol.underlyingType = symbolTableInfo->getIdentifier(declaration_node->key);
                    currentSymbol.type = symbol_type::Variable;
                    symbolTableInfo->declareIdentifier(identifierNode->key, currentSymbol);
                }
                
                return position;
            }
        }

        if (outVector != nullptr and use_unnamed_keys) {
            key_pair pair;
            pair.memberName = identifierNode->key;
            pair.unnamedKey = symbolTableInfo->generateUnnamedKey();
            outVector->push_back(pair);

            parse_prefix_modifiers(identifierNode, prefixPosition, start, currentSymbol);
            currentSymbol.underlyingType = symbolTableInfo->getIdentifier(declaration_node->key);
            currentSymbol.type = symbol_type::Variable;
            symbolTableInfo->declareIdentifier(pair.unnamedKey, currentSymbol);
        }
        else {
            parse_prefix_modifiers(identifierNode, prefixPosition, start, currentSymbol);
            currentSymbol.underlyingType = symbolTableInfo->getIdentifier(declaration_node->key);
            currentSymbol.type = symbol_type::Variable;
            symbolTableInfo->declareIdentifier(identifierNode->key, currentSymbol);
        }

        return (size_t)0;
    };
    bool lastIsAssignment = false;
    size_t position = startIndex;
    for (; position < endIndex; position++) { //change the equals to be per identifier
        token* currentToken = &tokenVector[position];
        switch (currentToken->Type) {
        case comma:
            if (lastIsAssignment) {
                if (!(declaration_node->branches.empty())) {
                    AST_NODE* identifierNode = declaration_node->branches.back();
                    AST_NODE* value = parse_expression(start - 1, position);
                    identifierNode->branches.push_back(value);
                    value->parent = identifierNode;
                    identifierNode->endIndex = value->endIndex;
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
            identifierNode->endIndex = position;
            size_t newPosition = parse_postfix_modifiers(identifierNode, start);
            std::cout << identifierNode->key << " : " << start << " : " << newPosition << std::endl;
            
            if (identifierNode->type == syntax_type::null_type) {
                identifierNode->type = syntax_type::identifier;
            }
            
            if (newPosition == 0) {
                declaration_node->endIndex = identifierNode->endIndex;
                goto end;
            }
            else {
                declaration_node->endIndex = newPosition;
                
                position = newPosition;
                start = position + 1;
                
                if (tokenVector[newPosition].Type == equals) {
                    
                    size_t expressionEndIndex = newPosition;
                    for (size_t layers = 0; expressionEndIndex < endIndex; expressionEndIndex++) {
                        switch (tokenVector[expressionEndIndex].Type) {
                        case openBracket:
                        case openCurlyBracket:
                        case openParenthesis:
                            layers++;
                            break;
                        case closeBracket:
                        case closeCurlyBracket:
                        case closeParenthesis:
                            if (layers != 0) layers--;
                            else std::cout << "err expected matching parenthesis within expression at: " << expressionEndIndex << std::endl;
                            break;
                        case comma:
                            if (layers != 0) break;
                            else goto found;
                        }
                    }
                    found:
                    AST_NODE* identifierNode = declaration_node->branches.back();
                    AST_NODE* value = parse_expression(start - 1, expressionEndIndex);
                    identifierNode->branches.push_back(value);
                    value->parent = identifierNode;
                    identifierNode->endIndex = value->endIndex;
                    declaration_node->endIndex = identifierNode->endIndex;

                    position = identifierNode->endIndex;
                    start = position + 1;
                }
                else if (tokenVector[newPosition].Type == eol) {
                    position = newPosition - 1;
                }
            }
        }
            break;
        case eol:
            position--;
            goto end;
        case equals: //supposed to be found in the postfix parser
            std::cout << "err missing identifier at: " << position << std::endl;
            break;
        }
    }
end:
    assert(declaration_node->branches.back()->endIndex >= position);
    if (declaration_node->branches.back()->endIndex < position) std::cout << "err, unused tokens at: " << position << std::endl;
    if (declaration_node->branches.empty()) {
        std::cout << "err empty declaration at: " << declaration_node->index << std::endl;
    }
}

size_t parse_parameter_declaration_identifier(size_t endIndex, AST_NODE* declarationSpecifierNode) { //declaration_node->endIndex should be the end of the underlying type
        
        auto parse_prefix_modifiers = [](AST_NODE* identifierNode, size_t inputPosition, size_t start, symbol_data& currentSymbol) {
            for (size_t position = inputPosition; --position > start - 1;) { //prefix operators
                if (tokenVector[position].Type == multiplication) currentSymbol.typeObject.push_back(baseType::_pointer);
            }
        };

        auto parse_postfix_modifiers = [&parse_prefix_modifiers, &declarationSpecifierNode, &endIndex](AST_NODE* identifierNode, size_t start) {
            size_t identifierPosition = identifierNode->index;
            size_t prefixPosition = identifierPosition;
            symbol_data currentSymbol = *symbolTableInfo->declareVariable(identifierNode->key);
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

void parse_struct_definition(AST_NODE* structNode, size_t start) {
    symbol_data* structData = symbolTableInfo->getIdentifier(structNode->key);

    auto parse_as_declaration = [&structNode, structData](size_t index) {
        AST_NODE* declarationSpecifierNode = parse_declaration_specifier(index, structNode->endIndex);
        declarationSpecifierNode->parent = structNode;
        structNode->branches.push_back(declarationSpecifierNode);
        

        std::vector<key_pair> pairs;

        parse_declaration_identifiers(structNode->endIndex, declarationSpecifierNode, &pairs, true);
        assert(pairs.size() == declarationSpecifierNode->branches.size());
        for (size_t it = 0; it < pairs.size(); it++) {
            if (declarationSpecifierNode->branches[it]->type == syntax_type::function_declaration or
                declarationSpecifierNode->branches[it]->type == syntax_type::function_definition) {
                std::cout << "err, cannot have a function obj inside of a struct, at: " << index << std::endl;
            }
            key_pair pair = pairs[it];
            if (structData->memberKeyTable.find(pair.memberName) == structData->memberKeyTable.end()) {
                structData->memberKeyTable[pair.memberName] = pair.unnamedKey;
            }
            else {
                std::cout << "err, duplicate members in struct: " << structNode->key << " at: " << index << std::endl;
            }
        }
        return declarationSpecifierNode->endIndex;
    };
    
    for (size_t index = start; index < structNode->endIndex; index++) {
        token* currentToken = &tokenVector[index];
        switch (currentToken->Type) {
        case type::booleanType:
        case type::charType:
        case type::intType:
        case type::floatType:
        case type::doubleType:
        case type::shortType:
        case type::longType:
        case type::unsignedType:
        case type::signedType: //include all the type qualifiers etc here
            index = parse_as_declaration(index);
            break;
        case type::variable:
            std::string key((char*)currentToken->value, currentToken->size);
            symbol_data* data = symbolTableInfo->getIdentifier(key);
            if (data == nullptr) {
                std::cout << "err, attempted to parse unknown type in struct at: " << index << std::endl;
            }
            else {
                if (data->type == symbol_type::Type) {
                    index = parse_as_declaration(index);
                }
                else {
                    std::cout << "err, expected declaration at: " << index << std::endl;
                }
            }
        }
    }
}

void parse_single_statement(AST_NODE* inputNode, size_t firstIndex, size_t end) {
    std::cout << "parsing block!" << std::endl;

    auto parse_control_flow_statement = [&inputNode, firstIndex](const syntax_type statementType) {
        AST_NODE* newNode = new AST_NODE();
        newNode->type = statementType;
        newNode->parent = inputNode;
        newNode->index = firstIndex;
        inputNode->branches.push_back(newNode);
        recursive_parse(newNode);
        inputNode->endIndex = newNode->endIndex;
    };

    auto parse_declaration = [&inputNode]() {
        std::cout << "err, declaration in place of a statement" << std::endl;
    };

    token* currentToken = &tokenVector[firstIndex];
    switch (currentToken->Type) {
    case closeParenthesis:
    case closeBracket:
    case closeCurlyBracket:
    case colon:
        std::cout << "err, invalid token statement expected at: " << firstIndex << std::endl;
        break;
    case eol:
        break; //ignore these tokens because they mean nothing alone
    case ifStatement:
        parse_control_flow_statement(syntax_type::if_statement);
        break;
    case switchStatement:
        parse_control_flow_statement(syntax_type::switch_statement);
        break;
    case breakStatement:
        parse_control_flow_statement(syntax_type::break_statement);
        break;
    case gotoStatement:
        parse_control_flow_statement(syntax_type::goto_statement);
        break;
    case forStatement:
        parse_control_flow_statement(syntax_type::for_loop);
        break;
    case caseStatement:
        parse_control_flow_statement(syntax_type::case_statement);
        break;
    case variable: {
        std::string key((char*)currentToken->value, currentToken->size);
        std::cout << "currentToken size: " << key << std::endl;

        if (firstIndex + 1 < end and tokenVector[firstIndex + 1].Type == type::colon) { //label
            token* secondToken = &tokenVector[firstIndex + 1];

            symbol_data data;
            data.name = key;
            data.type = symbol_type::Label;

            AST_NODE* labelNode = new AST_NODE;
            labelNode->index = firstIndex;
            labelNode->key = key;
            labelNode->endIndex = end; //for bounds check tighten later
            labelNode->type = syntax_type::goto_destination;
            labelNode->parent = inputNode;
            inputNode->branches.push_back(labelNode);

            AST_NODE* destinationNode = new AST_NODE;
            destinationNode->index = labelNode->index + 2;
            destinationNode->parent = labelNode;
            destinationNode->type = syntax_type::grammar_statement;
            labelNode->branches.push_back(destinationNode);
            recursive_parse(destinationNode);
            labelNode->endIndex = destinationNode->endIndex; //tightened post bounds check

            data.destinationNode = labelNode;
            inputNode->endIndex = labelNode->endIndex;
            symbolTableInfo->declareIdentifier(key, data);
            break;
        }
        else {
            symbol_data* data = symbolTableInfo->getIdentifier(key);
            std::cout << "currentToken name: " << key << std::endl;

            if (data) {
                std::cout << "parsing!:" << key << ": " << std::endl;
                if (data->type == symbol_type::Type) { //if the variable is a type aka declaration
                    parse_declaration();
                    break;
                }
                else goto expression_case;
            }
            else {
                std::cout << "err attempted to parse unknown identifier:" << key << ": at: " << firstIndex << std::endl;
                assert(false);
                break;
            }
        }
        //if neither a type nor label then treat it as an expression
        goto expression_case;
    }
    case type::structType:
    case type::booleanType:
    case type::charType:
    case type::intType:
    case type::floatType:
    case type::doubleType:
    case type::shortType:
    case type::longType:
    case type::unsignedType:
    case type::signedType: //include all the type qualifiers etc here
        parse_declaration();
        break;
    default:
        assert(false);
    expression_case: {
        AST_NODE* expressionNode = parse_expression(firstIndex - 1, end);
        if (expressionNode != nullptr) {
            inputNode->branches.push_back(expressionNode);
            expressionNode->parent = inputNode;
            inputNode->endIndex = expressionNode->endIndex;
            break;
        }
        else {
            std::cout << "empty expression at: " << firstIndex << std::endl;
        }
        }
    }
}

void parse_block(AST_NODE* inputNode) { //inputNode->index is ( //endIndex is )
    std::cout << "parsing block!" << std::endl;
    
    auto parse_control_flow_statement = [&inputNode](const syntax_type statementType, size_t& i) {
        AST_NODE* newNode = new AST_NODE();
        newNode->type = statementType;
        newNode->parent = inputNode;
        newNode->index = i;
        inputNode->branches.push_back(newNode);
        recursive_parse(newNode);
        i = newNode->endIndex;
    };

    auto parse_declaration = [&inputNode](size_t& i) {
        AST_NODE* declarationSpecifierNode = parse_declaration_specifier(i, inputNode->endIndex);
        declarationSpecifierNode->parent = inputNode;
        inputNode->branches.push_back(declarationSpecifierNode);
        parse_declaration_identifiers(inputNode->endIndex, declarationSpecifierNode);
        if (declarationSpecifierNode->parent == inputNode) { //if it was a variable skip to the end of the declaration
            i = declarationSpecifierNode->endIndex;
            if (i + 1 > tokenVector.size() and tokenVector[i + 1].Type == eol) {
                i++;
            }
        } else {
            i = declarationSpecifierNode->branches[0]->endIndex; //if it was a function skip to the end of the function
        }
    };


    for (size_t i = inputNode->index + 1; i < inputNode->endIndex; i++) { //endIndex is the )
        token* currentToken = &tokenVector[i];
        std::cout << "parsing token: " << i << "  with type: " << currentToken->Type << std::endl;
        switch (currentToken->Type) {
        case closeParenthesis:
        case closeBracket:
        case closeCurlyBracket:
        case eol:
        case colon:
            break; //ignore these tokens because they mean nothing alone
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
        case forStatement:
            parse_control_flow_statement(syntax_type::for_loop, i);
            break;
        case caseStatement:
            parse_control_flow_statement(syntax_type::case_statement, i);
            break;
        case variable: {
            std::string key((char*)currentToken->value, currentToken->size);
            std::cout << "currentToken size: " << key << std::endl;

            if (i + 1 < inputNode->endIndex and tokenVector[i + 1].Type == type::colon) { //label
                token* secondToken = &tokenVector[i + 1];

                symbol_data data;
                data.name = key;
                data.type = symbol_type::Label;

                AST_NODE* labelNode = new AST_NODE;
                labelNode->index = i;
                labelNode->key = key;
                labelNode->endIndex = inputNode->endIndex; //for bounds check tighten later
                labelNode->type = syntax_type::goto_destination;
                labelNode->parent = inputNode;
                inputNode->branches.push_back(labelNode);

                AST_NODE* destinationNode = new AST_NODE;
                destinationNode->index = labelNode->index + 2;
                destinationNode->parent = labelNode;
                destinationNode->type = syntax_type::grammar_statement;
                labelNode->branches.push_back(destinationNode);
                recursive_parse(destinationNode);
                labelNode->endIndex = destinationNode->endIndex; //tightened post bounds check

                data.destinationNode = labelNode;
                i = labelNode->endIndex;
                symbolTableInfo->declareIdentifier(key, data);
                break;
            } else {
                symbol_data* data = symbolTableInfo->getIdentifier(key);
                std::cout << "currentToken name: " << key << std::endl;

                if (data) {
                    std::cout << "parsing!:" << key << ": " << std::endl;
                    if (data->type == symbol_type::Type) { //if the variable is a type aka declaration
                        parse_declaration(i);
                        break;
                    }
                    else goto expression_case;
                }
                else {
                    std::cout << "err attempted to parse unknown identifier:" << key << ": at: " << i << std::endl;
                    assert(false);
                    break;
                }
            }
            //if neither a type nor label then treat it as an expression
            goto expression_case;
            }
        case type::structType:
            if (i + 1 < tokenVector.size()) {
                std::string key;
                if (tokenVector[i + 1].Type == type::variable) {
                    token* identifierToken = &tokenVector[i + 1];
                    key = std::string((char*)identifierToken->value, identifierToken->size);
                }
                else if (tokenVector[i + 1].Type == type::openBracket) {
                    key = symbolTableInfo->generateUnnamedKey();
                }
                else {
                    std::cout << "err, expected identifier or {} at: " << i + 1 << std::endl;
                }

                
                AST_NODE* structNode = new AST_NODE;
                structNode->index = i;
                structNode->parent = inputNode;
                inputNode->branches.push_back(structNode);

                symbol_data* data = symbolTableInfo->declareType(key);
                if (data == nullptr and i + 2 < tokenVector.size()) {
                    data = symbolTableInfo->getIdentifier(key);
                }

                if (i + 2 < tokenVector.size() and tokenVector[i + 2].Type == openCurlyBracket) { //definition
                    if (data->defined) {
                        std::cout << "err, cannot redefine existing struct: " << structNode->key << std::endl;
                    }
                    structNode->type = syntax_type::struct_definition;
                    structNode->key = key;
                    data->destinationNode = structNode;
                    structNode->endIndex = *(size_t*)tokenVector[i + 2].value;
                    data->defined = true;
                    parse_struct_definition(structNode, i + 2);
                }
                else {
                    structNode->key = key;
                    structNode->endIndex = i + 1;
                    structNode->type = syntax_type::struct_declaration;
                }

                i = structNode->endIndex;

                if (i + 1 < tokenVector.size() and tokenVector[i + 1].Type != type::eol) { //add check for if the struct has been completed
                    parse_declaration_identifiers(inputNode->endIndex, structNode); //add a semantic check to ensure struct objects cannot be made before definition (ptrs to an undefined but declared struct is allowed)
                    
                    if (not data->defined) {
                        for (size_t it = 0; it < structNode->branches.size(); it++) { //checks to see if all declared identifiers are objects or pointers
                            AST_NODE* currentNode = structNode->branches[it];
                            symbol_data* currentData = symbolTableInfo->getIdentifier(currentNode->key);
                            assert(currentData != nullptr);
                            for (size_t specifierIt = 0; specifierIt < currentData->typeObject.size(); specifierIt++) {
                                if (currentData->typeObject[specifierIt] == baseType::_pointer or currentData->typeObject[specifierIt] == baseType::_array) goto isPtr;
                            }
                            std::cout << "err, cannot declare a struct object when struct internals are unknown, err at: " << i << std::endl;
                            break;
                        isPtr:;
                        }
                    }
                    
                    if (not ALLOW_FUNCTION_DECLARATIONS_FOLLOWING_STRUCT_DEFINITIONS) {
                        for (size_t it = data->memberKeyTable.size(); it < structNode->branches.size(); it++) { //checks to make sure no functions are declared
                            AST_NODE* currentNode = structNode->branches[it];
                            if (currentNode->type == syntax_type::function_declaration or currentNode->type == syntax_type::function_definition) std::cout << "err, function declaration cannot follow a struct definition: " << structNode->endIndex << std::endl;
                            else {
                                symbol_data* currentData = symbolTableInfo->getIdentifier(currentNode->key);
                                assert(currentData != nullptr);
                                for (size_t specifierIt = 0; specifierIt < currentData->typeObject.size(); specifierIt++) {
                                    if (currentData->typeObject[specifierIt] == baseType::_function) std::cout << "err, function declaration cannot follow a struct definition: " << structNode->endIndex << std::endl;
                                }
                            }
                        }
                    }

                    i = structNode->endIndex;
                    if (i + 1 > tokenVector.size() and tokenVector[i + 1].Type == eol) {
                        i++;
                    }

                }
                else if (i + 1 >= tokenVector.size()) std::cout << "err, incomplete struct definition at: " << structNode->endIndex << std::endl;
            }
            else {
                std::cout << "err, expected a identifer for the struct name at: " << i << std::endl;
            }

            break;
        case type::booleanType:
        case type::charType:
        case type::intType:
        case type::floatType:
        case type::doubleType:
        case type::shortType:
        case type::longType:
        case type::unsignedType:
        case type::signedType: //include all the type qualifiers etc here
            parse_declaration(i);
            break;
        default:
            assert(false);
        expression_case: {
            AST_NODE* expressionNode = parse_expression(i - 1, inputNode->endIndex);
            if (expressionNode != nullptr) {
                inputNode->branches.push_back(expressionNode);
                expressionNode->parent = inputNode;
                i = expressionNode->endIndex;
                break;
            }
            else {
                std::cout << "empty expression at: " << i << std::endl;
            }
        }
        }
    }
}

void recursive_parse(AST_NODE* inputNode) {
    size_t position = inputNode->index;
    if (position >= tokenVector.size()) {
        std::cout << "err attempted to parse outside of range at: " << position << std::endl;
        assert(false);
    }
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
            bodyNode->type = syntax_type::grammar_statement; //single line vs scope block is handled here
            inputNode->branches.push_back(bodyNode);
            recursive_parse(bodyNode);

            inputNode->endIndex = bodyNode->endIndex;
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
            bodyNode->type = syntax_type::grammar_statement; //single line vs scope block is handled here
            inputNode->branches.push_back(bodyNode);
            recursive_parse(bodyNode);

            if (bodyNode->endIndex + 1 < tokenVector.size() and tokenVector[bodyNode->endIndex + 1].Type == elseStatement) {
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
        case for_loop: {
            token* parenthesisToken = &tokenVector[position + 1];
            if (parenthesisToken->Type != openParenthesis) {
                std::cout << "err, expected parenthesis at: " << position + 1 << std::endl;
            }

            AST_NODE* parenthesis = new AST_NODE;
            parenthesis->value = symbolTableInfo->createLocalScope();
            parenthesis->type = syntax_type::for_loop_parenthesis;
            parenthesis->index = position + 1;
            parenthesis->endIndex = *(size_t*)parenthesisToken->value;
            parenthesis->parent = inputNode;
            recursive_parse(parenthesis);
            inputNode->branches.push_back(parenthesis);

            AST_NODE* bodyNode = new AST_NODE;
            bodyNode->index = parenthesis->endIndex + 1;
            bodyNode->parent = inputNode;
            bodyNode->type = syntax_type::grammar_statement; //single line vs scope block is handled here
            inputNode->branches.push_back(bodyNode);
            recursive_parse(bodyNode);

            inputNode->endIndex = bodyNode->endIndex;
        }
            break;
        case for_loop_parenthesis:
        {
            token* currentToken = &tokenVector[inputNode->index + 1];
            if (currentToken->Type == eol) {
                inputNode->branches.push_back(nullptr);
            }
            parse_block(inputNode);
            //add semantic checks
            break;
        }
        case grammar_statement: {
            token* currentToken = &tokenVector[position];
            if (currentToken->Type == openCurlyBracket) {
                inputNode->endIndex = *(size_t*)currentToken->value;
                inputNode->type = syntax_type::scope_block;
                inputNode->value = symbolTableInfo->createLocalScope();
                parse_block(inputNode);
                symbolTableInfo->exitScope();
            }
            else { //single statement
                inputNode->endIndex = inputNode->parent->parent->endIndex;
                parse_single_statement(inputNode, inputNode->index, inputNode->parent->endIndex);
            }
            break;
        }
        case scope_block:
            parse_block(inputNode);
            symbolTableInfo->exitScope();
            //implement later
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
        default:
            currentNode.Type = currentType;
            char priority = getPriority(currentType, false); //assume led 
            if (priority == 0) priority = getPriority(currentType, true); //default to nud if none is found
            currentNode.priority = priority;
            break;

    }
    tokenVector.push_back(currentNode);
}

void tokenize(std::string& source, std::vector<bracket_information>& bracketVector, std::unordered_map<std::string, type> tokenKeywordTable) { //fills the tokenVector
    size_t startOfLine = 0; //not sure if this will matter (possible removal)
    size_t startOfSymbol = 0;
    size_t endOfSymbol = 0;
    type currentType = type::null;
    type lastType = type::null;
    int currentNum = 0;
    
    size_t i = 0;
    size_t start = 0;
    auto generic_lex = [&startOfSymbol, &endOfSymbol, &currentType, &source, &bracketVector, &i](type inputType) {
        interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
        currentType = inputType;
        startOfSymbol = i;
        endOfSymbol = i + 1;
    };

    auto specific_lex = [&startOfSymbol, &endOfSymbol, &currentType, &source, &bracketVector, &i, &start](type inputType) {
        static size_t callCount = 0;
        callCount++;
        interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
        std::cout << "specific lexxed: " << source.substr(startOfSymbol, endOfSymbol - startOfSymbol) << " at " << startOfSymbol << " with len " << endOfSymbol - startOfSymbol << std::endl;
        startOfSymbol = start;
        endOfSymbol = i + 1;
        currentType = inputType;
    };

    size_t sourceLength = source.length();

    auto peek_forwards = [sourceLength, &source, &i](char inputChar) {
        if (i < sourceLength) { //i is incremented before use if the previous cond in the and is true
            if (source[i + 1] == inputChar) {
                i++;
                return true;
            }
        }
        return false;
    };

    auto lex_complex_token = [&startOfSymbol, &endOfSymbol, &currentType, &source, &bracketVector, &i, &start, peek_forwards, specific_lex](std::unordered_map<char, type> possibleExtensions, type def) {
        for (auto it = possibleExtensions.begin(); it != possibleExtensions.end(); ++it) {
            if (peek_forwards(it->first)) {
                specific_lex(it->second);
                return;
            }
        }
        specific_lex(def);
    };

    for (; i < sourceLength; i++) {
        start = i;
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
            else {
                generic_lex(structAccess);
            }
            break;
        case '-': //add decrement operator with variables in the future
            if (peek_forwards('=')) {
                specific_lex(plusEquals);
            } else if (peek_forwards('-')) {
                specific_lex(decrement);
            } else if (peek_forwards('>')) {
                specific_lex(structAccess);
            } else {
                specific_lex(subtraction);
            }
            break;
        case '+': //add increment operator with variables in the future
        {
            lex_complex_token(
                {
                    {'+', type::increment},
                    {'=', type::plusEquals},
                }, type::addition);
            break;
        }
        case '*': //add struct operator with variables in the future
            lex_complex_token(
                {
                    {'=', type::multEquals},
                }, type::multiplication);
            break;
        case '/': //add comment support in the future
            lex_complex_token(
                {
                    {'=', type::divEquals},
                }, type::division);
            break;
        case '%': //add comment support in the future
            lex_complex_token(
                {
                    {'=', type::modEquals},
                }, type::modulus);
            break;
        case '|':
            lex_complex_token(
                {
                    {'|', type::Or},
                    {'=', type::orEquals},
                }, type::bitwiseOr);
            break;
        case '^':
            lex_complex_token(
                {
                    {'=', type::xorEquals},
                }, type::xOr);
            break;
        case '&':
            lex_complex_token(
                {
                    {'&', type::And},
                    {'=', type::andEquals},
                }, type::address);
            break;
        case '?':
            generic_lex(conditional);
            break;
        case '(':
            generic_lex(openParenthesis); 
            break;
        case ')':
            generic_lex(closeParenthesis);
            break;
        case '[':
            generic_lex(openBracket); 
            break;
        case ']':
            generic_lex(closeBracket);
            break;
        case '{':
            generic_lex(openCurlyBracket);
            break;
        case '}':
            generic_lex(closeCurlyBracket);
            break;
        case ':':
            generic_lex(colon);
            break;
        case ',':
            generic_lex(comma);
            break;
        case '=':
            lex_complex_token(
                {
                    {'=', type::equalsComp},
                }, type::equals);
            break;
        case '!':
            lex_complex_token(
                {
                    {'=', type::notEquals},
                }, type::logicalNot);
            break;
        case '~':
            generic_lex(bitwiseNot);
            break;
        case '<':
            if (peek_forwards('<')) {
                if (peek_forwards('=')) {
                    specific_lex(shiftLeftEquals);
                } else {
                    specific_lex(shiftLeft);
                }
            } else if (peek_forwards('=')) {
                specific_lex(lessThanEqual);
            } else {
                specific_lex(lessThan);
            }
            break;
        case '>':
            if (peek_forwards('>')) {
                if (peek_forwards('=')) {
                    specific_lex(shiftRightEquals);
                } else {
                    specific_lex(shiftRight);
                }
            } else if (peek_forwards('=')) {
                specific_lex(greaterThanEqual);
            }
            else {
                specific_lex(greaterThan);
            }
            break;
        case '\n':
        case '\t':
        case '\v':
        case '\r':
        case '\f':
        case ' ':
            if (currentType != type::null) {
                interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
                currentType = type::null;
            }
            startOfSymbol = i + 1;
            endOfSymbol = i + 1;
            break;
        default: //change this when variables are added
            if (currentType != variable) {
                generic_lex(variable);
                break;
            }
            else {
                endOfSymbol = i + 1;
                try {
                    type keyword = tokenKeywordTable.at(source.substr(startOfSymbol, endOfSymbol - startOfSymbol));
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
    if (currentType != type::null) interpretSymbol(startOfSymbol, endOfSymbol, currentType, source, bracketVector);
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
    //implement type qualifiers here
    tokenKeywordTable["short"] = type::shortType;
    tokenKeywordTable["long"] = type::longType;
    tokenKeywordTable["signed"] = type::signedType;
    tokenKeywordTable["unsigned"] = type::unsignedType;
    //actual types (int, char, etc) are declared in the constructor for the symbol table class

    //control flow keywords
    tokenKeywordTable["for"] = type::forStatement;
    tokenKeywordTable["if"] = type::ifStatement;
    tokenKeywordTable["else"] = type::elseStatement;
    tokenKeywordTable["switch"] = type::switchStatement;
    tokenKeywordTable["case"] = type::caseStatement;

    //other keywords
    tokenKeywordTable["sizeof"] = type::sizeOf;
    tokenKeywordTable["alignof"] = type::alignOf;
    tokenKeywordTable["and"] = type::And;
    tokenKeywordTable["or"] = type::Or;
    tokenKeywordTable["xor"] = type::xOr;

    tokenKeywordTable["struct"] = type::structType;
    tokenKeywordTable["enum"] = type::enumType;
    tokenKeywordTable["union"] = type::unionType;


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
