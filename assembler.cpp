#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma warning(disable : 4996)

const char* recognisedArchitectures[2] = {
    "s8", "s16"
};

struct symbol8
{
    const char* symbol = NULL;
    char value = 0x00;
};

struct listNode
{
    void* data = NULL;
    listNode* next = NULL;
};

int add_list_element(listNode* startPos, void* data);
int generate_symbol_table_8(listNode* startPos);
void print_symbol_table_8(listNode* startPos);
int parse_integer(char* string, int* integer);

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("Assembly failed. Incorrect number of arguments.\n");
        return -1;
    }

    char validArch = 0;
    int architecture = 0;
    for (int arch = 0; arch < sizeof(recognisedArchitectures)/sizeof(const char*); arch++)
    {
        if (strcmp(argv[2], recognisedArchitectures[arch]) == 0)
        {
            validArch = 1;
            architecture = arch;
        }
    }
    if (validArch == 0)
    {
        printf("Assembly failed. Unrecognised architecture.\n");
        return -1;
    }

    FILE* file = fopen(argv[1], "r");
    if (file == NULL)
    {
        printf("Assembly failed. File not found.\n");
        return -1;
    }

    // validate whether the file name is a .asm and get the bit before it for the output file

    int index = 0;
    int lastDot = 0;
    while (argv[1][index] != '\0')
    {
        if (argv[1][index] == '.')
        {
            lastDot = index;
        }

        index++;
    }

    char* fileExtension = (char*)malloc(sizeof(char) * (index - lastDot));
    if (fileExtension == NULL)
    {
        printf("Assembly failed. Out of memory.\n");
        return -1;
    }
    for (int feIndex = 0; feIndex < (index - lastDot); feIndex++)
    {
        fileExtension[feIndex] = argv[1][lastDot + feIndex + 1];
    }
    if (strcmp(fileExtension, "asm") != 0)
    {
        printf("Assembly failed. Invalid file extension.\n");
        return -1;
    }

    char* fileName = (char*)malloc(sizeof(char) * lastDot);
    if (fileName == NULL)
    {
        printf("Assembly failed. Out of memory.\n");
        return -1;
    }
    for (int fileNameIndex = 0; fileNameIndex < index + 1; fileNameIndex++)
    {
        fileName[fileNameIndex] = argv[1][fileNameIndex];
    }
    fileName[index - 3] = 'p';
    fileName[index - 2] = 'r';
    fileName[index - 1] = 'o';

    printf("Assembling file %s to %s architecture.\n", argv[1], argv[2]);

    // create symbol table for the architecture

    listNode* symbolTable = (listNode*)malloc(sizeof(listNode*));
    symbolTable->data = NULL;
    symbolTable->next = NULL;
    switch (architecture)
    {
    case 0:
        // s8 architecture
        generate_symbol_table_8(symbolTable);
        break;
    }

    printf("Created symbol table for %s architecture.\n", argv[2]);

    // create new array to store the data in

    // // get the file size

    int fileSize = 0;
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // // define array

    char* assembly = (char*)malloc(fileSize * sizeof(char));
    if (assembly == NULL)
    {
        printf("Assembly failed. Out of memory.\n");
        return -1;
    }

    // remove comments, unnecessary whitespace etc.

    int character = 0;
    char isComment = 0;
    while (character < fileSize)
    {
        int newChar = fgetc(file);
        if (newChar == EOF)
        {
            // blank out anything past the end of the file.
            assembly[character] = (char)0x00;
            character++;
        }
        else if (newChar == 0x0a)
        {
            // if its a linebreak and the previous character isnt a linebreak then add the linebreak
            // if the previous character is a space then just overwrite the space
            if (character != 0 && assembly[character - 1] == ' ')
            {
                assembly[character - 1] = (char)newChar;
            }
            else if (character != 0 && assembly[character - 1] != 0x0a)
            {
                assembly[character] = (char)newChar;
                character++;
            }
            isComment = 0;
        }
        else if (newChar == ' ' && isComment == 0)
        {
            // if its a space and there isnt a space or linebreak before then add the space
            if (character != 0 && assembly[character - 1] != ' ')
            {
                assembly[character] = (char)newChar;
                character++;
            }
        }
        else if (newChar == '#')
        {
            // start a new comment if the hash is there, dont add it to the buffer
            isComment = 1;
        }
        else if (!isComment)
        {
            // if its not a comment add the character
            assembly[character] = (char)newChar;
            character++;
        }
    }

    printf("Removed comments and whitespace.\n");

    // add constants and labels into the symbol table
    
    int startOfLine = 0;
    int endOfLine = 0;
    int lineNumber = 1;
    int byteIndex = 0;
    // loop through each character in the array
    for (int index = 0; index < fileSize; index++)
    {
        // if its a linebreak or the end of the string
        if (assembly[index] == 0x0a || assembly[index] == '\0')
        {
            endOfLine = index;

            // check if the line is long enough to fit the const or label keyword
            if (endOfLine - startOfLine >= 6)
            {
                // if the first word is const
                if (assembly[startOfLine + 0] == 'c' &&
                    assembly[startOfLine + 1] == 'o' &&
                    assembly[startOfLine + 2] == 'n' &&
                    assembly[startOfLine + 3] == 's' &&
                    assembly[startOfLine + 4] == 't' &&
                    assembly[startOfLine + 5] == ' ')
                {
                    // make an array where the words will be stored
                    char** words = (char**)malloc(sizeof(char*) * 2);
                    if (words == NULL)
                    {
                        printf("Assembly failed. Out of memory.");
                        return -1;
                    }

                    // find the words in the line
                    int wordStart = 0;
                    int wordEnd = 0;
                    int wordCount = 0;
                    char inWord = 0;
                    for (int wordIndex = startOfLine + 6; wordIndex < endOfLine + 1; wordIndex++)
                    {
                        if (assembly[wordIndex] != ' ' && inWord == 0)
                        {
                            wordStart = wordIndex;
                            inWord = 1;
                        }
                        else if ((assembly[wordIndex] == ' ' || assembly[wordIndex] == 0x0a) && inWord == 1)
                        {
                            wordEnd = wordIndex;
                            inWord = 0;
                            if (wordCount < 2)
                            {
                                char* word = (char*)malloc(sizeof(char) * (wordEnd - wordStart + 1));
                                for (int letter = 0; letter < wordEnd - wordStart; letter++)
                                {
                                    word[letter] = assembly[wordStart + letter];
                                }
                                word[wordEnd - wordStart] = '\0';
                                words[wordCount] = word;
                                wordCount++;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                    // if 2 words were found
                    if (wordCount == 2)
                    {
                        // parse the second word

                        int intValue = 0;
                        if (parse_integer(words[1], &intValue) != 0)
                        {
                            printf("Assembly failed. Invalid value on line %d\n.", lineNumber);
                        }
                        char value = (char)intValue;

                        // define a new symbol

                        symbol8* newSymbol = (symbol8*)malloc(sizeof(symbol8));
                        if (newSymbol == NULL)
                        {
                            printf("Assembly failed. Out of memory.");
                            return -1;
                        }
                        char* word = words[0];
                        newSymbol->symbol = word;
                        newSymbol->value = value;

                        // define a new list node
                        
                        add_list_element(symbolTable, newSymbol);
                    }
                    else
                    {
                        printf("Assembly failed. Not enough values on line %d to define a constant.\n", lineNumber);
                        return -1;
                    }
                    free(words);
                }
                else if (assembly[startOfLine + 0] == 'l' &&
                    assembly[startOfLine + 1] == 'a' &&
                    assembly[startOfLine + 2] == 'b' &&
                    assembly[startOfLine + 3] == 'e' &&
                    assembly[startOfLine + 4] == 'l' &&
                    assembly[startOfLine + 5] == ' ')
                {
                    // find the word in the line
                    int wordStart = 0;
                    int wordEnd = 0;
                    int wordCount = 0;
                    char inWord = 0;
                    char* word = 0;
                    for (int wordIndex = startOfLine + 6; wordIndex < endOfLine + 1; wordIndex++)
                    {
                        if (assembly[wordIndex] != ' ' && inWord == 0)
                        {
                            wordStart = wordIndex;
                            inWord = 1;
                        }
                        else if ((assembly[wordIndex] == ' ' || assembly[wordIndex] == 0x0a) && inWord == 1)
                        {
                            wordEnd = wordIndex;
                            inWord = 0;
                            if (wordCount < 2)
                            {
                                word = (char*)malloc(sizeof(char) * (wordEnd - wordStart + 1));
                                if (word == NULL)
                                {
                                    printf("Assembly failed. Out of memory.\n");
                                }
                                for (int letter = 0; letter < wordEnd - wordStart; letter++)
                                {
                                    word[letter] = assembly[wordStart + letter];
                                }
                                word[wordEnd - wordStart] = '\0';
                                wordCount++;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }

                    // define a new symbol

                    symbol8* newSymbol = (symbol8*)malloc(sizeof(symbol8));
                    if (newSymbol == NULL)
                    {
                        printf("Assembly failed. Out of memory.");
                        return -1;
                    }
                    newSymbol->symbol = word;
                    newSymbol->value = byteIndex + 2;

                    // define a new list node

                    add_list_element(symbolTable, newSymbol);
                }
                else
                {
                    byteIndex += 2;
                }
            }

            if (assembly[index] == '\0')
            {
                break;
            }
            startOfLine = index + 1;
            lineNumber++;
        }
    }

    printf("Added user-defined symbols to symbol table.\n");

    //print_symbol_table_8(symbolTable);

    // actually do the translation into machine code

    char program[256];
    char programIndex = 0;

    startOfLine = 0;
    endOfLine = 0;

    for (int character = 0; character < fileSize; character++)
    {
        if (assembly[character] == '\0' || assembly[character] == 0x0a)
        {
            // reached the end of a line
            
            endOfLine = character;
            char ignoreLine = 0;

            if (endOfLine - startOfLine >= 6)
            {
                // if the line defines a constant or label ignore it
                if (assembly[startOfLine + 0] == 'c' &&
                    assembly[startOfLine + 1] == 'o' &&
                    assembly[startOfLine + 2] == 'n' &&
                    assembly[startOfLine + 3] == 's' &&
                    assembly[startOfLine + 4] == 't' &&
                    assembly[startOfLine + 5] == ' ')
                {
                    ignoreLine = 1;
                }
                else if (assembly[startOfLine + 0] == 'l' &&
                    assembly[startOfLine + 1] == 'a' &&
                    assembly[startOfLine + 2] == 'b' &&
                    assembly[startOfLine + 3] == 'e' &&
                    assembly[startOfLine + 4] == 'l' &&
                    assembly[startOfLine + 5] == ' ')
                {
                    ignoreLine = 1;
                }
            }

            if (ignoreLine == 0)
            {
                // for each word, find the associated value in the symbol table and add it to the program

                int startOfWord = startOfLine;
                int endOfWord = startOfLine;

                for (int letter = startOfLine; letter <= endOfLine; letter++)
                {

                    if (assembly[letter] == ' ' || assembly[letter] == 0x0a || assembly[letter] == '\0')
                    {
                        // new word found

                        endOfWord = letter;

                        // copy it into a new array

                        char* word = (char*)malloc(sizeof(char) * (endOfWord - startOfWord + 1));
                        if (word == NULL)
                        {
                            printf("Assembly failed. Out of memory.");
                            return -1;
                        }

                        for (int wordIndex = 0; wordIndex < endOfWord - startOfWord; wordIndex++)
                        {
                            word[wordIndex] = assembly[startOfWord + wordIndex];
                        }
                        word[endOfWord - startOfWord] = '\0';

                        // compare it to anything in the symbol table

                        listNode* currentNode = symbolTable;
                        char foundNode = 0;
                        while (currentNode != NULL)
                        {
                            symbol8* symbol = (symbol8*)(currentNode->data);
                            if (symbol == NULL)
                            {
                                printf("Assembly failed. Invalid symbol table.\n");
                                return -1;
                            }
                            if (strcmp(symbol->symbol, word) == 0)
                            {
                                foundNode = 1;
                                program[programIndex] = symbol->value;
                                programIndex++;
                            }
                            
                            currentNode = currentNode->next;
                        }
                        if (foundNode == 0)
                        {
                            int intSymbol = 0;
                            if (parse_integer(word, &intSymbol) == 0)
                            {
                                program[programIndex] = (char)intSymbol;
                                programIndex++;
                            }
                            else
                            {
                                printf("Assembly failed. Symbol not found for word %s.\n", word);
                                return -1;
                            }
                        }

                        free(word);

                        if (assembly[letter] != '\0')
                        {
                            startOfWord = endOfWord + 1;
                        }
                    }
                }
            }

            if (assembly[character] != '\0')
            {
                startOfLine = endOfLine + 1;
            }
            else
            {
                break;
            }
        }
    }

    printf("Translated into machine code.\n");

    //printf("PROGRAM ENDS AT %02x\n", programIndex & 0xff);


    //// print the output

    //for (int character = 0; character < fileSize; character++)
    //{
    //    if (character % 16 == 0)
    //    {
    //        printf("\n");
    //    }
    //    printf("%02x ", assembly[character] & 0xff);
    //}
    //for (int character = 0; character < 256; character++)
    //{
    //    if (character % 16 == 0)
    //    {
    //        printf("\n");
    //    }
    //    printf("%02x ", program[character] & 0xff);
    //}
    //printf("\n");
    //print_symbol_table_8(symbolTable);
    //printf("\n");
    //printf("%s", assembly);

    free(assembly);
    fclose(file);

    // output program into the output file

    FILE* outputFile = fopen(fileName, "w");
    if (outputFile == NULL)
    {
        printf("Assembly failed. Could not create output file.\n");
        return -1;
    }

    fwrite(program, sizeof(char), programIndex, outputFile);

    fclose(outputFile);

    printf("Written to file %s.\n", fileName);

    return 0;
}

int add_list_element(listNode* startPos, void* data)
{
    listNode* currentNode = startPos;
    listNode* nextNode = currentNode->next;
    while (nextNode != NULL)
    {
        currentNode = nextNode;
        nextNode = currentNode->next;
    }
    listNode* newNode = (listNode*)malloc(sizeof(listNode));
    if (newNode == NULL)
    {
        printf("Assembly failed. Out of memory.");
        return -1;
    }
    newNode->data = data;
    newNode->next = NULL;
    currentNode->next = newNode;
    return 0;
}

int generate_symbol_table_8(listNode* startPos)
{ 
    // add opcodes

    symbol8* addSymbol = (symbol8*)malloc(sizeof(symbol8));
    if (addSymbol == NULL)
    {
        printf("Assembly failed. Out of memory.\n");
        return -1;
    }
    addSymbol->symbol = "add";
    addSymbol->value = 0x01;
    symbol8* notSymbol = (symbol8*)malloc(sizeof(symbol8));
    if (notSymbol == NULL)
    {
        printf("Assembly failed. Out of memory.\n");
        return -1;
    }
    notSymbol->symbol = "not";
    notSymbol->value = 0x02;
    symbol8* savSymbol = (symbol8*)malloc(sizeof(symbol8));
    if (savSymbol == NULL)
    {
        printf("Assembly failed. Out of memory.\n");
        return -1;
    }
    savSymbol->symbol = "sav";
    savSymbol->value = 0x04;
    symbol8* movSymbol = (symbol8*)malloc(sizeof(symbol8));
    if (movSymbol == NULL)
    {
        printf("Assembly failed. Out of memory.\n");
        return -1;
    }
    movSymbol->symbol = "mov";
    movSymbol->value = 0x08;
    symbol8* immSymbol = (symbol8*)malloc(sizeof(symbol8));
    if (immSymbol == NULL)
    {
        printf("Assembly failed. Out of memory.\n");
        return -1;
    }
    immSymbol->symbol = "imm";
    immSymbol->value = 0x10;
    symbol8* jmpSymbol = (symbol8*)malloc(sizeof(symbol8));
    if (jmpSymbol == NULL)
    {
        printf("Assembly failed. Out of memory.\n");
        return -1;
    }
    jmpSymbol->symbol = "jmp";
    jmpSymbol->value = 0x20;
    symbol8* inpSymbol = (symbol8*)malloc(sizeof(symbol8));
    if (inpSymbol == NULL)
    {
        printf("Assembly failed. Out of memory.\n");
        return -1;
    }
    inpSymbol->symbol = "inp";
    inpSymbol->value = 0x40;
    symbol8* outSymbol = (symbol8*)malloc(sizeof(symbol8));
    if (outSymbol == NULL)
    {
        printf("Assembly failed. Out of memory.\n");
        return -1;
    }
    outSymbol->symbol = "out";
    outSymbol->value = (char)0x80;

    startPos->data = addSymbol;
    add_list_element(startPos, notSymbol);
    add_list_element(startPos, savSymbol);
    add_list_element(startPos, movSymbol);
    add_list_element(startPos, immSymbol);
    add_list_element(startPos, jmpSymbol);
    add_list_element(startPos, inpSymbol);
    add_list_element(startPos, outSymbol);

    // add registers

    for (int regIndex = 0; regIndex < 8; regIndex++)
    {
        symbol8* regSymbol = (symbol8*)malloc(sizeof(symbol8));
        if (regSymbol == NULL)
        {
            printf("Assembly failed. Out of memory.\n");
            return -1;
        }
        char* name = (char*)malloc(sizeof(char) * 5);
        if (name == NULL)
        {
            printf("Assembly failed. Out of memory.\n");
            return -1;
        }
        name[0] = 'r';
        name[1] = 'e';
        name[2] = 'g';
        name[3] = 48 + regIndex;
        name[4] = '\0';
        regSymbol->symbol = (const char*)name;
        regSymbol->value = (char)0x01 << regIndex;

        add_list_element(startPos, regSymbol);
    }

    return 0;
}

void print_symbol_table_8(listNode* startPos)
{
    listNode* currentNode = startPos;
    while (currentNode != NULL)
    {
        symbol8* symbol = (symbol8*)(currentNode->data);
        printf("SYMBOL: %s, VALUE: 0x%02x\n", symbol->symbol, symbol->value & 0xff);

        currentNode = currentNode->next;
    }
}

int parse_integer(char* string, int* integer)
{
    // check if they are all digits
    int stringLength = 0;
    while (string[stringLength] != '\0')
    {
        if (string[stringLength] < 48 || string[stringLength] > 57)
        {
            return -1;
        }
        stringLength++;
    }
    for (int digit = 0; digit < stringLength; digit++)
    {
        int newBit = string[digit] - 48;
        for (int power = 0; power < stringLength - digit - 1; power++)
        {
            newBit *= 10;
        }
        *integer += newBit;
    }

    return 0;
}