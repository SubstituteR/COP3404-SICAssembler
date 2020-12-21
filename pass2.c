#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma region GCC

#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wunused-function"

#pragma endregion

#pragma region macros

#pragma region OOP bootstrap
/*
*
*	Some bootstrapping macros to help make C feel a little more like C++.
*	___XYZ are "internal" functions to add some "object"-ness such as calling constructors and destructors
*	OBJECT macro creates a struct with the required fields and the associated ___XYZ constructor and destructor methods.
*	FUNCTION (and FUNCTION_NOARG) create functions with a "this" parameter to give some instance-level method functionality
*	VALID = simple NULL check.
*	NEW = create an "instance" of an "object" and invoke constructor (and set destructor reference)
*	DELETE = calls "object" destructor for easy clean up. Destructors should invoke ___defaultConstructor to free the memory for the "object instance."
*/
typedef enum { false, true } bool;
typedef void* (*constructor)();
typedef void (*destructor)(void*);
typedef struct object { destructor destructor; } object;

unsigned long objectCount = 0; //unsigned long = space for plenty of objects.

void* ___constructObject(constructor a, destructor b)
{
	void* instance = a();
	((struct object*)instance)->destructor = b;
	++objectCount;
	return instance;
}
void* ___invokeDestructor(object** who)
{
	if (who != NULL && *who != NULL && (*who)->destructor != NULL)
	{
		(*who)->destructor(*who);
		--objectCount;
		*who = NULL;
	}
	return NULL;
}
bool ___is_valid(const void* who) {
	return who != NULL;
}
/*mischief*/
void ___memory_managed()
{
	if (objectCount != 0)
		printf("Program exited without properly cleaning up some objects. This indiciates a potential memory leak! (%li != 0)\n", objectCount);
}
object* ___defaultConstructor() { printf("default constructor\n"); return calloc(1, sizeof(object*)); }
void ___defaultDestructor(void* who) { free(who); }
int ___main(int argc, char* argv[]);
#define VALID(who) ___is_valid(who)
#define ASSERT_MEM ___memory_managed()
#define MAIN ___main
int main(int argc, char* argv[]) /* All code is in MAIN. This entry point simply maps to that while also adding some debugging code. */
{
	int ret = MAIN(argc, argv);
#if DEBUG_MEM
	ASSERT_MEM;
#endif
	return ret;
}

#define CONSTRUCTOR(TYPE) TYPE* ___construct##TYPE()
#define DESTRUCTOR(TYPE) void ___destroy##TYPE(TYPE* instance)
#define USE_DEFAULT_CONSTRUCTOR(TYPE) CONSTRUCTOR(TYPE) {return (TYPE*)___defaultConstructor(); }
#define USE_DEFAULT_DESTRUCTOR(TYPE)  DESTRUCTOR(TYPE){___defaultDestructor(instance);};
#define USE_DEFAULT_CTORS(TYPE) USE_DEFAULT_CONSTRUCTOR(TYPE); USE_DEFAULT_DESTRUCTOR(TYPE);
#define NEW(TYPE) ___constructObject((constructor) ___construct##TYPE, (destructor) ___destroy##TYPE)
#define DELETE(WHO) ___invokeDestructor((object**)&WHO)
#define OBJECT(TYPE, REST) \
typedef struct TYPE { \
	destructor destructor; \
	REST; \
} TYPE; \
CONSTRUCTOR(TYPE); \
DESTRUCTOR(TYPE); \

#define FUNCTION(TYPE, NAME, RET, ...) static RET TYPE##_##NAME(TYPE* _this, __VA_ARGS__)
#define STATIC_FUNCTION(TYPE, NAME, RET, ...) static RET TYPE##_##NAME( __VA_ARGS__)
#define FUNCTION_NOARG(TYPE, NAME, RET) static RET TYPE##_##NAME(TYPE* _this)
#pragma endregion

#pragma region object defs

OBJECT(vector, object** data; unsigned int num; unsigned int limit;)
FUNCTION(vector, push_back, void, object*);
FUNCTION_NOARG(vector, grow, void);

OBJECT(string, unsigned int length; char* c_str; unsigned int limit; ) //string object.
FUNCTION(string, append, void, const char*); //append text to string.
FUNCTION(string, append_int, void, const int);
FUNCTION(string, setEqual, string*, string*); //make string A into B and destroy the orignal A.
FUNCTION(string, areSame, bool, string*); //checks if A == B by value.
FUNCTION(string, format, void, const char*, ...);
STATIC_FUNCTION(string, make_and_format, string*, const char*, ...);
FUNCTION_NOARG(string, hash, unsigned int); //gets the hash of a string.
FUNCTION(string, split, void, vector*, const char*);

OBJECT(pair, unsigned int first; unsigned int second;) //key-value pair object for hash table.
FUNCTION(pair, make, void, unsigned int, unsigned int); //create key-value pair from string and int.

OBJECT(bucket, string* first; pair* second;)
FUNCTION(bucket, make, void, const char*, pair*);
/*
* Hash table data-structure. TODO non const char* versions of has/remove
*/
OBJECT(hashTable, bucket** buckets; unsigned int num; unsigned int limit;); //hash table object.
FUNCTION(hashTable, insert, void, const char* k, pair* v); //insert item into hash table.
FUNCTION(hashTable, remove, void, const char*); //remove item from hash table.
FUNCTION(hashTable, has, bool, const char*); //checks if hash table includes item.
FUNCTION_NOARG(hashTable, grow, void); //if the number of buckets is insufficient to hold items, increase the number of buckets.
FUNCTION(hashTable, get, bucket*, const char*);

OBJECT(file, FILE* handle;);
FUNCTION(file, open, void, const char*, const char*);
FUNCTION_NOARG(file, close, void);
FUNCTION_NOARG(file, length, int);
FUNCTION(file, readAll, void, string* str);

OBJECT(instruction, string* symbol; string* opcode; string* operand; string* comment; unsigned int line;);

void* rvalue_to_lvalue(void* rvalue, unsigned int sizeof_rvalue)
{
	void* lvalue = calloc(1, sizeof_rvalue);
	memcpy(lvalue, &rvalue, sizeof_rvalue);
	return lvalue;
}
#define LVALUE(A) \
rvalue_to_lvalue(A, sizeof(A))


#pragma endregion

#pragma region colors
#define RED "\033[22;31m"
#define LIGHT_RED "\033[1;31m"
#define CYAN "\033[22;36m"
#define LIGHT_CYAN "\033[1;36m"
#define NEWLINE "\033[0;27m\n"
#define RESET "\033[39m"

#define YELLOW "\033[22;33m"
#define GREEN "\033[22;32m"
#pragma endregion

#pragma endregion

#pragma region literals


typedef struct opcodes
{
	const char* mnemonic;
	unsigned int value;
} opcodes;

static const opcodes instructions[] = {
	{"ADD", 0x18}, {"ADDF",0x58}, {"ADDR", 0x90}, {"AND", 0x40}, {"CLEAR", 0xB4}, {"COMP", 0x28}, {"COMPF", 0x88},
	{"COMPR", 0xA0}, {"DIV", 0x24}, {"DIVF", 0x64}, {"DIVR", 0x9C}, {"FIX", 0xC4}, {"FLOAT", 0xC0}, {"HIO", 0xC0},
	{"J", 0x3C}, {"JEQ", 0x30}, {"JGT", 0x34}, {"JLT", 0x38}, {"JSUB", 0x48}, {"LDA", 0x00}, {"LDB", 0x68}, {"LDCH", 0x50},
	{"LDF", 0x70}, {"LDL", 0x08}, {"LDS", 0x6C}, {"LDT", 0x74}, {"LDX", 0x04}, {"LPS", 0xD0}, {"MUL", 0x20}, {"MULF", 0x60},
	{"MULR", 0x98}, {"NORM",0xC8}, {"OR",0x44}, {"RD",0xD8}, {"RMO", 0xAC}, {"RSUB", 0x4C}, {"SHIFTL", 0xA4}, {"SHIFTR", 0xA8},
	{"SIO", 0xF0}, {"SSK", 0xEC}, {"STA", 0x0C}, {"STB", 0x78}, {"STCH", 0x54}, {"STF", 0x80}, {"STI", 0xD4},
	{"STL", 0x14}, {"STS", 0x7C}, {"STSW", 0xE8}, {"STT", 0x84}, {"STX", 0x10}, {"SUB", 0x1C}, {"SUBF", 0x5C},
	{"SUBR", 0x94},{"SVC", 0xB0}, {"TD", 0xE0}, {"TIO", 0xF8} ,{"TIX", 0x2C}, {"TIXR", 0xB8},{ "WD", 0xDC}
};


static const unsigned char totalInstructions = 59;

static const char* directives[] = {
	"END", "BYTE", "WORD", "RESB", "RESW", "RESR", "EXPORTS", "START"
};
static const unsigned char totalDirectives = 8;

static const char invalidSymbolCharacters[] = { ' ', '$', '!', '=', '+', '-', '(', ')', '@' };
static const unsigned char totalInvalidSymbolCharacters = 9;

static const char whitespace[] = { ' ', '\r', '\n' };
int totalWhitespace = 3;

#define MIN_ALPHA 0x41 //A
#define MAX_ALPHA 0x5A //Z

#pragma endregion

#pragma region helpers

#pragma region general

bool fromHex(const char* who, unsigned long* val)
{
	char* end;
	*val = strtoul(who, &end, 16);
	return (*who != '\0' && *end == '\0');
}

void toHex(char* who, string* val)
{
	unsigned int accumulator = 0;
	while (who[accumulator] != 0)
		string_format(val, "%02X", who[accumulator++]);
}

bool fromDecimal(const char* who, long* val)
{
	char* end;
	*val = strtol(who, &end, 10); //ez checks
	return (*who != '\0' && *end == '\0');
}

string* removeWhitespace(string* str) //functional
{
	for (int i = str->length - 1; i >= 0; --i)
	{
		bool cont = false;
		if (str->c_str[i] == 0)
			continue; //next;
		for (int j = 0; j < totalWhitespace; j++)
		{
			if (str->c_str[i] == whitespace[j])
			{
				str->c_str[i] = 0; /* juuuust in case. */
				cont = true;
				break;
			}
		}
		if (!cont)
			break;
	}
	str->length = strlen(str->c_str);
	return str;
}

#pragma endregion

#pragma region pass one

bool isDirective(string* who)
{
	if (!VALID(who))
		return false;
	for (int i = 0; i < totalDirectives; ++i)
		if (strcmp(who->c_str, directives[i]) == 0)
			return true;
	return false;
}

#define nullptr 0

bool isOPCode(string* who, int* out)
{
	if (!VALID(who))
		return false;
	for (int i = 0; i < totalInstructions; ++i)
		if (strcmp(who->c_str, instructions[i].mnemonic) == 0)
		{
			if (out)
			{
				*out = instructions[i].value;
			}
			return true;
		}
	return false;
}

bool isSymbol(string* who)
{
	if (!VALID(who) || who->length > 6)
		return false;
	char c = who->c_str[0];
	if (MIN_ALPHA > c || c > MAX_ALPHA)
		return false;

	for (unsigned int i = 1; i < who->length; ++i)
	{
		c = who->c_str[i];
		for (unsigned int j = 0; j < totalInvalidSymbolCharacters; ++j)
		{
			char o = invalidSymbolCharacters[j];
			if (c == o)
				return false;
		}
	}
	return !isDirective(who);
}

bool isComment(string* who)
{
	if (!VALID(who) || who->c_str[0] != '#')
		return false;
	return true;
}

bool isOperand(string* who)
{
	return (!isComment(who) && !isDirective(who) && !isOPCode(who, nullptr));
}

void parseInstruction(instruction* parsed, string* what)
{
	vector* columns = NEW(vector);
	string_split(what, columns, "\t");
	switch (columns->num > 4 ? 4 : columns->num)
	{
	case 4:
		string_append(parsed->comment, removeWhitespace(((string*)columns->data[3]))->c_str);
	case 3:
		string_append(parsed->operand, removeWhitespace(((string*)columns->data[2]))->c_str);
	case 2:
		string_append(parsed->opcode, removeWhitespace(((string*)columns->data[1]))->c_str);
	case 1:
		string_append(parsed->symbol, removeWhitespace(((string*)columns->data[0]))->c_str);
	}
	DELETE(columns);
}

#pragma endregion

#pragma endregion

typedef struct program {
	unsigned long start;
	unsigned long end;
	unsigned long firstInstruction;
	string* name;
	hashTable* symtab;
	vector* instructions;
	vector* warnings;
} program;

unsigned int mnemonicToOpCode(string* opcode)
{
	if (!VALID(opcode))
		return 0;
	for (int i = 0; i < totalInstructions; ++i)
		if (strcmp(opcode->c_str, instructions[i].mnemonic) == 0)
			return instructions[i].value;
	return 0;
}

bool operandToValue(string* operand, program* programData, unsigned long* value)
{
	vector* data = NEW(vector);
	string_split(operand, data, ",");
	string* operand_value = (string*)data->data[0];
	if (isSymbol(operand_value))
	{
		if (hashTable_has(programData->symtab, operand_value->c_str))
		{
			bucket* hashData = hashTable_get(programData->symtab, operand_value->c_str);
			*value = hashData->second->second;
			DELETE(hashData);
		}
		else {
			DELETE(data);
			return false;
		}
	}
	else {
		switch (operand_value->length)
		{
		case 0:
			*value = 0;
			break;
		default:
			if (!fromHex(operand_value->c_str, value))
			{
				DELETE(data);
				return false;
			}
			break;
		}
	}

	if (data->num == 2)
	{

		if (strcmp(((string*)data->data[1])->c_str, "X") != 0)
		{
			DELETE(data);
			return false;
		}
		else
			*value |= 0x8000; //set upper 8th bit for indexed mode
	}

	DELETE(data);
	return true;
}

void printWarnings(vector* warnings)
{
	printf("\n %s%i%s WARNINGS DETECTED%s", LIGHT_CYAN, warnings->num, YELLOW, NEWLINE);

	printf("┌────────────────────────┐\n");
	printf("│ \33[7m%sWARRNING SUMMARY BELOW\33[27m%s │%s", YELLOW, RESET, NEWLINE);
	printf("└────────────────────────┘\n");

	for (unsigned int i = 0; i < warnings->num; ++i)
	{
		string* warning = (string*)warnings->data[i];
		printf(" %s%u.%s %s", LIGHT_CYAN, i + 1, RESET, warning->c_str);
	}
}

bool pass1(vector* lines, program* programData)
{
	vector* errors = NEW(vector);
	vector* symbols = NEW(vector);

	bool explicitStart = false; bool addressExceeded = false;  bool explicitEnd = false; unsigned int totalInstructions = 0;

	for (unsigned int i = 0; i < lines->num; ++i)
	{
		string* text = (string*)lines->data[i];
		unsigned int line = i + 1;
		//why <= 1? whitepace on one of the test files. A proper solution would be to remove leading and trailing whitespace,,,, TODO!
		if (removeWhitespace(text)->length < 1) //ignore last line potential whitespace.
		{
			if (i != lines->num - 1)
				vector_push_back(errors, (object*)string_make_and_format("%sLINE %s%i%s WAS EMPTY!%s", LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, NEWLINE));
		}	
		else {

			if (isComment(text)) //comment
				continue;
			if (explicitEnd) /* program has ended! */
			{
				vector_push_back(programData->warnings, (object*)string_make_and_format("%sINSTRUCTION ON LINE %s%i%s IS AFTER %sEND%s AND IS IGNORED%s", YELLOW, LIGHT_CYAN, line, YELLOW, LIGHT_CYAN, YELLOW, NEWLINE));
				continue;
			}
			if (!addressExceeded && programData->end >= 0x8000)
			{
				addressExceeded = true;
				vector_push_back(errors, (object*)string_make_and_format("%sMAXIMUM ADDRESSABLE MEMORY EXCEEDED %s%X%s >= %s%X%s BY LINE %s%i%s!%s",
					LIGHT_RED, LIGHT_CYAN, programData->end, LIGHT_RED, LIGHT_CYAN, 0x8000, LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, NEWLINE));
			}
			instruction* parsed = NEW(instruction);
			parseInstruction(parsed, text);
			parsed->line = line;
			if (programData->firstInstruction == (long unsigned int) -1 && parsed->opcode->length != 0 && isOPCode(parsed->opcode, nullptr))
				programData->firstInstruction = programData->end;
			++totalInstructions;
			switch (totalInstructions)
			{
			case 1:
				if (strcmp(parsed->opcode->c_str, "START") == 0)
				{
					if (parsed->operand->length == 0)
						vector_push_back(errors, (object*)string_make_and_format("%sLINE %s%i%s MISSING OPERAND!%s", LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, NEWLINE));
					else if (parsed->operand->c_str[0] == '-')
						vector_push_back(errors, (object*)string_make_and_format("%sLINE %s%i%s CONTAINS INVALID HEXADECIMAL %s%s%s < %s0%s%s",
							LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, LIGHT_CYAN, parsed->operand->c_str, LIGHT_RED, LIGHT_CYAN, LIGHT_RED, NEWLINE));
					else if (!fromHex(parsed->operand->c_str, &programData->end))
					{
						vector_push_back(errors, (object*)string_make_and_format("%sLINE %s%i%s CONTAINS INVALID HEXADECIMAL %s%s%s!%s",
							LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, LIGHT_CYAN, parsed->operand->c_str, LIGHT_RED, NEWLINE));
					}
					else {
						explicitStart = true;
						programData->start = programData->end;
						string_append(programData->name, parsed->symbol->c_str);
					}
				}
			/* fall through */
			default:
				if (parsed->symbol->length != 0)
				{
					if (!isSymbol(parsed->symbol))
					{
						string* errorMessage = NEW(string);
						string_format(errorMessage, "%sILLEGAL SYMBOL DEFINITION %s%s%s ON LINE %s%i%s\n", LIGHT_RED, LIGHT_CYAN, parsed->symbol->c_str, LIGHT_RED, LIGHT_CYAN, line, NEWLINE);
						vector_push_back(errors, (object*)errorMessage);
					}else if (hashTable_has(programData->symtab, parsed->symbol->c_str))
					{
						bucket* dupe = hashTable_get(programData->symtab, parsed->symbol->c_str);
						vector_push_back( /* ugly */
							errors, (object*)string_make_and_format(
							"%sDUPLICATE SYMBOL %s%s%s DETECTED ON LINE %s%i%s, DEFINED ON LINE %s%i%s!%s",
							LIGHT_RED, LIGHT_CYAN, parsed->symbol->c_str, LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, LIGHT_CYAN, dupe->second->first, LIGHT_RED, NEWLINE
						));
						DELETE(dupe);
					}
					else {
						pair* data = NEW(pair);
						pair_make(data, line, programData->end); //line, address
						hashTable_insert(programData->symtab, parsed->symbol->c_str, data);
						DELETE(data);
					};
					if (errors->num == 0) /* no point in adding to this symbol table, we already have a fatal error. */
						vector_push_back(symbols, (object*)string_make_and_format("│ %s%-8s%s│ %s%04lX %s│%s", YELLOW, parsed->symbol->c_str, RESET, LIGHT_CYAN, programData->end, RESET, NEWLINE));
				}
				if (parsed->opcode->length != 0)
				{
					vector_push_back(programData->instructions, (object*)parsed);
					if (isOPCode(parsed->opcode, nullptr))
					{
						programData->end += 3; //initial increase...
					}
					else if (isDirective(parsed->opcode))
					{
						if (strcmp(parsed->opcode->c_str, "START") == 0)
						{
							if (totalInstructions > 1)
								vector_push_back(errors, 
									(object*)string_make_and_format(
										"%sINVALID DIRECTIVE ON LINE %s%i%s, %sSTART%s IS ONLY VALID AS THE FIRST INSTRUCTION%s", 
										LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, LIGHT_CYAN, LIGHT_RED, NEWLINE));
							break;
						}
						else if (strcmp(parsed->opcode->c_str, "WORD") == 0)
						{
							long parsedValue = 0;
							if (!fromDecimal(parsed->operand->c_str, &parsedValue) || parsedValue >= 0xFFFFFF || parsedValue < -0xFFFFFF)
							{
								vector_push_back(errors, (object*)string_make_and_format(
									"%sINVALID VALUE %s%s%s ON LINE %s%i%s FOR DIRECTIVE %sWORD%s!%s",
									LIGHT_RED, LIGHT_CYAN, parsed->operand->c_str, LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, LIGHT_CYAN, LIGHT_RED, NEWLINE));
							}
							else
							{
																		 //upper bit is set!
								if (parsedValue > 0 && (parsedValue & 0x800000) == 0x800000)
								{
									vector_push_back(programData->warnings, (object*)string_make_and_format(
										"%sPOSSIBLE OVERFLOW ON LINE %s%i%s %s%i%s > %s%i%s FOR DIRECTIVE %sWORD%s!%s",
										YELLOW, LIGHT_CYAN, line, YELLOW, LIGHT_CYAN, parsedValue, YELLOW, LIGHT_CYAN, 0x7FFFFF, YELLOW, LIGHT_CYAN, YELLOW, NEWLINE));
								}
								programData->end += 3;
							}
						}
						else if (strcmp(parsed->opcode->c_str, "END") == 0)
						{
							explicitEnd = true;
						}
						else if (strcmp(parsed->opcode->c_str, "RESW") == 0)
						{
							long parsedValue = 0;
							if (!fromDecimal(parsed->operand->c_str, &parsedValue))
							{
								vector_push_back(errors, (object*)string_make_and_format(
									"%sINVALID VALUE %s%s%s ON LINE %s%i%s FOR DIRECTIVE %sRESW%s!%s",
									LIGHT_RED, LIGHT_CYAN, parsed->operand->c_str, LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, LIGHT_CYAN, LIGHT_RED, NEWLINE));
							}
							else
							{
								programData->end += 3 * parsedValue;
							}
						}
						else if (strcmp(parsed->opcode->c_str, "RESB") == 0)
						{
							long parsedValue = 0;
							if (!fromDecimal(parsed->operand->c_str, &parsedValue) || parsedValue < 1)
							{
								vector_push_back(errors, (object*)string_make_and_format(
									"%sINVALID VALUE %s%s%s ON LINE %s%i%s FOR DIRECTIVE %sRESB%s!%s",
									LIGHT_RED, LIGHT_CYAN, parsed->operand->c_str, LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, LIGHT_CYAN, LIGHT_RED, NEWLINE));
							}
							else
							{
								programData->end += parsedValue;
							}
						}
						else if (strcmp(parsed->opcode->c_str, "BYTE") == 0)
						{
							if (parsed->operand->length == 0)
							{
								vector_push_back(errors, (object*)string_make_and_format(
									"%sMISSING OPERAND ON LINE %s%i%s FOR DIRECTIVE %sBYTE%s!%s",
									LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, LIGHT_CYAN, LIGHT_RED, NEWLINE));
								break;
							}

							vector* op = NEW(vector);
							string_split(parsed->operand, op, "'"); //there will be an extra blank string at the end. -> C'a' -> a ' b ' c -> C,a,

							/* quick check if C/X */
							if (op->num < 3 || (strcmp(((string*)op->data[0])->c_str, "C") != 0 && strcmp(((string*)op->data[0])->c_str, "X") != 0))
							{
								vector_push_back(errors, (object*)string_make_and_format(
									"%sINVALID OPERAND %s%s%s ON LINE %s%i%s FOR DIRECTIVE %sBYTE%s!%s",
									LIGHT_RED, LIGHT_CYAN, removeWhitespace(parsed->operand)->c_str, LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, LIGHT_CYAN, LIGHT_RED, NEWLINE));
								DELETE(op);
								break;
							}
							string* opType = (string*)op->data[0];
							string* opData = (string*)op->data[1];
							if (strcmp(opType->c_str, "C") == 0)
							{
								programData->end += strlen(opData->c_str); //check the chars maybe ? idk
							}
							else { /* (strcmp(opType->c_str, "X") == 0) */ //This MUST be X
								int length = strlen(opData->c_str);
								unsigned long parsedValue = 0;
								if (!fromHex(opData->c_str, &parsedValue))
								{
									vector_push_back(errors, (object*)string_make_and_format(
										"%sINVALID HEX VALUE %s%s%s ON LINE %s%i%s FOR DIRECTIVE %sBYTE%s!%s",
										LIGHT_RED, LIGHT_CYAN, opData->c_str, LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, LIGHT_CYAN, LIGHT_RED, NEWLINE));
								}
								else
								{
									if ((length / 2) + (length % 2) != (length / 2))
									{
										vector_push_back(programData->warnings, (object*)string_make_and_format(
											"%sIMPLICIT LEADING ZERO ON LINE %s%i%s, %s%s%s DID YOU MEAN %s0%s%s?%s",
											YELLOW, LIGHT_CYAN, line, YELLOW, LIGHT_CYAN, opData->c_str, YELLOW, LIGHT_CYAN, opData->c_str, YELLOW, NEWLINE));
									}
									programData->end += (length / 2) + (length % 2); //round-up to nearest multiple of 2. FFF -> 0F FF NOT FF!!
								}
							}
							DELETE(op);
						}
						else {
							programData->end += 3;
						}
					}
					else
					{
						vector_push_back(errors, (object*)string_make_and_format(
							"%sILLEGAL INSTRUCTION %s%s%s ON LINE %s%i%s!%s",
							LIGHT_RED, LIGHT_CYAN, parsed->opcode->c_str, LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, NEWLINE));
					}
				}
				else {
				vector_push_back(errors, (object*)string_make_and_format(
					"%sMISSING INSTRUCTION ON LINE %s%i%s!%s",
					LIGHT_RED, LIGHT_CYAN, line, LIGHT_RED, NEWLINE));
				}
				break;
			}
			//DELETE(parsed);
		}
	}

	if (!explicitStart)
		vector_push_back(programData->warnings, (object*)string_make_and_format("%sSTART DIRECTIVE MISSING %sSTART —> 0%s", YELLOW, LIGHT_CYAN, NEWLINE));

	if (!explicitEnd)
		vector_push_back(programData->warnings, (object*)string_make_and_format("%sEND DIRECTIVE MISSING %sEND —> LAST INSTRUCTION%s", YELLOW, LIGHT_CYAN, NEWLINE));


	/* SYMBOL TABLE PRINTING
	if (symbols->num > 0)
	{
		printf("┌────────────────┐\n");
		printf("│  %sSymbol Table  %s│%s", errors->num == 0 ? GREEN : RED, RESET, NEWLINE);
		printf("├─────────┬──────┤\n");
		for (unsigned int i = 0; i < symbols->num - 1; ++i)
		{
			string* symbol = (string*)symbols->data[i];
			printf("%s", symbol->c_str);
			printf("├─────────┼──────┤%s",  NEWLINE);
		}



		string* symbol = (string*)symbols->data[symbols->num - 1];
		printf("%s", symbol->c_str);
		if (errors->num > 0)
		{
			printf("├─────────┴──────┤%s", NEWLINE);
			printf("│ \33[7m%sPASS 1 ABORTED\33[27m%s │ \n", RED, RESET);
			printf("└────────────────┘\n");
		}
		else {
			printf("└─────────┴──────┘\n");
		}
		
	}
	

	if (errors->num == 0)
	{
		printf("%sProgram Length %lX%s", LIGHT_CYAN, programData->end - programData->start, NEWLINE);
	}
	
	*/

	/* I know there's smarter ways to store error messages. I just didn't bother here... */
	if (programData->warnings->num > 0 && errors->num > 0)
		printWarnings(programData->warnings);
	if (errors->num > 0)
	{

		printf("\n %s%i%s ERRORS DETECTED%s", LIGHT_CYAN, errors->num, RED, NEWLINE);

		printf("┌─────────────────────┐\n");
		printf("│ \33[7m%sERROR SUMMARY BELOW\33[27m%s │%s", RED, RESET, NEWLINE);
		printf("└─────────────────────┘\n");

		for (unsigned int i = 0; i < errors->num; ++i)
		{
			string* error = (string*)errors->data[i];
			printf(" %s%u.%s %s", LIGHT_CYAN, i+1, RESET, error->c_str);
		}
	}

	bool passed = errors->num == 0;

	/* We take ownership */
	DELETE(lines);  DELETE(symbols); DELETE(errors);

	return passed;
}

void writeTRecord(string* output, string** data, unsigned int* location)
{
	unsigned int length = (*data)->length / 2 + (*data)->length % 2;
	string_format(output, "T%06X%02X%s\n", *location, length, (*data)->c_str);
	DELETE(*data);
	*data = NEW(string); //clear buffer completely...
	*location += length;
}

#define CAST(A, B) ((B*) A)

unsigned int minimum(unsigned int A, unsigned int B)
{
	return A < B ? A : B; //returns the smaller of 2 ints.
}

bool pass2(program* programData, string* output)
{
	vector* errors = NEW(vector);
	if (programData->name->length == 0)
	{
		vector_push_back(programData->warnings, (object*)string_make_and_format("%sPROGRAM NAME MISSING, %sNAME —> NONAME%s", YELLOW, LIGHT_CYAN, NEWLINE));
		string_format(output, "H%-6s%06X%06X\n", "NONAME", programData->start, programData->end - programData->start);
	}
	else {
		string_format(output, "H%-6s%06X%06X\n", programData->name->c_str, programData->start, programData->end - programData->start);
	}
	

	unsigned int lineStart = programData->start;
	string* builder = NEW(string);

	for (unsigned int i = 0; i < programData->instructions->num; ++i)
	{
		instruction* what = (instruction*)programData->instructions->data[i];
		if (strcmp(what->opcode->c_str, "START") == 0)
			continue; /* these are checked in pass 1 */

		string* part = NEW(string);
		int opcode = 0;
		if (isDirective(what->opcode) && strcmp(what->opcode->c_str, "END") != 0)
		{
			if (strcmp(what->opcode->c_str, "BYTE") == 0)
			{
				vector* parts = NEW(vector);
				string_split(what->operand, parts, "'");

				if (strcmp(((string*)parts->data[0])->c_str, "C") == 0)
				{
					toHex(CAST(parts->data[1], string)->c_str, part);
					int read = 0;
					char buffer[61] = { 0 };
					while (part->length - read > 60)
					{
						
						strncpy(buffer, part->c_str + read, 60);
						string_format(builder, "%s", buffer);
						writeTRecord(output, &builder, &lineStart);
						read += 60;
					}
					if (part->length - read > 0)
					{
						strcpy(buffer, part->c_str + read);
						string_format(builder, "%s", buffer);
					}
					DELETE(part); part = NEW(string);
				}
				else { /*X*/
					string_append(part, CAST(parts->data[1], string)->c_str);
				}
				DELETE(parts);
			}
			else if (strcmp(what->opcode->c_str, "RESB") == 0)
			{
				if (builder->length != 0)
					writeTRecord(output, &builder, &lineStart);
				long val = 0;
				fromDecimal(what->operand->c_str, &val); /* checked in pass 1 */
				lineStart += val;
			}
			else if (strcmp(what->opcode->c_str, "RESW") == 0)
			{
				if (builder->length != 0)
					writeTRecord(output, &builder, &lineStart);
				long val = 0;
				fromDecimal(what->operand->c_str, &val); /* checked in pass 1 */
				lineStart += val * 3;
			}
			else if (strcmp(what->opcode->c_str, "WORD") == 0)
			{
				long val = 0;
				fromDecimal(what->operand->c_str, &val); /* checked in pass 1 */
				string_format(part, "%06X", val);
			}
		}
		else if(isOPCode(what->opcode, &opcode) || strcmp(what->opcode->c_str, "END") == 0)
		{
			unsigned long operand_value = 0;
			if (operandToValue(what->operand, programData, &operand_value))
			{
				if (strcmp(what->opcode->c_str, "END") == 0 && operand_value != 0)
				{
					if (programData->firstInstruction != operand_value)
					{
						vector_push_back(programData->warnings, (object*)string_make_and_format(
							"%sINCORRECT VALUE FOR END, EXPECTED != ACTUAL (%s%X != %X%s) ON LINE %s%i%s!%s",
							YELLOW, LIGHT_CYAN, programData->firstInstruction, operand_value, YELLOW, LIGHT_CYAN, what->line, YELLOW, NEWLINE));
					}
					programData->firstInstruction = operand_value;
				}
				else {
					string_format(part, "%02X%04X", opcode, operand_value);
				}
				
			}
			else {
				vector* getFirst = NEW(vector);
				string_split(what->operand, getFirst, ",");
				string* symbol = CAST(getFirst->data[0], string);
				if (isSymbol(symbol))
				{
					vector_push_back(errors, (object*)string_make_and_format(
						"%sUNDEFINED SYMBOL %s%s%s FOR %s%s%s %s%s ON LINE %s%i%s!%s",
						LIGHT_RED, LIGHT_CYAN, removeWhitespace(symbol)->c_str, LIGHT_RED, LIGHT_CYAN, what->symbol->c_str, what->opcode->c_str, what->operand->c_str, LIGHT_RED, LIGHT_CYAN, what->line, LIGHT_RED, NEWLINE));
				}
				else {
					vector_push_back(errors, (object*)string_make_and_format(
						"%sUNDEFINED OPERAND %s%s%s FOR %s%s%s ON LINE %s%i%s!%s",
						LIGHT_RED, LIGHT_CYAN, removeWhitespace(symbol)->c_str, LIGHT_RED, LIGHT_CYAN, what->opcode->c_str, LIGHT_RED, LIGHT_CYAN, what->line, LIGHT_RED, NEWLINE));
				}
				DELETE(getFirst);
				
			}
			
		}

#if EXPANDED
		if (builder->length > 0)
		{
			writeTRecord(output, &builder, &lineStart);
		}
#else
		if (builder->length + part->length > 60)
		{
			writeTRecord(output, &builder, &lineStart);
		}
#endif
		string_format(builder, "%s", part->c_str);
		DELETE(part);
	}

	writeTRecord(output,&builder, &lineStart);
	string_format(output, "E%06X\n", programData->firstInstruction);

	DELETE(builder);

	if (programData->warnings->num > 0 && errors->num > 0)
		printWarnings(programData->warnings);
	if (errors->num > 0)
	{

		printf("\n %s%i%s ERRORS DETECTED%s", LIGHT_CYAN, errors->num, RED, NEWLINE);

		printf("┌─────────────────────┐\n");
		printf("│ \33[7m%sERROR SUMMARY BELOW\33[27m%s │%s", RED, RESET, NEWLINE);
		printf("└─────────────────────┘\n");

		for (unsigned int i = 0; i < errors->num; ++i)
		{
			string* error = (string*)errors->data[i];
			printf(" %s%u.%s %s", LIGHT_CYAN, i + 1, RESET, error->c_str);
		}
	}

	bool passed = errors->num == 0;
	DELETE(errors);
	return passed;



}

int MAIN(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("USAGE: %s <filename>\n", argv[0]);
		return -1;
	}

	file* fileInstructions = NEW(file);
	string* fileContents = NEW(string);

	file_open(fileInstructions, argv[1], "r");
	file_readAll(fileInstructions, fileContents);
	DELETE(fileInstructions);

	if (fileContents->length == 0)
	{
		printf("\n%sINVALID FILE, ASSEMBLER CAN NOT CONTINUE!%s\n", LIGHT_RED, NEWLINE);
		DELETE(fileContents);
		return -1;
	}

	int status = 0; //assembler status

	/*gets the lines of the file*/
	vector* lines = NEW(vector);
	program programData = { 0, 0, -1, NEW(string), NEW(hashTable), NEW(vector), NEW(vector) };

	string_split(fileContents, lines, "\n");
	DELETE(fileContents);

	if (!pass1(lines, &programData))
	{
		printf("%sPASS 1 FAIL, STOPPING ASSEMBLY%s", RED, NEWLINE);
		status = -1;
	}
	else {
		string* objectCode = NEW(string);
		if (!pass2(&programData, objectCode))
		{
			printf("%sPASS 2 FAIL, STOPPING ASSEMBLY%s", RED, NEWLINE);
			status = -1;
		}
		else {
			if (programData.warnings->num > 0)
				printWarnings(programData.warnings);
			string* fileName = NEW(string);
			string_append(fileName, argv[1]);
			string_append(fileName, ".obj");
			file* fileObjectFile = NEW(file);
			file_open(fileObjectFile, fileName->c_str, "w");
			fprintf(fileObjectFile->handle, "%s", objectCode->c_str);
			file_close(fileObjectFile);
			DELETE(fileObjectFile);
			DELETE(fileName);
			
		}
		DELETE(objectCode);
	}
	
	DELETE(programData.name);
	DELETE(programData.symtab);
	DELETE(programData.instructions);
	DELETE(programData.warnings);
	return status;
}

#pragma region objects

#pragma region file
CONSTRUCTOR(file)
{
	file* instance = calloc(1, sizeof(file));
	instance->handle = NULL;
#if DEBUG_MEM
	printf("[file] constructed\n");
#endif
	return instance;
}
DESTRUCTOR(file)
{
	if (!VALID(instance)) return;
	if (VALID(instance->handle))
		file_close(instance);
#if DEBUG_MEM
	printf("[file] destructed\n");
#endif
	return ___defaultDestructor(instance);
}
FUNCTION(file, open, void, const char* path, const char* mode)
{
	_this->handle = fopen(path, mode);
}
FUNCTION_NOARG(file, close, void)
{
	if (!VALID(_this) || !VALID(_this->handle))
		return;
	if (fclose(_this->handle) != 0)
	{
		printf("[FATAL] Unable to properly close file handle.");
		exit(-1);
	}
	_this->handle = NULL;
}
FUNCTION_NOARG(file, length, int)
{
	if (!VALID(_this) || !VALID(_this->handle))
		return 0;
	unsigned long value = 0;
	fseek(_this->handle, 0L, SEEK_END);
	value = ftell(_this->handle);
	fseek(_this->handle, 0L, SEEK_SET);
	return value;
}
FUNCTION(file, readAll, void, string* str)
{
	if (!VALID(_this) || !VALID(_this->handle) || !VALID(str))
		return;

	unsigned long length = file_length(_this);
	if (length == 0)
		return;
	char* buffer = calloc(length + 1, sizeof(char));
	fread(buffer, 1, length, _this->handle);
	string_append(str, buffer);
	free(buffer);
}


#pragma endregion

#pragma region hashTable
CONSTRUCTOR(hashTable)
{
	hashTable* instance = calloc(1, sizeof(hashTable));
	instance->limit = 32;
	instance->num = 0;
	instance->buckets = calloc(instance->limit, sizeof(bucket*));
#if DEBUG_MEM
	printf("[table] constructed\n");
#endif
	return instance;
}
DESTRUCTOR(hashTable)
{
	if (!VALID(instance)) return;
	for (unsigned int i = 0; i < instance->limit; ++i)
		if (VALID(instance->buckets[i]))
			DELETE(instance->buckets[i]);
	free(instance->buckets);
#if DEBUG_MEM
	printf("[table] destructed\n");
#endif
	return ___defaultDestructor(instance);
}

FUNCTION_NOARG(hashTable, grow, void)
{
	if (!VALID(_this) || !VALID(_this->buckets)) return;
	_this->limit <<= 1;
	bucket** nextBuckets = calloc(_this->limit, sizeof(bucket*));
	for (unsigned int i = 0; i < (_this->limit >> 1); ++i)
	{
		if (!VALID(_this->buckets[i]))
			continue;
		bucket* entry = _this->buckets[i];
		unsigned int hash = string_hash(entry->first) % _this->limit;
		while (VALID(nextBuckets[hash]))
			hash = (hash + 1) % _this->limit;
		nextBuckets[hash] = entry;
	}
	free(_this->buckets);
	_this->buckets = nextBuckets;
}

FUNCTION(hashTable, insert, void, const char* k, pair* v)
{
	bucket* entry = NEW(bucket);
	bucket_make(entry, k, v);
	unsigned int hash = string_hash(entry->first) % _this->limit;
	while (VALID(_this->buckets[hash]))
		if (string_areSame(_this->buckets[hash]->first, entry->first))
		{
			DELETE(entry); return;
		}
		else
			hash = (hash + 1) % _this->limit;
	_this->buckets[hash] = entry;
	if (++_this->num == _this->limit)
		hashTable_grow(_this);
}

FUNCTION(hashTable, remove, void, const char* val)
{
	string* key = NEW(string);
	string_append(key, val);
	unsigned int hash = string_hash(key) % _this->limit;
	while (VALID(_this->buckets[hash]))
	{
		if (string_areSame(_this->buckets[hash]->first, key))
		{
			DELETE(_this->buckets[hash]);
			_this->buckets[hash] = NULL;
			DELETE(key);
			return;
		}
		hash = (hash + 1) % _this->limit;
	}
	DELETE(key);
}
FUNCTION(hashTable, get, bucket*, const char* val)
{
	string* key = NEW(string);
	pair* what = NEW(pair);
	bucket* who = NEW(bucket);

	string_append(key, val);
	unsigned int hash = string_hash(key) % _this->limit;
	while (VALID(_this->buckets[hash]))
	{
		if (string_areSame(_this->buckets[hash]->first, key))
		{
			pair* data = _this->buckets[hash]->second;
			who->first = key;
			who->second = what;
			what->first = data->first;
			what->second = data->second;
			return who;
		}
		hash = (hash + 1) % _this->limit;
	}
	DELETE(key);
	DELETE(what);
	DELETE(who);
	return NULL;
}
FUNCTION(hashTable, has, bool, const char* val)
{
	string* key = NEW(string);
	string_append(key, val);
	unsigned int hash = string_hash(key) % _this->limit;
	while (VALID(_this->buckets[hash]))
	{
		if (string_areSame(_this->buckets[hash]->first, key))
		{
			DELETE(key);
			return true;
		}
		hash = (hash + 1) % _this->limit;
	}
	DELETE(key);
	return false;
}

#pragma endregion

#pragma region vector
CONSTRUCTOR(vector)
{
	vector* instance = calloc(1, sizeof(vector));
	instance->limit = 16;
	instance->num = 0;
	instance->data = calloc(instance->limit, sizeof(void*));
#if DEBUG_MEM
	printf("[vector] constructed\n");
#endif
	return instance;
}
DESTRUCTOR(vector)
{
	if (!VALID(instance)) return;
	for (unsigned int i = 0; i < instance->num; ++i)
		DELETE(instance->data[i]);
	free(instance->data);
#if DEBUG_MEM
	printf("[vector] destructed\n");
#endif
	return ___defaultDestructor(instance);
}
FUNCTION(vector, push_back, void, object* data)
{
	if (!VALID(_this) || !VALID(_this->data))
		return;
	_this->data[_this->num] = data;
	if (++_this->num == _this->limit)
		vector_grow(_this);
}
FUNCTION_NOARG(vector, grow, void)
{
	if (!VALID(_this) || !VALID(_this->data)) return;
	_this->limit <<= 1;
	object** nextData = calloc(_this->limit, sizeof(void*));
	memcpy(nextData, _this->data, sizeof(void*) * _this->num);
	free(_this->data);
	_this->data = nextData;
}
#pragma endregion

#pragma region pair

CONSTRUCTOR(bucket)
{
	bucket* instance = calloc(1, sizeof(bucket));
	instance->first = NULL;
	instance->second = NULL;
#if DEBUG_MEM
	printf("[pair] constructed\n");
#endif
	return instance;
}
DESTRUCTOR(bucket)
{
	if (!VALID(instance)) return;
	if (VALID(instance->first))
		DELETE(instance->first);
	if (VALID(instance->second))
		DELETE(instance->second);
#if DEBUG_MEM
	printf("[pair] destructed\n");
#endif
	return ___defaultDestructor(instance);
}
FUNCTION(bucket, make, void, const char* str, pair* value)
{
	if (VALID(_this->first))
		DELETE(_this->first);
	if (VALID(_this->second))
		DELETE(_this->second);
	string* first = NEW(string);
	pair* second = NEW(pair);
	string_append(first, str);
	second->first = value->first;
	second->second = value->second;
	_this->first = first; _this->second = second;
}

CONSTRUCTOR(pair)
{
	pair* instance = calloc(1, sizeof(bucket));
	instance->first = 0;
	instance->second = 0;
#if DEBUG_MEM
	printf("[pair] constructed\n");
#endif
	return instance;
}
DESTRUCTOR(pair)
{
	if (!VALID(instance)) return;
#if DEBUG_MEM
	printf("[pair] destructed\n");
#endif
	return ___defaultDestructor(instance);
}
FUNCTION(pair, make, void, unsigned int one, unsigned int two)
{
	_this->first = one; _this->second = two;
}
#pragma endregion

#pragma region strings
CONSTRUCTOR(string)
{
	string* instance = calloc(1, sizeof(string));
	instance->limit = 32; //inital max
	instance->c_str = calloc(instance->limit, sizeof(char));
	instance->length = 0;
#if DEBUG_MEM
	printf("[string] constructed\n");
#endif
	return instance;
}
DESTRUCTOR(string)
{
	if (!VALID(instance)) return;
	if (VALID(instance->c_str))
		free(instance->c_str);
#if DEBUG_MEM
	printf("[string] destructed\n");
#endif
	return ___defaultDestructor(instance);
}

FUNCTION(string, format, void, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	int length = vsnprintf(NULL, 0, format, args) + 1;
	va_end(args);
	va_start(args, format);
	char* buffer = calloc(length, sizeof(char));
	vsnprintf(buffer, length, format, args);
	va_end(args);
	string_append(_this, buffer);
	free(buffer);
}
STATIC_FUNCTION(string, make_and_format, string*, const char* format, ...)
{
	string* str = NEW(string);
	va_list args;
	va_start(args, format);
	int length = vsnprintf(NULL, 0, format, args) + 1;
	va_end(args);
	va_start(args, format);
	char* buffer = calloc(length, sizeof(char));
	vsnprintf(buffer, length, format, args);
	va_end(args);
	string_append(str, buffer);
	free(buffer);
	return str;
}
char* strtok_all(char* str, char const* delims)
{
	static char* src = NULL; char* loc = 0; char* ret = 0;
	if (VALID(str))
		src = str;
	if (!VALID(src))
		return NULL;
	loc = strpbrk(src, delims);
	ret = src;
	if (VALID(loc)) {
		loc[0] = 0; //convert delim to nullptr...

		src = ++loc; //move to next spot in string.
	}
	else if (VALID(src))
		src = NULL;
	return ret;
}

FUNCTION(string, split, void, vector* vect, const char* delim)
{
	if (!VALID(_this) || !VALID(_this->c_str) || !VALID(vect))
		return;
	char* data = calloc(_this->limit, sizeof(char)); //create a copy of the string
	strcpy(data, _this->c_str);
	char* token = strtok_all(data, delim);

	while (token != NULL) {
		/*
			vector takes ownership of this object. Normally I would use a copy-construct,
			but without adjusting a bunch of macros to include copy-constructors this isnt' trivial
			with a "generic" container. The vector will DELETE this when it's deleted.
		*/
		string* str = NEW(string);
		string_append(str, token);
		vector_push_back(vect, (object*)str);
		token = strtok_all(NULL, delim);
	}
	free(data);
}
FUNCTION(string, append, void, const char* data)
{
	unsigned int nextLength = _this->length + strlen(data);
	if (!VALID(data) || nextLength == _this->length)
		return;
	if (nextLength >= _this->limit)
	{
		unsigned int nextLimit = _this->limit;
		while (nextLength >= nextLimit)
			nextLimit <<= 1;
		_this->c_str = realloc(_this->c_str, nextLimit);
		memset(_this->c_str + _this->length, 0, nextLimit - _this->limit);
		_this->limit = nextLimit;
	}
	strcpy(_this->c_str + _this->length, data);
	_this->length = nextLength;
}
FUNCTION(string, append_int, void, const int data)
{
	int len = snprintf(NULL, 0, "%i", data) + 1;
	char* str = calloc(len, sizeof(char));
	snprintf(str, len + 1, "%i", data);
	string_append(_this, str);
	free(str);
}

FUNCTION(string, setEqual, string*, string* other)
{
	if (VALID(_this))
		DELETE(_this);
	return other;
}
FUNCTION(string, areSame, bool, string* other)
{
	if (!VALID(_this) || !VALID(other) || _this->length != other->length)
		return false;
	return strcmp(_this->c_str, other->c_str) == 0;
}
FUNCTION_NOARG(string, hash, unsigned int)
{
	if (!VALID(_this) || !VALID(_this->c_str))
		return 0;
	unsigned int hash = 0, accumulator = 0;
	while (_this->c_str[accumulator] != 0)
		hash += _this->c_str[accumulator++] & ~0x20;
	return hash;
}
#pragma endregion

#pragma region instruction
CONSTRUCTOR(instruction)
{
	instruction* instance = calloc(1, sizeof(instruction));
	instance->comment = NEW(string);
	instance->operand = NEW(string);
	instance->opcode = NEW(string);
	instance->symbol = NEW(string);
#if DEBUG_MEM
	printf("[instruction] constructed\n");
#endif
	return instance;
}
DESTRUCTOR(instruction)
{
	if (!VALID(instance)) return;
	if (VALID(instance->comment))
		DELETE(instance->comment);
	if (VALID(instance->operand))
		DELETE(instance->operand);
	if (VALID(instance->opcode))
		DELETE(instance->opcode);
	if (VALID(instance->symbol))
		DELETE(instance->symbol);
#if DEBUG_MEM
	printf("[instruction] destructed\n");
#endif
	return ___defaultDestructor(instance);
}
#pragma endregion

#pragma endregion
