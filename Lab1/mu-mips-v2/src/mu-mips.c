#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {
    printf("------------------------------------------------------------------\n\n");
    printf("\t**********MU-MIPS Help MENU**********\n\n");
    printf("sim\t-- simulate program to completion \n");
    printf("run <n>\t-- simulate program for <n> instructions\n");
    printf("rdump\t-- dump register values\n");
    printf("reset\t-- clears all registers/memory and re-loads the program\n");
    printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
    printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
    printf("high <val>\t-- set the HI register to <val>\n");
    printf("low <val>\t-- set the LO register to <val>\n");
    printf("print\t-- print the program loaded into memory\n");
    printf("?\t-- display help menu\n");
    printf("quit\t-- exit the simulator\n\n");
    printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
    int i;
    for (i = 0; i < NUM_MEM_REGION; i++) {
        if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
            uint32_t offset = address - MEM_REGIONS[i].begin;
            return (MEM_REGIONS[i].mem[offset+3] << 24) |
            (MEM_REGIONS[i].mem[offset+2] << 16) |
            (MEM_REGIONS[i].mem[offset+1] <<  8) |
            (MEM_REGIONS[i].mem[offset+0] <<  0);
        }
    }
    return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
    int i;
    uint32_t offset;
    for (i = 0; i < NUM_MEM_REGION; i++) {
        if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
            offset = address - MEM_REGIONS[i].begin;
            
            MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
            MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
            MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
            MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
        }
    }
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {
    handle_instruction();
    CURRENT_STATE = NEXT_STATE;
    INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {
    
    if (RUN_FLAG == FALSE) {
        printf("Simulation Stopped\n\n");
        return;
    }
    
    printf("Running simulator for %d cycles...\n\n", num_cycles);
    int i;
    for (i = 0; i < num_cycles; i++) {
        if (RUN_FLAG == FALSE) {
            printf("Simulation Stopped.\n\n");
            break;
        }
        cycle();
    }
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {
    if (RUN_FLAG == FALSE) {
        printf("Simulation Stopped.\n\n");
        return;
    }
    
    printf("Simulation Started...\n\n");
    while (RUN_FLAG){
        cycle();
    }
    printf("Simulation Finished.\n\n");
}

/***************************************************************/
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {
    uint32_t address;
    
    printf("-------------------------------------------------------------\n");
    printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
    printf("-------------------------------------------------------------\n");
    printf("\t[Address in Hex (Dec) ]\t[Value]\n");
    for (address = start; address <= stop; address += 4){
        printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
    }
    printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */
/***************************************************************/
void rdump() {
    int i;
    printf("-------------------------------------\n");
    printf("Dumping Register Content\n");
    printf("-------------------------------------\n");
    printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
    printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
    printf("-------------------------------------\n");
    printf("[Register]\t[Value]\n");
    printf("-------------------------------------\n");
    for (i = 0; i < MIPS_REGS; i++){
        printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
    }
    printf("-------------------------------------\n");
    printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
    printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
    printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */
/***************************************************************/
void handle_command() {
    char buffer[20];
    uint32_t start, stop, cycles;
    uint32_t register_no;
    int register_value;
    int hi_reg_value, lo_reg_value;
    
    printf("MU-MIPS SIM:> ");
    
    if (scanf("%s", buffer) == EOF){
        exit(0);
    }
    
    switch(buffer[0]) {
        case 'S':
        case 's':
            runAll();
            break;
        case 'M':
        case 'm':
            if (scanf("%x %x", &start, &stop) != 2){
                break;
            }
            mdump(start, stop);
            break;
        case '?':
            help();
            break;
        case 'Q':
        case 'q':
            printf("**************************\n");
            printf("Exiting MU-MIPS! Good Bye...\n");
            printf("**************************\n");
            exit(0);
        case 'R':
        case 'r':
            if (buffer[1] == 'd' || buffer[1] == 'D'){
                rdump();
            }else if(buffer[1] == 'e' || buffer[1] == 'E'){
                reset();
            }
            else {
                if (scanf("%d", &cycles) != 1) {
                    break;
                }
                run(cycles);
            }
            break;
        case 'I':
        case 'i':
            if (scanf("%u %i", &register_no, &register_value) != 2){
                break;
            }
            CURRENT_STATE.REGS[register_no] = register_value;
            NEXT_STATE.REGS[register_no] = register_value;
            break;
        case 'H':
        case 'h':
            if (scanf("%i", &hi_reg_value) != 1){
                break;
            }
            CURRENT_STATE.HI = hi_reg_value;
            NEXT_STATE.HI = hi_reg_value;
            break;
        case 'L':
        case 'l':
            if (scanf("%i", &lo_reg_value) != 1){
                break;
            }
            CURRENT_STATE.LO = lo_reg_value;
            NEXT_STATE.LO = lo_reg_value;
            break;
        case 'P':
        case 'p':
            print_program();
            break;
        default:
            printf("Invalid Command.\n");
            break;
    }
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {
    int i;
    /*reset registers*/
    for (i = 0; i < MIPS_REGS; i++){
        CURRENT_STATE.REGS[i] = 0;
    }
    CURRENT_STATE.HI = 0;
    CURRENT_STATE.LO = 0;
    
    for (i = 0; i < NUM_MEM_REGION; i++) {
        uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
        memset(MEM_REGIONS[i].mem, 0, region_size);
    }
    
    /*load program*/
    load_program();
    
    /*reset PC*/
    INSTRUCTION_COUNT = 0;
    CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
    NEXT_STATE = CURRENT_STATE;
    RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {
    int i;
    for (i = 0; i < NUM_MEM_REGION; i++) {
        uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
        MEM_REGIONS[i].mem = malloc(region_size);
        memset(MEM_REGIONS[i].mem, 0, region_size);
    }
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {
    FILE * fp;
    int i, word;
    uint32_t address;
    
    /* Open program file. */
    fp = fopen(prog_file, "r");
    if (fp == NULL) {
        printf("Error: Can't open program file %s\n", prog_file);
        exit(-1);
    }
    
    /* Read in the program. */
    
    i = 0;
    while( fscanf(fp, "%x\n", &word) != EOF ) {
        address = MEM_TEXT_BEGIN + i;
        mem_write_32(address, word);
        printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
        i += 4;
    }
    PROGRAM_SIZE = i/4;
    printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
    fclose(fp);
}

/************************************************************/
/* decode and execute instruction                                                                     */
/************************************************************/
void handle_instruction()
{
    /*IMPLEMENT THIS*/
    /* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
    uint32_t Op_Code_Special = 0;
    uint32_t op = 0;
    uint32_t rs = 0;
    uint32_t rt = 0;
    uint32_t rd = 0;
    uint32_t sa = 0;
    uint32_t immediate = 0;
    uint32_t base = 0;
    uint32_t mem_location = 0;
    uint32_t temp = 0;
    uint32_t target = 0;
    printf("Instruction: %x\n",instruction);
    if((instruction | 0x03ffffff) == 0x03ffffff){
        Op_Code_Special = instruction & 0x0000003f;
        
        switch(Op_Code_Special){
            case 0x00000020: case 0x00000021: //ADD, ADDU
            rs = instruction & 0x03E00000;
            rs = rs >> 21;
            rt = instruction & 0x001F0000;
            rt = rt >> 16;
            rd = instruction & 0x0000F800;
            rd = rd >> 11;
            NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
            NEXT_STATE.REGS[2] = 0x0A;
            printf("ADD/ADDU, %x\n\n",NEXT_STATE.REGS[rd]);
            
            break;

            case 0x00000022: case 0x00000023: //SUB, SUBU
			rs = instruction & 0x03E00000;
			rs = rs >> 21;
			rt = instruction & 0x001F0000;
			rt = rt >> 16;
			rd = instruction & 0x0000F800;
			rd = rd >> 11;
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
			NEXT_STATE.REGS[2] = 0x0A;
			printf("SUB, %x\n\n",NEXT_STATE.REGS[rd]);
			break;

			case 0x00000018: case 0x00000019: //MULT, MULTU
			rs = instruction & 0x03E00000;
			rs = rs >> 21;
			rt = instruction & 0x001F0000;
			rt = rt >> 16;
            temp = CURRENT_STATE.REGS[rs] * CURRENT_STATE.REGS[rt];
            NEXT_STATE.HI = temp & 0xFFFF0000;
            NEXT_STATE.LO = temp & 0x0000FFFF;
            NEXT_STATE.REGS[2] = 0x0A;
			printf("MULT/MULTU, Hi: %x LO: %x\n\n",NEXT_STATE.HI, NEXT_STATE.LO);
			break;

            case 0x0000001A: case 0x00000001B: //DIV, DIVU
            rs = instruction & 0x03E00000;
			rs = rs >> 21;
			rt = instruction & 0x001F0000;
			rt = rt >> 16;
            if(CURRENT_STATE.REGS[rt] != 0x0000){
                NEXT_STATE.HI = CURRENT_STATE.REGS[rs] % CURRENT_STATE.REGS[rt];
                NEXT_STATE.LO = CURRENT_STATE.REGS[rs] / CURRENT_STATE.REGS[rt];
            }
            else{
                NEXT_STATE.HI = NULL;
                NEXT_STATE.LO = NULL;
            }
            NEXT_STATE.REGS[2] = 0x0A;
			printf("DIV/DIVU, Hi: %x LO: %x\n\n",NEXT_STATE.HI, NEXT_STATE.LO);
            break;

			case 0x00000024: //AND
			rs = instruction & 0x03E00000;
			rs = rs >> 21;
			rt = instruction & 0x001F0000;
			rt = rt >> 16;
			rd = instruction & 0x0000F800;
			rd = rd >> 11;
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] & CURRENT_STATE.REGS[rt];
			NEXT_STATE.REGS[2] = 0x0A;
			printf("AND, %x\n\n",NEXT_STATE.REGS[rd]);
			break;

			case 0x00000025: //OR
			rs = instruction & 0x03E00000;
			rs = rs >> 21;
			rt = instruction & 0x001F0000;
			rt = rt >> 16;
			rd = instruction & 0x0000F800;
			rd = rd >> 11;
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt];
			NEXT_STATE.REGS[2] = 0x0A;
			printf("OR, %x\n\n",NEXT_STATE.REGS[rd]);
			break;

			case 0x00000026: //XOR
			rs = instruction & 0x03E00000;
			rs = rs >> 21;
			rt = instruction & 0x001F0000;
			rt = rt >> 16;
			rd = instruction & 0x0000F800;
			rd = rd >> 11;
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] ^ CURRENT_STATE.REGS[rt];
			NEXT_STATE.REGS[2] = 0x0A;
			printf("XOR, %x\n",NEXT_STATE.REGS[rd]);
			break;

			case 0x00000027: //NOR
			rs = instruction & 0x03E00000;
			rs = rs >> 21;
			rt = instruction & 0x001F0000;
			rt = rt >> 16;
			rd = instruction & 0x0000F800;
			rd = rd >> 11;
			NEXT_STATE.REGS[rd] = ~ (CURRENT_STATE.REGS[rs] ^ CURRENT_STATE.REGS[rt]);
			NEXT_STATE.REGS[2] = 0x0A;
			printf("XOR, %x\n\n",NEXT_STATE.REGS[rd]);
			break;

            case 0x0000002A: //SLT
            rs = instruction & 0x03E00000;
			rs = rs >> 21;
			rt = instruction & 0x001F0000;
			rt = rt >> 16;
			rd = instruction & 0x0000F800;
			rd = rd >> 11;
            if(CURRENT_STATE.REGS[rs] < CURRENT_STATE.REGS[rt]){
                NEXT_STATE.REGS[rd] = 0x01;
            }
            else{
                NEXT_STATE.REGS[rd] = 0x00;
            }
            NEXT_STATE.REGS[2] = 0x0A;
			printf("SLT, %x\n",NEXT_STATE.REGS[rd]);
            break;

            case 0x00000000: //SLL
            rt = instruction & 0x001F0000;
			rt = rt >> 16;
			rd = instruction & 0x0000F800;
			rd = rd >> 11;
            sa = instruction & 0x000007C0;
            sa = sa >> 6;
            NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] << sa;
            NEXT_STATE.REGS[2] = 0x0A;
			printf("SLL, %x\n\n",NEXT_STATE.REGS[rd]);
            break;

            case 0x00000002: //SRL
            rt = instruction & 0x001F0000;
			rt = rt >> 16;
			rd = instruction & 0x0000F800;
			rd = rd >> 11;
            sa = instruction & 0x000007C0;
            sa = sa >> 6;
            NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> sa;
            NEXT_STATE.REGS[2] = 0x0A;
			printf("SRL, %x\n",NEXT_STATE.REGS[rd]);
            break;

            case 0x00000003: //SRA - ask about what is meant by sign extension
            rt = instruction & 0x001F0000;
			rt = rt >> 16;
			rd = instruction & 0x0000F800;
			rd = rd >> 11;
            sa = instruction & 0x000007C0;
            sa = sa >> 6;
            NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> sa;
            NEXT_STATE.REGS[2] = 0x0A;
			printf("SRA, %x\n\n",NEXT_STATE.REGS[rd]);
            break;

            case 0x00000010: //MFHI
            rd = instruction & 0x0000F800;
			rd = rd >> 11;
            NEXT_STATE.REGS[rd] = CURRENT_STATE.HI;
            NEXT_STATE.REGS[2] = 0x0A;
			printf("MFHI, %x\n\n",NEXT_STATE.REGS[rd]);
            break;

            case 0x00000012: //MFLO
            rd = instruction & 0x0000F800;
			rd = rd >> 11;
            NEXT_STATE.REGS[rd] = CURRENT_STATE.LO;
            NEXT_STATE.REGS[2] = 0x0A;
			printf("MFLO, %x\n\n",NEXT_STATE.REGS[rd]);
            break;

            case 0x00000011: //MTHI
            rs = instruction & 0x03E00000;
			rs = rs >> 21;
            NEXT_STATE.HI = CURRENT_STATE.REGS[rs];
            NEXT_STATE.REGS[2] = 0x0A;
			printf("MTHI, %x\n\n",NEXT_STATE.HI);
            break;

            case 0x00000013: //MTLO
            rs = instruction & 0x03E00000;
			rs = rs >> 21;
            NEXT_STATE.LO = CURRENT_STATE.REGS[rs];
            NEXT_STATE.REGS[2] = 0x0A;
			printf("MTLO, %x\n\n",NEXT_STATE.LO);
            break;

            case 0x00000008: //JR
            rs = instruction & 0x03E00000;
			rs = rs >> 21;
            NEXT_STATE.PC = CURRENT_STATE.REGS[rs];
            NEXT_STATE.REGS[2] = 0x0A;
			printf("JR, %x\n\n",NEXT_STATE.PC);
            break;



            case 0x00000009: //JALR
            rs = instruction & 0x03E00000;
			rs = rs >> 21;
			rd = instruction & 0x0000F800;
			rd = rd >> 11;
            NEXT_STATE.REGS[rd] = CURRENT_STATE.PC + 8;
            NEXT_STATE.PC = CURRENT_STATE.REGS[rs];
            NEXT_STATE.REGS[2] = 0x0A;
			printf("JALR, %x\n\n",NEXT_STATE.PC);
            break;

			case 0x0000000C:  //SYSTEMCALL
			if(CURRENT_STATE.REGS[2] == 0x0A)
			{
				RUN_FLAG = FALSE;
			}
			break;
                
        }
    }
    else{
        op = instruction & 0xFC000000;
        switch(op){
            case 0x3C000000: //LUI
            immediate = instruction & 0x0000FFFF;
            immediate = immediate << 16;
            rt = instruction & 0x001F0000;
            rt = rt >> 16;
            NEXT_STATE.REGS[rt] = immediate;
            NEXT_STATE.REGS[2] = 0x0A;
            printf("LUI, %x\n\n",NEXT_STATE.REGS[rt]);
            break;
                
            case 0x24000000: case 0x20000000: //ADDI, ADDIU
            immediate = instruction & 0x0000FFFF;
            rt = instruction & 0x001F0000;
            rt = rt >> 16;
            rs = instruction & 0x03E00000;
            rs = rs >> 21;
            NEXT_STATE.REGS[rt] = immediate + CURRENT_STATE.REGS[rs];
            printf("ADDIU, %x\n\n",NEXT_STATE.REGS[rt]);
            NEXT_STATE.REGS[2] = 0x0A;
            break;
                
            case 0x8C000000: //LW
            base = instruction & 0x03E00000;
            base = base >> 21;
            immediate = instruction & 0x0000FFFF;
            rt = instruction & 0x001F0000;
            rt = rt >> 16;
            mem_location = CURRENT_STATE.REGS[base] + immediate;
            mem_location = mem_location | 0x00010000;
            NEXT_STATE.REGS[rt] = mem_read_32(mem_location);
            printf("LW, %x\n\n",NEXT_STATE.REGS[rt]);
            NEXT_STATE.REGS[2] = 0x0A;
            break;

            case 0xAC000000: //SW
			base = instruction & 0x03E00000;
			base = base >> 21;
			rt = instruction & 0x001F0000;
			rt = rt >> 16;
			immediate = instruction & 0x0000FFFF;
			mem_location = CURRENT_STATE.REGS[base] + immediate;
            mem_location = mem_location | 0x00010000;
			mem_write_32(mem_location, CURRENT_STATE.REGS[rt]);
			printf("SW, %x\n\n",CURRENT_STATE.REGS[rt]);
			NEXT_STATE.REGS[2] = 0x0A;
			break;

            case 0x30000000: //ANDI
            immediate = instruction & 0x0000FFFF;
            rs = instruction & 0x03E00000;
            rs = rs >> 21;
            rt = instruction & 0x001F0000;
			rt = rt >> 16;
            NEXT_STATE.REGS[rt] = immediate & CURRENT_STATE.REGS[rt];
            printf("ANDI, %x\n\n",CURRENT_STATE.REGS[rt]);
            NEXT_STATE.REGS[2] = 0x0A;
            break;

            case 0x34000000: //ORI
            immediate = instruction & 0x0000FFFF;
            rs = instruction & 0x03E00000;
            rs = rs >> 21;
            rt = instruction & 0x001F0000;
			rt = rt >> 16;
            NEXT_STATE.REGS[rt] = immediate | CURRENT_STATE.REGS[rt];
            printf("ORI, %x\n\n",CURRENT_STATE.REGS[rt]);
            NEXT_STATE.REGS[2] = 0x0A;
            break;

            case 0x38000000: //XORI
            immediate = instruction & 0x0000FFFF;
            rs = instruction & 0x03E00000;
            rs = rs >> 21;
            rt = instruction & 0x001F0000;
			rt = rt >> 16;
            NEXT_STATE.REGS[rt] = immediate ^ CURRENT_STATE.REGS[rt];
            printf("XORI, %x\n\n",CURRENT_STATE.REGS[rt]);
            NEXT_STATE.REGS[2] = 0x0A;
            break;

            case 0x28000000: //SLTI
            immediate = instruction & 0x0000FFFF;
            rs = instruction & 0x03E00000;
            rs = rs >> 21;
            rt = instruction & 0x001F0000;
			rt = rt >> 16;
            if(CURRENT_STATE.REGS[rs] < immediate){
                NEXT_STATE.REGS[rt] = 0x01;
            }
            else{
                NEXT_STATE.REGS[rt] = 0x00;
            }
            printf("SLTI, %x\n\n",CURRENT_STATE.REGS[rt]);
            NEXT_STATE.REGS[2] = 0x0A;
            break;

            case 0xA4000000: //SH
            base = instruction & 0x03E00000;
			base = base >> 21;
			rt = instruction & 0x001F0000;
			rt = rt >> 16;
			immediate = instruction & 0x0000FFFF;
			mem_location = CURRENT_STATE.REGS[base] + immediate;
            mem_location = mem_location | 0x00010000;
			mem_write_32(mem_location, (CURRENT_STATE.REGS[rt] & 0x0000FFFF));
			printf("SH, %x\n\n",CURRENT_STATE.REGS[rt]);
			NEXT_STATE.REGS[2] = 0x0A;
            break;

            case 0xA0000000: //SB
            base = instruction & 0x03E00000;
			base = base >> 21;
			rt = instruction & 0x001F0000;
			rt = rt >> 16;
			immediate = instruction & 0x0000FFFF;
			mem_location = CURRENT_STATE.REGS[base] + immediate;
            mem_location = mem_location | 0x00010000;
			mem_write_32(mem_location, (CURRENT_STATE.REGS[rt] & 0x000000FF));
			printf("SB, %x\n\n",CURRENT_STATE.REGS[rt]);
			NEXT_STATE.REGS[2] = 0x0A;
            break;

            case 0x08000000: //J
            target = instruction & 0x03FFFFFF;
            target = target << 2;
            temp = CURRENT_STATE.PC & 0xF0000000;
            NEXT_STATE.PC = temp + target;
            NEXT_STATE.REGS[2] = 0x0A;
			printf("J, %x\n\n",NEXT_STATE.PC);
            break;

            case 0x0C000000: //JAL
            target = instruction & 0x03FFFFFF;
            target = target << 2;
            NEXT_STATE.REGS[31] = CURRENT_STATE.PC + 8;
            temp = CURRENT_STATE.PC & 0xF0000000;
            NEXT_STATE.PC = temp + target;
            NEXT_STATE.REGS[2] = 0x0A;
			printf("JAL, %x\n\n",NEXT_STATE.PC);
            break;

            case 


        }
        
    }
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    
    
    
}


/************************************************************/
/* Initialize Memory                                                                                                    */
/************************************************************/
void initialize() {
    init_memory();
    CURRENT_STATE.PC = MEM_TEXT_BEGIN;
    NEXT_STATE = CURRENT_STATE;
    RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */
/************************************************************/
void print_program(){
    int i;
    uint32_t addr;
    
    for(i=0; i<PROGRAM_SIZE; i++){
        addr = MEM_TEXT_BEGIN + (i*4);
        printf("[0x%x]\t", addr);
        print_instruction(addr);
    }
}

/************************************************************/
/* Print the instruction at given memory address (in MIPS assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){
    /*IMPLEMENT THIS*/
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {
    printf("\n**************************\n");
    printf("Welcome to MU-MIPS SIM...\n");
    printf("**************************\n\n");
    
    if (argc < 2) {
        printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
        exit(1);
    }
    
    strcpy(prog_file, argv[1]);
    initialize();
    load_program();
    help();
    while (1){
        handle_command();
    }
    return 0;
}
