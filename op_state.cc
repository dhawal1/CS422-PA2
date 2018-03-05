/* Author: Chris Wilkerson;   Created: Thu Aug 12 16:19:58 PDT 2004
 * Description: This file defines a structure for accessing the architectural
 * state.
*/

#include "op_state.h"
#include <cstdio>
#include <cstring>

using namespace std;

// op_record methods 
op_record_c::op_record_c(){
    init();
    osptr = 0;
}
op_record_c::~op_record_c(){
}
void op_record_c::init(){
    op_state_c *tmp_osptr = osptr;
    memset((char *)this, 0, sizeof(op_record_c));
    osptr = tmp_osptr;
}
void op_record_c::set_op_state(op_state_c *new_osptr){
    assert(!osptr);
    osptr = new_osptr;
}
// print information about the op_record
void op_record_c::debug_print(){
    printf("op-op(%2x)fp(%1x)lip(%8x)", op_class, is_fp, instruction_addr);
    printf("rf(%1x)wf(%1x)ec(%1x)ms(%1x)md(%1x)", writ_flg, read_flg, special_esc, has_mem_src, has_mem_dst);
    printf("s1(%3x)(%8x)s2(%3x)(%8x)d(%3x)(%8x)", src1, src1_val, src2, src2_val, dst, dst_val);
    if(has_mem_src){
        printf("src:va(%8x)ms1(%3x)ms2(%3x)ms3(%3x)", src_vaddr, mem_src1, mem_src2, mem_src3); 
    }
    if(has_mem_dst){
        printf("dst:va(%8x)", dst_vaddr);
    }
    printf("\n");
    printf("s1%10s - s2%10s", osptr->register_name(src1), osptr->register_name(src2));
    printf(" - d%10s\n", osptr->register_name(dst));
}


// methods for getting data values prevent the user from getting values before it is time
bool op_record_c::are_values_available(){
    uint time_available = (clock_time_set + osptr->inst_delay);
    if(time_available > osptr->get_clock()){
        return false;
    }
    return true;
}
// methods for instruction src 1
void op_record_c::set_src1_val(uint new_src1_val){
    src1_val = new_src1_val;
    clock_time_set = osptr->get_clock();
}
uint op_record_c::get_src1_val(){   
    if(!are_values_available()){
        uint time_available = (clock_time_set + osptr->inst_delay);
        printf("get_src1_val called before src1_val available %8d > %8d\n", time_available, osptr->get_clock());
        assert(0);
    }
    return src1_val;
}
// methods for instruction src 2
void op_record_c::set_src2_val(uint new_src2_val){
    src2_val = new_src2_val;
    clock_time_set = osptr->get_clock();
}
uint op_record_c::get_src2_val(){
    if(!are_values_available()){
        uint time_available = (clock_time_set + osptr->inst_delay);
        printf("get_src2_val called before src2_val available %8d > %8d\n", time_available, osptr->get_clock());
        assert(0);
    }
    return src2_val;
}
// methods for instruction dst 
void op_record_c::set_dst_val(uint new_dst_val){
    dst_val = new_dst_val;
    clock_time_set = osptr->get_clock();
}
uint op_record_c::get_dst_val(){
    if(!are_values_available()){
        uint time_available = (clock_time_set + osptr->inst_delay);
        printf("get_dst_val called before dst_val available %8d > %8d\n", time_available, osptr->get_clock());
        assert(0);
    }
    return dst_val;
}
// methods for instruction src addr 
void op_record_c::set_src_vaddr(uint new_src_vaddr){
    src_vaddr = new_src_vaddr;
    clock_time_set = osptr->get_clock();
}
uint op_record_c::get_src_vaddr(){
    if(!are_values_available()){
        uint time_available = (clock_time_set + osptr->inst_delay);
        printf("get_src_vaddr called before src_vaddr available %8d > %8d\n", time_available, osptr->get_clock());
        assert(0);
    }
    return src_vaddr;
}
// methods for instruction dst addr 
void op_record_c::set_dst_vaddr(uint new_dst_vaddr){
    dst_vaddr = new_dst_vaddr;
    clock_time_set = osptr->get_clock();
}
uint op_record_c::get_dst_vaddr(){
    if(!are_values_available()){
        uint time_available = (clock_time_set + osptr->inst_delay);
        printf("get_dst_vaddr called before dst_vaddr available %8d > %8d\n", time_available, osptr->get_clock());
        assert(0);
    }
    return dst_vaddr;
}

// op_state methods
op_state_c::op_state_c(){
    clock       = 0;

    num_regs    = g_num_regs;
    inst_delay  = g_inst_delay;
    num_ops     = g_num_ops;

    regs        = new uint[num_regs];
    regs_valid  = new bool[num_regs];
    for(uint i = 0; i < num_regs; i++){
        regs[i]       = 0;
        regs_valid[i] = false;
    }
    op_list_ptr = 0;
    op_list     = new op_record_c[num_ops];
}
op_state_c::~op_state_c(){
    delete [] regs;
    delete [] regs_valid;
    delete [] op_list;
}
void op_state_c::init(op_state_c *new_osptr){
    assert(this == new_osptr);
    for(uint i = 0; i < num_ops; i++){
        op_list[i].set_op_state(new_osptr);
    } 
}
const char* op_state_c::register_name(uint register_code){
    switch(register_code){
        //general purpose registers
    case REG_NUL: return "NUL";
    case REG_EAX: return "EAX";
    case REG_ECX: return "ECX";
    case REG_EDX: return "EDX";
    case REG_EBX: return "EBX";
    case REG_ESP: return "ESP";
    case REG_EBP: return "EBP";
    case REG_ESI: return "ESI";
    case REG_EDI: return "EDI";
        //segment registers
    case REG_ES : return "ES";
    case REG_CS : return "CS";
    case REG_SS : return "SS";
    case REG_DS : return "DS";
    case REG_FS : return "FS";
    case REG_GS : return "GS";
        //control registers
    case REG_CR0: return "CR0";
    case REG_CR1: return "CR1";
    case REG_CR2: return "CR2";
    case REG_CR3: return "CR3";
    case REG_CR4: return "CR4";
        //FP registers
    case REG_MM0: return "MM0";
    case REG_MM1: return "MM1";
    case REG_MM2: return "MM2";
    case REG_MM3: return "MM3";
    case REG_MM4: return "MM4";
    case REG_MM5: return "MM5";
    case REG_MM6: return "MM6";
    case REG_MM7: return "MM7";
        //SSE registers
    case REG_XMM0: return "XMM0";
    case REG_XMM1: return "XMM1";
    case REG_XMM2: return "XMM2";
    case REG_XMM3: return "XMM3";
    case REG_XMM4: return "XMM4";
    case REG_XMM5: return "XMM5";
    case REG_XMM6: return "XMM6";
    case REG_XMM7: return "XMM7";
    default:
        printf("reg undefined %8x\n", register_code);
        return "broken";
    }
    return "broken";
}

























