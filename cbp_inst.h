/* Author: Jared Stark;   Created: Tue Aug  3 15:45:21 PDT 2004
 * Description: This file defines the structure CBP_INST, which contains the
 * information for one instruction in a trace.  It also provides functions for
 * performing input and output of CBP_INST structures.
*/ 

#ifndef CBP_INST_H_SEEN
#define CBP_INST_H_SEEN

#include <cstdio>
#include <inttypes.h>

namespace cbp
{
    // Unused fields (e.g., BRANCH ONLY fields for non-branches) should be set to 0 or false.
    struct CBP_INST
    {
        // ****************************************
        // static information
        // ****************************************
    
        uint32_t instruction_addr;        // the instruction's PC (program counter)
        uint32_t instruction_next_addr;   // (BRANCH ONLY) the PC of the next static instruction
    
        uint8_t op_class;                 // opcode class (2-bit value)
        bool is_load;                     // 0 == op_class
        bool is_store;                    // 1 == op_class
        bool is_op;                       // 2 == op_class
        bool is_branch;                   // 3 == op_class
    
        bool is_fp;                       // is floating-point instruction
    
        bool is_indirect;                 // (BRANCH ONLY) true if target computed; false if target PC-rel
        bool is_conditional;              // (BRANCH ONLY) is conditional branch
        bool is_call;                     // (BRANCH ONLY) is call
        bool is_return;                   // (BRANCH ONLY) is return
    
        bool read_flg;                    // true if the instruction reads EFLAGS; false otherwise
        bool writ_flg;                    // true if the instruction writes EFLAGS; false otherwise
    
        bool has_mem_src;                 // true if the instruction has a src mem operand; false otherwise
        bool has_mem_dst;                 // true if the instruction has a dst mem operand; false otherwise
    
        uint8_t src1;                     // src reg 1 specifier
        uint8_t src2;                     // src reg 2 specifier
        uint8_t dst;                      // dst reg specifier
    
        uint8_t mem_src1;                 // src reg 1 specifier for effective addr computation of mem operand
        uint8_t mem_src2;                 // src reg 2 specifier for effective addr computation of mem operand
        uint8_t mem_src3;                 // src reg 3 specifier for effective addr computation of mem operand
    
        // ****************************************
        // partially dynamic information
        // ****************************************
  
        // (BRANCH ONLY) Target of the branch if it's taken.  This field is static
        // only if the branch target is PC-rel.  Otherwise, it's dynamic.
        uint32_t branch_target;

        // ****************************************
        // dynamic information
        // ****************************************
    
        uint32_t src1_val;                // src reg 1 value
        uint32_t src2_val;                // src reg 2 value
        uint32_t dst_val;                 // dst reg value
    
        uint32_t src_vaddr;               // effective addr of src mem operand
        uint32_t dst_vaddr;               // effective addr of dst mem operand
    
        bool taken;                       // (BRANCH ONLY) false if not-taken conditional branch; true for all other branches
    };
    
    // This type controls a stream of CBP_INST structures.  The stream can be
    // used for either input or output, but not both.
    struct CBP_INST_STREAM;
    
    // Constructs a CBP_INST_STREAM from open std::FILE* 'stream' and returns a
    // pointer to it.
    CBP_INST_STREAM* cbp_inst_open(std::FILE* stream);
    
    // Destructs 'stream'.  Returns the std::FILE* that was used to construct 'stream'.
    // It is the client's responsibility to close this std::FILE*.
    std::FILE* cbp_inst_close(CBP_INST_STREAM* stream);
    
    // Reads 'inst' from 'stream'.  Returns true on success and false on failure.
    bool cbp_inst_read(CBP_INST_STREAM* stream, CBP_INST* inst);
    
    // Writes 'inst' to 'stream'.  Returns true on success and false on failure.
    bool cbp_inst_write(CBP_INST_STREAM* stream, const CBP_INST* inst);
    
    // Writes the statistics for 'cbp_inst_stream' to 'stream'.  Returns true on success and
    // false on failure.  This function is used for debugging.
    bool cbp_inst_print_statistics(std::FILE* stream, const CBP_INST_STREAM* cbp_inst_stream);
} // namespace cbp

#endif // CBP_INST_H_SEEN

