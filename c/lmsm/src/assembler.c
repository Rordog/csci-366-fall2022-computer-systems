//
// Created by carson on 11/15/21.
//

#include "assembler.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

char *ASM_ERROR_UNKNOWN_INSTRUCTION = "Unknown instruction";
char *ASM_ERROR_ARG_REQUIRED = "Argument Required";
char *ASM_ERROR_BAD_LABEL = "Bad Label";
char *ASM_ERROR_OUT_OF_RANGE = "Number is out of range";

//=========================================================
//  All the instructions available on the LMSM architecture
//=========================================================
const char *INSTRUCTIONS[25] =
        {"ADD", "SUB", "LDA", "STA", "BRA", "BRZ", "BRP", "INP", "OUT", "HLT", "COB", "DAT",
         "LDI",
         "CALL", "RET",
         "SPUSH", "SPUSHI", "SPOP", "SDUP", "SADD", "SSUB", "SMAX", "SMIN", "SMUL", "SDIV"
        };
const int INSTRUCTION_COUNT = 25;

//===================================================================
//  All the instructions that require an arg on the LMSM architecture
//===================================================================
const char *ARG_INSTRUCTIONS[11] =
        {"ADD", "SUB", "LDA", "STA", "BRA", "BRZ", "BRP", "DAT",
         "LDI",
         "CALL",
         "SPUSHI"
        };
const int ARG_INSTRUCTION_COUNT = 11;

//======================================================
// Constructors/Destructors
//======================================================

instruction * asm_make_instruction(char* type, char *label, char *label_reference, int value, instruction * predecessor) {
    instruction *new_instruction = calloc(1, sizeof(instruction));
    new_instruction->instruction = type;
    new_instruction->label = label;
    new_instruction->label_reference = label_reference;
    new_instruction->value = value;
    new_instruction->next = NULL;
    if (predecessor != NULL) {
        predecessor->next = new_instruction;
        new_instruction->offset = predecessor->offset + predecessor->slots;
    } else {
        new_instruction->offset = 0;
    }

    if(strcmp("SPUSHI", new_instruction->instruction) == 0){
        new_instruction->slots = 2;
    } else if (strcmp("CALL", new_instruction->instruction) == 0){
        new_instruction->slots = 3;
    } else {new_instruction->slots = 1;}

    return new_instruction;
}

void asm_delete_instruction(instruction *instruction) {
    if (instruction == NULL) {
        return;
    }
    asm_delete_instruction(instruction->next);
    free(instruction);
}

compilation_result * asm_make_compilation_result() {
    compilation_result *result = calloc(1, sizeof(compilation_result));
    return result;
}

void asm_delete_compilation_result(compilation_result *result) {
    asm_delete_instruction(result->root);
    free(result);
}

//======================================================
// Helpers
//======================================================
int asm_is_instruction(char * token) {
    for (int i = 0; i < INSTRUCTION_COUNT; ++i) {
        if (strcmp(token, INSTRUCTIONS[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int asm_instruction_requires_arg(char * token) {
    for (int i = 0; i < ARG_INSTRUCTION_COUNT; ++i) {
        if (strcmp(token, ARG_INSTRUCTIONS[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int asm_is_num(char * token){
    if (*token == '-') { // allow a leading negative
        token++;
    }
    while (*token != '\0') {
        if (*token < '0' || '9' < *token) {
            return 0;
        }
        token++;
    }
    return 1;
}

int asm_find_label(instruction *root, char *label) {
    // scan the linked list for the given label, return -1 if not found
    while(root != NULL){
        if(root->label != NULL && strcmp(root->label, label) == 0){
            return root->offset;
        }
        root = root->next;
    }
    return -1;
}


//======================================================
// Assembly Parsing/Scanning
//======================================================

void asm_parse_src(compilation_result * result, char * original_src){

    // copy over so strtok can mutate
    char * src = calloc(strlen(original_src) + 1, sizeof(char));
    strcat(src, original_src);

    instruction * last_instruction = NULL;
    instruction * current_instruction = NULL;

    char *token = strtok(src, " \n");
    while (token != NULL){
        char *label = NULL;
        if(!asm_is_instruction(token)) {
            label = token;
            token = strtok(NULL, " \n");
        }

        char *instruction = NULL;
        if(!asm_is_instruction(token)) {
            result->error = ASM_ERROR_UNKNOWN_INSTRUCTION;
            return;
        } else {
            instruction = token;
        }

        int value = 0;
        char *label_reference = NULL;
        if(asm_instruction_requires_arg(instruction)){
            token = strtok(NULL, " \n");
            if(asm_instruction_requires_arg(instruction) && token == NULL){
                result->error = ASM_ERROR_ARG_REQUIRED;
                return;
            }
            if(asm_is_num(token)){
                sscanf(token, "%d", &value);
            } else {
                label_reference = token;
            }
        }

        current_instruction = asm_make_instruction(instruction, label, label_reference, value, last_instruction);

        if (result->root == NULL){
            result->root = current_instruction;
        }

        last_instruction = current_instruction;
        // HINT last_instruction to create linked list w/current instruction
        token = strtok(NULL, " \n");

        if(value > 999){
            value = 999;
            result->error = ASM_ERROR_OUT_OF_RANGE;
        }
        if(value < -999){
            value = -999;
            result->error = ASM_ERROR_OUT_OF_RANGE;
        }
    }

}

//======================================================
// Machine Code Generation
//======================================================

void asm_gen_code_for_instruction(compilation_result  * result, instruction *instruction) {

    int value_for_instruction = instruction->value;
    if(instruction->label_reference){
        value_for_instruction = asm_find_label(result->root, instruction->label_reference);
        if(value_for_instruction == -1){
            result->error = ASM_ERROR_BAD_LABEL;
        }
    }
    if (strcmp("ADD", instruction->instruction) == 0) {
        result->code[instruction->offset] = 100 + value_for_instruction;
    } else if(strcmp("SUB", instruction->instruction) == 0){
        result->code[instruction->offset] = 200 + value_for_instruction;
    } else if(strcmp("STA", instruction->instruction) == 0){
        result->code[instruction->offset] = 300 + value_for_instruction;
    } else if(strcmp("LDI", instruction->instruction) == 0){
        result->code[instruction->offset] = 400 + value_for_instruction;
    } else if(strcmp("LDA", instruction->instruction) == 0){
        result->code[instruction->offset] = 500 + value_for_instruction;
    } else if(strcmp("BRA", instruction->instruction) == 0){
        result->code[instruction->offset] = 600 + value_for_instruction;
    } else if(strcmp("BRZ", instruction->instruction) == 0){
        result->code[instruction->offset] = 700 + value_for_instruction;
    } else if(strcmp("BRP", instruction->instruction) == 0){
        result->code[instruction->offset] = 800 + value_for_instruction;
    } else if(strcmp("INP", instruction->instruction) == 0){
        result->code[instruction->offset] = 901;
    } else if(strcmp("OUT", instruction->instruction) == 0){
        result->code[instruction->offset] = 902;
    } else if(strcmp("DAT", instruction->instruction) == 0){
        result->code[instruction->offset] = value_for_instruction;
    } else if(strcmp("CALL", instruction->instruction) == 0){
        result->code[instruction->offset] = 920;
        result->code[instruction->offset+1] = 400 + value_for_instruction;
        result->code[instruction->offset+2] = 910;
    } else if(strcmp("RET", instruction->instruction) == 0){
        result->code[instruction->offset] = 911;
    } else if(strcmp("SPUSH", instruction->instruction) == 0){
        result->code[instruction->offset] = 920;
    } else if(strcmp("SPUSHI", instruction->instruction) == 0){
        result->code[instruction->offset] = 920;
        result->code[instruction->offset+1] = 400 + value_for_instruction;
    } else if(strcmp("SPOP", instruction->instruction) == 0){
        result->code[instruction->offset] = 921;
    } else if(strcmp("SDUP", instruction->instruction) == 0){
        result->code[instruction->offset] = 922;
    } else if(strcmp("SADD", instruction->instruction) == 0){
        result->code[instruction->offset] = 923;
    } else if(strcmp("SSUB", instruction->instruction) == 0){
        result->code[instruction->offset] = 924;
    } else if(strcmp("SMAX", instruction->instruction) == 0){
        result->code[instruction->offset] = 925;
    } else if(strcmp("SMIN", instruction->instruction) == 0){
        result->code[instruction->offset] = 926;
    } else if(strcmp("SMUL", instruction->instruction) == 0){
        result->code[instruction->offset] = 927;
    } else if(strcmp("SDIV", instruction->instruction) == 0) {
        result->code[instruction->offset] = 928;
    } else if(strcmp("HLT", instruction->instruction) == 0){
        result->code[instruction->offset] = 000;
    } else if(strcmp("COB", instruction->instruction) == 0){
        result->code[instruction->offset] = 000;
    } else {
        result->error = ASM_ERROR_UNKNOWN_INSTRUCTION;
    }

}

void asm_gen_code(compilation_result * result) {
    instruction * current = result->root;
    while (current != NULL) {
        asm_gen_code_for_instruction(result, current);
        current = current->next;
    }
}

//======================================================
// Main API
//======================================================

compilation_result * asm_assemble(char *src) {
    compilation_result * result = asm_make_compilation_result();
    asm_parse_src(result, src);
    asm_gen_code(result);
    return result;
}
