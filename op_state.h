/* Author: Chris Wilkerson;   Created: Thu Aug 12 16:19:58 PDT 2004
 * Description: This file defines a structure for accessing the architectural
 * state.
*/

#ifndef OP_STATE_H_SEEN
#define OP_STATE_H_SEEN

#include <cassert>

typedef unsigned int uint;

// register types
const uint REG_NUL  = 0x0; 
const uint REG_EAX  = 0x1; 
const uint REG_ECX  = 0x2; 
const uint REG_EDX  = 0x3; 
const uint REG_EBX  = 0x4; 
const uint REG_ESP  = 0x5; 
const uint REG_EBP  = 0x6; 
const uint REG_ESI  = 0x7;
const uint REG_EDI  = 0x8;
                   
const uint REG_ES   = 0x45;
const uint REG_CS   = 0x46;
const uint REG_SS   = 0x47;
const uint REG_DS   = 0x48;
const uint REG_FS   = 0x49;
const uint REG_GS   = 0x4a;

const uint REG_CR0  = 0x50;
const uint REG_CR1  = 0x51;
const uint REG_CR2  = 0x52;
const uint REG_CR3  = 0x53;
const uint REG_CR4  = 0x54;

const uint REG_MM0  = 0x94;
const uint REG_MM1  = 0x95;
const uint REG_MM2  = 0x96;
const uint REG_MM3  = 0x97;
const uint REG_MM4  = 0x98;

const uint REG_MM5  = 0x99;
const uint REG_MM6  = 0x9a;
const uint REG_MM7  = 0x9b;      

const uint REG_XMM0 = 0x9c;
const uint REG_XMM1 = 0x9d;
const uint REG_XMM2 = 0x9e;
const uint REG_XMM3 = 0x9f;
const uint REG_XMM4 = 0xa0;
const uint REG_XMM5 = 0xa1;
const uint REG_XMM6 = 0xa2;
const uint REG_XMM7 = 0xa3;
      
const uint g_num_regs   = 256;
const uint g_inst_delay = 64; // the delay before the values from an instruction become visible 
const uint g_num_ops    = 64;

class op_state_c;

class op_record_c
{
    // when was the latest time that the values were set
    uint clock_time_set;
    // src and dst values
    uint src1_val;
    uint src2_val;
    uint dst_val;
    // mem src addr
    uint src_vaddr;
    // mem dst addr
    uint dst_vaddr;
    op_state_c *osptr;
public:
    // methods for getting data values from an instruction.
    // the user should access the data values using these methods to avoid
    // using data values that shouldn't yet be available.
    // instructions have 2 srcs (src1, src2) and a dst (dst) as well as a memory source
    // (address "src_vaddr") and a memory dest (address "dst_vaddr").

    // Are the values contained in the record available and can they be inspected.  This should be called
    // before trying to look at the data values.
    bool are_values_available();
    // set/get the value for src1 
    void set_src1_val(uint new_src1_val);
    uint get_src1_val();
    // set/get the value for src2 
    void set_src2_val(uint new_src2_val);
    uint get_src2_val();
     // set/get the value for dst
    void set_dst_val(uint new_dst_val);
    uint get_dst_val();
    // set/get the address for a memory source 
    void set_src_vaddr(uint new_src_vaddr);
    uint get_src_vaddr();
    // set/get the address for a memory dest
    void set_dst_vaddr(uint new_dst_vaddr);
    uint get_dst_vaddr();
    // is this a valid record containing arch state uploaded from a trace
    bool is_valid;

    // opcode class, either load, store, branch, or op
    uint op_class;   
    // instruction address
    uint instruction_addr;
    // what type of instruction is this
    bool is_load;     // 0 == op_class
    bool is_store;    // 1 == op_class
    bool is_branch;   // 3 == op_class
    bool is_op;       // 2 == op_class
    // is floating-point instruction
    bool is_fp;      
    // does this uop source the condition flags
    bool read_flg;
    // does this uop write to the condition flags
    bool writ_flg;
    // the register numbers of the two sources for the instruction, use REG_XXX to decode
    // the particular register that is being referred to
    uint src1;
    uint src2;
    // the register number of the instruction's destination (only one is allowed)
    uint dst;
    // does this uop have a memory source
    bool has_mem_src;
    // does this uop have a memory dest
    bool has_mem_dst;
    // special escape value used by the trace reader
    bool special_esc;
    // for a memory source, the register numbers for the index/base/segment regs
    uint mem_src1;
    uint mem_src2;
    uint mem_src3;
  
    op_record_c();
    ~op_record_c();
    void init();
    void set_op_state(op_state_c *new_osptr);
    void debug_print();
};

// op_state_c holds all the register values that have been committed and are available when the branch is predicted
class op_state_c{
private:
    int clock;
public:
    // array indicating which regs are valid
    bool *regs_valid;
    // regs: register file of data values visible to the branch predictor
    uint *regs;
    // number of regs in the register file
    uint num_regs;
    // the delay before an instructions result becomes visible to the branch predictor
    uint inst_delay;
    // number of ops contained in the op_list
    uint num_ops;
    // the op_list contains all ops that produce data values not yet visible to the branch predictor
    op_record_c *op_list;
    // pointer to the oldest op in the op list 
    uint op_list_ptr;

    op_state_c();
    ~op_state_c();
    void init(op_state_c *new_osptr);
    const char *register_name(uint register_code);
    // clock methods
    uint get_clock(){
        return clock;
    }
    void inc_clock(){
        clock++;
        op_list_ptr = (op_list_ptr + 1) % inst_delay;
    }
    // op state method:
    // is_reg_valid: use this method for checking to see if a register has had a valid result
    // written into it.  Regnum can be any number from 0 - 255 since there are 256 registers.
    bool is_reg_valid(uint reg_num){
        return regs_valid[reg_num];
    }
    // get_reg_state:  use this method for getting values that are stored in a register file entry.
    // It is wise to check that the values are valid before using them.  Some register file entries
    // may never be written to over the course of a trace's execution.
    uint get_reg_state(uint reg_num){
        return regs[reg_num];
    }
    // get_op_record: use this method to get the op_record.  op_num = 0 is the most recent op_record.
    // op_num = 63 is the oldest.
    op_record_c *get_op_record(uint op_num){
        uint index  = (op_list_ptr - op_num) % inst_delay;
        assert(op_num < num_ops);
        return op_list + index;
    }
};

#endif // OP_STATE_H_SEEN
