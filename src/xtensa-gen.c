/*
    xtensa-gen.c -- Gadget searching for xtensa
    Copyright (C) 2014  Amat I. Cama
    This file is part of xrop.
    Xrop is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    Xrop is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 
*/

#include "../include/xrop.h"
#include "../include/common.h"
#include <string.h>
#include <stdio.h>

unsigned int xtensa_child_index_for_size(unsigned int inst_size) {
    if(inst_size < XTENSA_MIN_INSTR_SIZE || inst_size > XTENSA_MAX_INSTR_SIZE) {
        perror("invalid xtensa instruction size.");
        exit(-1);
    }
    return inst_size - XTENSA_MIN_INSTR_SIZE;
}



// char *, int
// Check if the given instruction is a gadget end sequence
int is_xtensa_end(insn_t *i){
    if(strstr(i->decoded_instrs, "ret") 
	|| strstr(i->decoded_instrs, "retw.n") 
	|| strstr(i->decoded_instrs, "ret.n") 
	|| strstr(i->decoded_instrs, "retw") 
	|| strstr(i->decoded_instrs, "jx") 
	|| strstr(i->decoded_instrs, "callx0")) {
    //if((strstr(i->decoded_instrs, "ret") && !strstr(i->decoded_instrs, "retw")) || strstr(i->decoded_instrs, "jx") || strstr(i->decoded_instrs, "callx0")) {
        return 1;
    }
    return 0;
}

// xtensa_node_t *, char *, char *, size_t, size_t, size_t, int, size_t, int
// Generate all the gadgets connected to the xtensa node
void get_children_xtensa(xtensa_node_t * currnode, char * begptr, char * rawbuf, unsigned long long lowervma, size_t bufsize, int bits, size_t depth, int endian){
    int i = 0;
    insn_t * it = NULL, * curr = NULL;
    unsigned long long rvma = 0;
    it = currnode->insn;

    // xtensa instructions are 2 or 3 bytes long
    for(i = XTENSA_MIN_INSTR_SIZE; i <= XTENSA_MAX_INSTR_SIZE && depth > 0; i++){
        char * nrawbuf = rawbuf - i;
        rvma = it->vma - i;
        if(nrawbuf < begptr) break;
        if(rvma < lowervma) break;
       

        curr = disassemble_one(rvma, nrawbuf, bufsize + i, ARCH_xtensa, bits, endian);
        if(is_branch(curr, ARCH_xtensa)) break;
        if(is_valid_instr(curr, ARCH_xtensa) && (curr->instr_size == i)){
            xtensa_node_t * tmpn = malloc(sizeof(xtensa_node_t));
            if(!tmpn){
                perror("malloc");
                exit(-1);
            }
            memset(tmpn, 0, sizeof(xtensa_node_t));
            tmpn->insn = curr;
            currnode->children[xtensa_child_index_for_size(i)] = tmpn;
            get_children_xtensa(tmpn, begptr, nrawbuf, lowervma, bufsize + i, bits, depth - 1, endian);
        }
    }
}

// xtensa_node_t *, insn_t *, size_t, int
// Recursively print the gadgets in the xtensa trie
void r_print_xtensa_gadgets_trie(xtensa_node_t * n, insn_t * path[], size_t depth, int pathlen, char ** re){
    int i = 0;
    xtensa_node_t * tmp = NULL;   
    int acc = 1;

    path[pathlen] = n->insn;
 
    if((int) depth < 0){
        return;
    }
    
    for(i = XTENSA_MIN_INSTR_SIZE; i <= XTENSA_MAX_INSTR_SIZE; i++){
        tmp = n->children[xtensa_child_index_for_size(i)];
        if(tmp){
            acc = 0;
            r_print_xtensa_gadgets_trie(tmp, path, depth - 1, pathlen + 1, re);
        }
    }

    if(acc){
        print_path(path, pathlen, NORM_INSTR, re);
    }

}

// xtensa_node_t *, size_t
// Print the gadgets in the xtensa trie
void print_xtensa_gadgets_trie(xtensa_node_t * n, size_t depth, char ** re){
    insn_t * path[MAX_GADGET_LEN] = {0};
    r_print_xtensa_gadgets_trie(n, path, depth, 0, re);
}

// Generate the xtensa gadgets in the given buffer
// Called First
// Will find a ret command
// Then, will call get_children_xtensa for the ret gadgets
gadget_list * generate_xtensa(unsigned long long vma, char * rawbuf, size_t size, int bits, int endian, size_t depth, char ** re){
    insn_t * it = NULL;
    unsigned long long  i = 0;
    unsigned long long rvma = 0;
    xtensa_node_t * retrootn = NULL;
   
    for(; i < size; i++){
        rvma = vma + i;
     
	it = disassemble_one(rvma, rawbuf + i, XTENSA_MAX_INSTR_SIZE, ARCH_xtensa, bits, endian);
        if(is_xtensa_end(it)) {
            retrootn = malloc(sizeof(xtensa_node_t));
            if(!retrootn){
                perror("malloc");
                exit(-1);
            }
            memset(retrootn, 0, sizeof(xtensa_node_t));

            retrootn->insn = it;
            get_children_xtensa(retrootn, rawbuf, rawbuf + i, vma, size - i, bits, depth, endian);

            print_xtensa_gadgets_trie(retrootn, depth, re);
        }
    }

    return NULL;
}
