/* Author: Jared Stark;   Created: Tue Aug  3 15:45:21 PDT 2004
 * Description: This file defines the structure CBP_INST, which contains the
 * information for one instruction in a trace.  It also provides functions for
 * performing input and output of CBP_INST structures.
*/

#include "cbp_inst.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <inttypes.h>
#include <sstream>
#include <string>
#include "cbp_assert.h"
#include "cbp_fatal.h"
#include "cond_pred.h"
#include "finite_stack.h"
#include "indirect_pred.h"
#include "stride_pred.h"
#include "value_cache.h"

#define NBYTE(arg)   (sizeof(arg) / sizeof(uint8_t))

namespace cbp
{
    using namespace std;

    /* **************************************** */

    // Unless otherwise noted, these fields contain the same values
    // as the same-named fields in the CBP_INST structure.
    struct STATIC_INFO
    {
        uint8_t src1;                        // src reg 1 specifier
        uint8_t src2;                        // src reg 2 specifier
        uint8_t dst;                         // dst reg specifier
    
        uint8_t mem_src1;                    // src reg 1 specifier for effective addr computation of mem operand
        uint8_t mem_src2;                    // src reg 2 specifier for effective addr computation of mem operand
        uint8_t mem_src3;                    // src reg 3 specifier for effective addr computation of mem operand

        // **********

        typedef uint16_t BIT_FIELD_TYPE;

        BIT_FIELD_TYPE op_class       : 2;   // opcode class (2-bit value)

        /* The following fields are just derived from op_class. */
        // bool is_load;                     // 0 == op_class
        // bool is_store;                    // 1 == op_class
        // bool is_op;                       // 2 == op_class
        // bool is_branch;                   // 3 == op_class
    
        BIT_FIELD_TYPE is_fp          : 1;   // is floating-point instruction
    
        BIT_FIELD_TYPE is_indirect    : 1;   // (BRANCH ONLY) true if target computed; false if target PC-rel
        BIT_FIELD_TYPE is_conditional : 1;   // (BRANCH ONLY) is conditional branch
        BIT_FIELD_TYPE is_call        : 1;   // (BRANCH ONLY) is call
        BIT_FIELD_TYPE is_return      : 1;   // (BRANCH ONLY) is return
    
        BIT_FIELD_TYPE read_flg       : 1;   // true if the instruction reads EFLAGS; false otherwise
        BIT_FIELD_TYPE writ_flg       : 1;   // true if the instruction writes EFLAGS; false otherwise
    
        BIT_FIELD_TYPE has_mem_src    : 1;   // true if the instruction has a src mem operand; false otherwise
        BIT_FIELD_TYPE has_mem_dst    : 1;   // true if the instruction has a dst mem operand; false otherwise
    
        BIT_FIELD_TYPE get_bit_field_members(void) const;
        void set_bit_field_members(BIT_FIELD_TYPE members);

        // **********
     
        uint32_t instruction_addr;           // the instruction's PC (program counter)

        // For branch instructions, the PC of the next static instruction.  For non-branch
        // instructions, and unlike the CBP_INST structure, this field contains a prediction
        // for the PC of the next static instruction.
        uint32_t instruction_next_addr;

        // (BRANCH ONLY) Target of the branch if it's taken.  Unlike the CBP_INST structure,
        // this field is set to 0 if the target is not PC-rel.
        uint32_t branch_target;

        // **********
      
        STATIC_INFO(void);
        explicit STATIC_INFO(const CBP_INST& inst);
        // uses compiler generated copy constructor
        // uses compiler generated destructor
        // uses compiler generated assignment operator
       
        void fill(CBP_INST* inst) const;
    };

    STATIC_INFO::BIT_FIELD_TYPE
    STATIC_INFO::get_bit_field_members(void) const
    {
        return (0
            | (static_cast<BIT_FIELD_TYPE>(op_class)       << 0)
            | (static_cast<BIT_FIELD_TYPE>(is_fp)          << 2)
            | (static_cast<BIT_FIELD_TYPE>(is_indirect)    << 3)
            | (static_cast<BIT_FIELD_TYPE>(is_conditional) << 4)
            | (static_cast<BIT_FIELD_TYPE>(is_call)        << 5)
            | (static_cast<BIT_FIELD_TYPE>(is_return)      << 6)
            | (static_cast<BIT_FIELD_TYPE>(read_flg)       << 7)
            | (static_cast<BIT_FIELD_TYPE>(writ_flg)       << 8)
            | (static_cast<BIT_FIELD_TYPE>(has_mem_src)    << 9)
            | (static_cast<BIT_FIELD_TYPE>(has_mem_dst)    << 10));
    }

    void
    STATIC_INFO::set_bit_field_members(BIT_FIELD_TYPE members)
    {
        op_class       = ((members >> 0)  & 3);
        is_fp          = ((members >> 2)  & 1);
        is_indirect    = ((members >> 3)  & 1);
        is_conditional = ((members >> 4)  & 1);
        is_call        = ((members >> 5)  & 1);
        is_return      = ((members >> 6)  & 1);
        read_flg       = ((members >> 7)  & 1);
        writ_flg       = ((members >> 8)  & 1);
        has_mem_src    = ((members >> 9)  & 1);
        has_mem_dst    = ((members >> 10) & 1);
    }

    STATIC_INFO::STATIC_INFO(void)
        : src1(0),
          src2(0),
          dst(0),
          mem_src1(0),
          mem_src2(0),
          mem_src3(0),
          op_class(/* op */ 2),
          is_fp(0),
          is_indirect(0),
          is_conditional(0),
          is_call(0),
          is_return(0),
          read_flg(0),
          writ_flg(0),
          has_mem_src(0),
          has_mem_dst(0),
          instruction_addr(0),
          instruction_next_addr(0),
          branch_target(0)
    {
    }

    STATIC_INFO::STATIC_INFO(const CBP_INST& inst)
        : src1(inst.src1),
          src2(inst.src2),
          dst(inst.dst),
          mem_src1(inst.mem_src1),
          mem_src2(inst.mem_src2),
          mem_src3(inst.mem_src3),
          op_class(inst.op_class),
          is_fp(inst.is_fp),
          is_indirect(inst.is_indirect),
          is_conditional(inst.is_conditional),
          is_call(inst.is_call),
          is_return(inst.is_return),
          read_flg(inst.read_flg),
          writ_flg(inst.writ_flg),
          has_mem_src(inst.has_mem_src),
          has_mem_dst(inst.has_mem_dst),
          instruction_addr(inst.instruction_addr),
          instruction_next_addr(inst.is_branch ? inst.instruction_next_addr : /* wild guess */ (inst.instruction_addr + 3)),
          branch_target(inst.is_indirect ? 0 : inst.branch_target)
    {
        CBP_ASSERT(1 == (inst.is_load + inst.is_store + inst.is_op + inst.is_branch));
        switch (inst.op_class) {
          case 0: CBP_ASSERT(inst.is_load   == true); break;
          case 1: CBP_ASSERT(inst.is_store  == true); break;
          case 2: CBP_ASSERT(inst.is_op     == true); break;
          case 3: CBP_ASSERT(inst.is_branch == true); break;
          default:
            CBP_FATAL("invalid op_class");
            break;
        }
        CBP_ASSERT(!inst.is_indirect    || inst.is_branch);
        CBP_ASSERT(!inst.is_conditional || inst.is_branch);
        CBP_ASSERT(!inst.is_call        || inst.is_branch);
        CBP_ASSERT(!inst.is_return      || inst.is_branch);
        CBP_ASSERT((inst.instruction_next_addr == 0) || inst.is_branch);
        CBP_ASSERT((inst.branch_target == 0) || inst.is_branch);
        CBP_ASSERT(!inst.is_return || inst.is_indirect);   // returns should be indirect
    }

    void
    STATIC_INFO::fill(CBP_INST* inst) const
    {
        inst->src1                  = src1;
        inst->src2                  = src2;
        inst->dst                   = dst;
        inst->mem_src1              = mem_src1;
        inst->mem_src2              = mem_src2;
        inst->mem_src3              = mem_src3;
        inst->op_class              = op_class;
        inst->is_load               = (0 == op_class);
        inst->is_store              = (1 == op_class);
        inst->is_op                 = (2 == op_class);
        inst->is_branch             = (3 == op_class);
        inst->is_fp                 = is_fp;
        inst->is_indirect           = is_indirect;
        inst->is_conditional        = is_conditional;
        inst->is_call               = is_call;
        inst->is_return             = is_return;
        inst->read_flg              = read_flg;
        inst->writ_flg              = writ_flg;
        inst->has_mem_src           = has_mem_src;
        inst->has_mem_dst           = has_mem_dst;
        inst->instruction_addr      = instruction_addr;
        inst->instruction_next_addr = (inst->is_branch ? instruction_next_addr : 0);
        CBP_ASSERT(!is_return || is_indirect);   // returns should be indirect
        if (!is_indirect)
            inst->branch_target = branch_target;
    }

    bool
    operator!=(const STATIC_INFO& lhs, const STATIC_INFO& rhs)
    {
        return (0
            || (lhs.instruction_addr      != rhs.instruction_addr)
            || (/* is branch */ (lhs.op_class == 3)
                && (lhs.instruction_next_addr != rhs.instruction_next_addr))
            || (lhs.branch_target         != rhs.branch_target   )
            || (lhs.src1                  != rhs.src1            )
            || (lhs.src2                  != rhs.src2            )
            || (lhs.dst                   != rhs.dst             )
            || (lhs.mem_src1              != rhs.mem_src1        )
            || (lhs.mem_src2              != rhs.mem_src2        )
            || (lhs.mem_src3              != rhs.mem_src3        )
            || (lhs.op_class              != rhs.op_class        )
            || (lhs.is_fp                 != rhs.is_fp           )
            || (lhs.is_indirect           != rhs.is_indirect     )
            || (lhs.is_conditional        != rhs.is_conditional  )
            || (lhs.is_call               != rhs.is_call         )
            || (lhs.is_return             != rhs.is_return       )
            || (lhs.read_flg              != rhs.read_flg        )
            || (lhs.writ_flg              != rhs.writ_flg        )
            || (lhs.has_mem_src           != rhs.has_mem_src     )
            || (lhs.has_mem_dst           != rhs.has_mem_dst     ));
    }

    /* **************************************** */
    
    struct CBP_INST_STREAM
    {
      private:
        // not implemented
        explicit CBP_INST_STREAM(const CBP_INST_STREAM&);
        CBP_INST_STREAM& operator=(const CBP_INST_STREAM&);
    
        // the underlying stream
        FILE* stream;
    
        // input or output buffer
        enum { BUFFER_SIZE = 50 };     // size of largest io format CBP_INST
        uint8_t buffer[BUFFER_SIZE];
        uint8_t* buffer_tail;
        template <class Type> void get_buffer(Type* ptr);
        template <class Type> void put_buffer(const Type* ptr);
    
        // The key describes how to construct a CBP_INST.  Each member is either
        // predicted or read from the buffer, except for 1-bit members.  For 1-bit
        // members, the key simply indicates whether the 1-bit prediction is wrong
        // and needs to be flipped.  Most members have only a single prediction, but
        // some have multiple, in which case the key specifies the predictor type.
        //
        // There are two key sizes: 1 and 2 bytes.  A 2-byte key is only needed if
        // the second byte of the key is non-zero.  The first byte of the key has a
        // bit (TWO_BYTE_KEY) that specifies whether this is the case.  The key
        // format is designed to reduce the number of times a 2-byte key is needed.
        typedef uint16_t KEY_TYPE;
        /* first byte */
        static const KEY_TYPE TWO_BYTE_KEY          = (KEY_TYPE(0x1) << 0 );
        static const KEY_TYPE MASK_DST_VAL          = (KEY_TYPE(0x3) << 1 );
        static const KEY_TYPE TYPE0_DST_VAL         =  (KEY_TYPE(0) << 1 );
        static const KEY_TYPE TYPE1_DST_VAL         =  (KEY_TYPE(1) << 1 );
        static const KEY_TYPE TYPE2_DST_VAL         =  (KEY_TYPE(2) << 1 );
        static const KEY_TYPE READ_DST_VAL          =  (KEY_TYPE(3) << 1 );
        static const KEY_TYPE MASK_VADDR1           = (KEY_TYPE(0x3) << 3 );
        static const KEY_TYPE TYPE0_VADDR1          =  (KEY_TYPE(0) << 3 );
        static const KEY_TYPE TYPE1_VADDR1          =  (KEY_TYPE(1) << 3 );
        static const KEY_TYPE TYPE2_VADDR1          =  (KEY_TYPE(2) << 3 );
        static const KEY_TYPE READ_VADDR1           =  (KEY_TYPE(3) << 3 );
        static const KEY_TYPE READ_SRC2_VAL         = (KEY_TYPE(0x1) << 5 );
        static const KEY_TYPE FLIP_TAKEN            = (KEY_TYPE(0x1) << 6 );
        static const KEY_TYPE READ_STATIC_INFO      = (KEY_TYPE(0x1) << 7 );
        /* second byte */
        static const KEY_TYPE READ_INSTRUCTION_ADDR = (KEY_TYPE(0x1) << 8 );
        static const KEY_TYPE MASK_BRANCH_TARGET    = (KEY_TYPE(0x3) << 9 );
        static const KEY_TYPE TYPE0_BRANCH_TARGET   =  (KEY_TYPE(0) << 9 );
        static const KEY_TYPE TYPE1_BRANCH_TARGET   =  (KEY_TYPE(1) << 9 );
        static const KEY_TYPE TYPE2_BRANCH_TARGET   =  (KEY_TYPE(2) << 9 );
        static const KEY_TYPE READ_BRANCH_TARGET    =  (KEY_TYPE(3) << 9 );
        static const KEY_TYPE READ_SRC1_VAL         = (KEY_TYPE(0x1) << 11);
        static const KEY_TYPE READ_VADDR2           = (KEY_TYPE(0x1) << 12);
        static const KEY_TYPE UNUSED_KEY_BIT1       = (KEY_TYPE(0x1) << 13);
        static const KEY_TYPE UNUSED_KEY_BIT2       = (KEY_TYPE(0x1) << 14);
        static const KEY_TYPE UNUSED_KEY_BIT3       = (KEY_TYPE(0x1) << 15);
    
        // For each CBP_INST member, there is a pair of functions: a get function and
        // a put function.  When a CBP_INST is read, the get functions are called to
        // construct each member.  When a CBP_INST is written, the put functions are
        // called to access the predictors, update the key, and write the members into
        // the buffer.  The sequence of either get or put functions uses the variables
        // below to do their work.  The functions must be called in the order they are
        // declared.  They use the variables to communicate to each other, and thus
        // have dependencies between them.
    
        KEY_TYPE key;
        CBP_INST inst;
        STATIC_INFO* static_info;
    
        const CBP_INST* output_inst;
    
        uint32_t get_instruction_addr_prediction(void) const;
        void get_instruction_addr(void);
        void put_instruction_addr(void);

        enum { STATIC_INFO_CACHE_SIZE = (1 << 18) };
        STATIC_INFO static_info_cache[STATIC_INFO_CACHE_SIZE];
        STATIC_INFO* get_static_info_prediction(uint32_t vip);
        void get_static_info(void);
        void put_static_info(void);
    
        static const int REG_NUL = 0;
        static const int REG_MAX = 256;
        uint32_t register_file[REG_MAX];
        void get_src1_val(void);
        void put_src1_val(void);

        void get_src2_val(void);
        void put_src2_val(void);

        typedef VALUE_CACHE<8, 0> DST_VAL_L0;
        typedef VALUE_CACHE<16, 0> DST_VAL_L1;
        STRIDE_PRED<14> dst_val_stride_pred;
        DST_VAL_L0 dst_val_l0;
        DST_VAL_L1 dst_val_l1;
        void get_dst_val(void);
        void put_dst_val(void);

        typedef VALUE_CACHE<2, 6> VADDR1_L0;
        typedef VALUE_CACHE<7, 9> VADDR1_L1;
        STRIDE_PRED<14> vaddr1_stride_pred;
        VADDR1_L0 vaddr1_l0;
        VADDR1_L1 vaddr1_l1;
        uint32_t get_vaddr1(void);
        void put_vaddr1(uint32_t output_vaddr1);
        uint32_t get_vaddr2(void);
        void put_vaddr2(uint32_t output_vaddr2);
        void get_src_vaddr(void);
        void put_src_vaddr(void);

        void get_dst_vaddr(void);
        void put_dst_vaddr(void);
    
        COND_PRED<14> taken_cond_pred;
        bool get_taken_prediction(void) const;
        void update_taken_predictor(void);
        void get_taken(void);
        void put_taken(void);

        typedef VALUE_CACHE<8, 0> BRANCH_TARGET_L0;
        typedef VALUE_CACHE<16, 0> BRANCH_TARGET_L1;
        FINITE_STACK<128> branch_target_ret_pred;
        INDIRECT_PRED<10> branch_target_ind_pred;
        BRANCH_TARGET_L0 branch_target_l0;
        BRANCH_TARGET_L1 branch_target_l1;
        uint32_t get_branch_target_prediction(void) const;
        void update_branch_target_predictors(void);
        void get_branch_target(void);
        void put_branch_target(void);
    
        // statistics
        typedef uint64_t EVENT_COUNTER;
        EVENT_COUNTER stat_cbp_inst;
        EVENT_COUNTER stat_two_byte_key;
        EVENT_COUNTER stat_type0_dst_val;
        EVENT_COUNTER stat_type1_dst_val;
        EVENT_COUNTER stat_type2_dst_val;
        EVENT_COUNTER stat_read_dst_val;
        EVENT_COUNTER stat_type0_vaddr1;
        EVENT_COUNTER stat_type1_vaddr1;
        EVENT_COUNTER stat_type2_vaddr1;
        EVENT_COUNTER stat_read_vaddr1;
        EVENT_COUNTER stat_read_src2_val;
        EVENT_COUNTER stat_flip_taken;
        EVENT_COUNTER stat_read_static_info;
        EVENT_COUNTER stat_read_instruction_addr;
        EVENT_COUNTER stat_type0_branch_target;
        EVENT_COUNTER stat_type1_branch_target;
        EVENT_COUNTER stat_type2_branch_target;
        EVENT_COUNTER stat_read_branch_target;
        EVENT_COUNTER stat_read_src1_val;
        EVENT_COUNTER stat_read_vaddr2;
        void update_statistics(void);
    
      public:
        CBP_INST_STREAM(FILE* stream_arg);
        // uses compiler generated destructor
    
        FILE* get_stream(void) { return stream; }
    
        // These functions return true on success and false on failure.
        bool read(CBP_INST* inst_arg);
        bool write(const CBP_INST* inst_arg);
    
        string get_statistics_string(void) const;
    };
    
    template <class Type>
    inline void
    CBP_INST_STREAM::get_buffer(Type* ptr)
    {
        memcpy(ptr, buffer_tail, (sizeof(uint8_t) * NBYTE(Type)));
        buffer_tail += NBYTE(Type);
    }
    
    template <class Type>
    inline void
    CBP_INST_STREAM::put_buffer(const Type* ptr)
    {
        memcpy(buffer_tail, ptr, (sizeof(uint8_t) * NBYTE(Type)));
        buffer_tail += NBYTE(Type);
    }
    
    uint32_t
    CBP_INST_STREAM::get_instruction_addr_prediction(void) const
    {
        if (!inst.is_branch)
            return static_info->instruction_next_addr;
        else if (inst.is_conditional)
            return (inst.taken ? inst.branch_target : inst.instruction_next_addr);
        else
            return inst.branch_target;
    }

    inline void
    CBP_INST_STREAM::get_instruction_addr(void)
    {
        uint32_t instruction_addr = get_instruction_addr_prediction();
        if (key & READ_INSTRUCTION_ADDR) {
            uint32_t patch;
            get_buffer(&patch);
            instruction_addr ^= patch;
        }
        inst.instruction_addr = instruction_addr;

        // For non-branches, copy the instruction_addr field into the
        // instruction_next_addr field of the previous instruction.
        if (!inst.is_branch)
            static_info->instruction_next_addr = inst.instruction_addr;
    }
    
    inline void
    CBP_INST_STREAM::put_instruction_addr(void)
    {
        uint32_t output_instruction_addr = output_inst->instruction_addr;
        inst.instruction_addr = output_instruction_addr;
    
        uint32_t instruction_addr = get_instruction_addr_prediction();
        if (instruction_addr != output_instruction_addr) {
            key |= READ_INSTRUCTION_ADDR;
            uint32_t patch = (output_instruction_addr ^ instruction_addr);
            put_buffer(&patch);
        }

        // For non-branches, copy the instruction_addr field into the
        // instruction_next_addr field of the previous instruction.
        if (!inst.is_branch)
            static_info->instruction_next_addr = inst.instruction_addr;
    }
    
    inline STATIC_INFO*
    CBP_INST_STREAM::get_static_info_prediction(uint32_t vip)
    {
        size_t index = (static_cast<size_t>(vip) % STATIC_INFO_CACHE_SIZE);
        return (static_info_cache + index);
    }
    
    inline void
    CBP_INST_STREAM::get_static_info(void)
    {
        static_info = get_static_info_prediction(inst.instruction_addr);
        if (key & READ_STATIC_INFO) {
            get_buffer(&static_info->src1);
            get_buffer(&static_info->src2);
            get_buffer(&static_info->dst);
            get_buffer(&static_info->mem_src1);
            get_buffer(&static_info->mem_src2);
            get_buffer(&static_info->mem_src3);
            STATIC_INFO::BIT_FIELD_TYPE bit_field_members;
            get_buffer(&bit_field_members);
            static_info->set_bit_field_members(bit_field_members);
            get_buffer(&static_info->instruction_addr);
            get_buffer(&static_info->instruction_next_addr);
            get_buffer(&static_info->branch_target);
        }
        static_info->fill(&inst);
    }
    
    inline void
    CBP_INST_STREAM::put_static_info(void)
    {
        const STATIC_INFO output_static_info(*output_inst);
        output_static_info.fill(&inst);

        static_info = get_static_info_prediction(inst.instruction_addr);
        if (*static_info != output_static_info) {
            key |= READ_STATIC_INFO;
            put_buffer(&output_static_info.src1);
            put_buffer(&output_static_info.src2);
            put_buffer(&output_static_info.dst);
            put_buffer(&output_static_info.mem_src1);
            put_buffer(&output_static_info.mem_src2);
            put_buffer(&output_static_info.mem_src3);
            STATIC_INFO::BIT_FIELD_TYPE bit_field_members;
            bit_field_members = output_static_info.get_bit_field_members();
            put_buffer(&bit_field_members);
            put_buffer(&output_static_info.instruction_addr);
            put_buffer(&output_static_info.instruction_next_addr);
            put_buffer(&output_static_info.branch_target);
            *static_info = output_static_info;
        }
    }

    inline void
    CBP_INST_STREAM::get_src1_val(void)
    {
        uint32_t* src1_val = &register_file[inst.src1];
        if (key & READ_SRC1_VAL) {
            uint32_t patch;
            get_buffer(&patch);
            *src1_val ^= patch;
        }
        inst.src1_val = *src1_val;
    }
    
    inline void
    CBP_INST_STREAM::put_src1_val(void)
    {
        uint32_t output_src1_val = output_inst->src1_val;
        inst.src1_val = output_src1_val;
    
        uint32_t* src1_val = &register_file[inst.src1];
        if (*src1_val != output_src1_val) {
            key |= READ_SRC1_VAL;
            uint32_t patch = (output_src1_val ^ *src1_val);
            put_buffer(&patch);
            *src1_val = output_src1_val;
        }
    }

    inline void
    CBP_INST_STREAM::get_src2_val(void)
    {
        uint32_t* src2_val = &register_file[inst.src2];
        if (key & READ_SRC2_VAL) {
            uint32_t patch;
            get_buffer(&patch);
            *src2_val ^= patch;
        }
        inst.src2_val = *src2_val;
    }
    
    inline void
    CBP_INST_STREAM::put_src2_val(void)
    {
        uint32_t output_src2_val = output_inst->src2_val;
        inst.src2_val = output_src2_val;
    
        uint32_t* src2_val = &register_file[inst.src2];
        if (*src2_val != output_src2_val) {
            key |= READ_SRC2_VAL;
            uint32_t patch = (output_src2_val ^ *src2_val);
            put_buffer(&patch);
            *src2_val = output_src2_val;
        }
    }

    inline void
    CBP_INST_STREAM::get_dst_val(void)
    {
        // get the dst reg value
        uint32_t dst_val = 0;
        uint8_t output_l0_id;
        uint16_t output_l1_id;
        switch (MASK_DST_VAL & key) {
          case TYPE0_DST_VAL:   // 0 byte encoding
            if (REG_NUL != inst.dst)
                dst_val = dst_val_stride_pred.get_prediction(inst.instruction_addr);
            break;
          case TYPE1_DST_VAL:   // 1 byte encoding
            get_buffer(&output_l0_id);
            dst_val = dst_val_l0[output_l0_id];
            break;
          case TYPE2_DST_VAL:   // 2 byte encoding
            get_buffer(&output_l1_id);
            dst_val = dst_val_l1[output_l1_id];
            break;
          case READ_DST_VAL:    // 4 byte encoding
            get_buffer(&dst_val);
            break;
          default:
            CBP_FATAL("invalid key");
            break;
        }
        inst.dst_val = dst_val;

        // update the register file
        if (REG_NUL != inst.dst)
            register_file[inst.dst] = inst.dst_val;
    
        // train the predictor and update the caches
        switch (MASK_DST_VAL & key) {
          case READ_DST_VAL:    // 4 byte encoding
          case TYPE2_DST_VAL:   // 2 byte encoding
            dst_val_l1.insert(dst_val);
          case TYPE1_DST_VAL:   // 1 byte encoding
            dst_val_l0.insert(dst_val);
          case TYPE0_DST_VAL:   // 0 byte encoding
            if (REG_NUL != inst.dst)
                dst_val_stride_pred.train(inst.instruction_addr, dst_val);
            break;
          default:
            CBP_FATAL("invalid key");
            break;
        }
    }
    
    inline void
    CBP_INST_STREAM::put_dst_val(void)
    {
        uint32_t output_dst_val = output_inst->dst_val;
        inst.dst_val = output_dst_val;
    
        // update the register file
        if (REG_NUL != inst.dst)
            register_file[inst.dst] = inst.dst_val;

        // try to encode the dst reg value in 0 bytes
        uint32_t dst_val = 0;
        if (REG_NUL != inst.dst) {
            dst_val = dst_val_stride_pred.get_prediction(inst.instruction_addr);
            dst_val_stride_pred.train(inst.instruction_addr, output_dst_val);
        }
        if (dst_val == output_dst_val) {
            key |= TYPE0_DST_VAL;
            return;
        }
    
        // try to encode the dst reg value in 1 byte
        typedef DST_VAL_L0::id_type L0_ID_TYPE;
        L0_ID_TYPE l0_id = dst_val_l0.find(output_dst_val);
        dst_val_l0.insert(output_dst_val);
        if (/* found */ (DST_VAL_L0::NOT_FOUND != l0_id)) {
            key |= TYPE1_DST_VAL;
            uint8_t output_l0_id = l0_id;
            put_buffer(&output_l0_id);
            return;
        }
    
        // try to encode the dst reg value in 2 bytes
        typedef DST_VAL_L1::id_type L1_ID_TYPE;
        L1_ID_TYPE l1_id = dst_val_l1.find(output_dst_val);
        dst_val_l1.insert(output_dst_val);
        if (/* found */ (DST_VAL_L1::NOT_FOUND != l1_id)) {
            key |= TYPE2_DST_VAL;
            uint16_t output_l1_id = l1_id;
            put_buffer(&output_l1_id);
            return;
        }
    
        // encode the dst reg value in 4 bytes
        key |= READ_DST_VAL;
        put_buffer(&output_dst_val);
    }
    
    uint32_t
    CBP_INST_STREAM::get_vaddr1(void)
    {
        // get the address
        uint32_t vaddr1 = 0;
        uint8_t output_l0_id;
        uint16_t output_l1_id;
        switch (MASK_VADDR1 & key) {
          case TYPE0_VADDR1:   // 0 byte encoding
            vaddr1 = vaddr1_stride_pred.get_prediction(inst.instruction_addr);
            break;
          case TYPE1_VADDR1:   // 1 byte encoding
            get_buffer(&output_l0_id);
            vaddr1 = vaddr1_l0[output_l0_id];
            break;
          case TYPE2_VADDR1:   // 2 byte encoding
            get_buffer(&output_l1_id);
            vaddr1 = vaddr1_l1[output_l1_id];
            break;
          case READ_VADDR1:    // 4 byte encoding
            get_buffer(&vaddr1);
            break;
          default:
            CBP_FATAL("invalid key");
            break;
        }
    
        // train the predictor and update the caches
        switch (MASK_VADDR1 & key) {
          case READ_VADDR1:    // 4 byte encoding
          case TYPE2_VADDR1:   // 2 byte encoding
            vaddr1_l1.insert(vaddr1);
          case TYPE1_VADDR1:   // 1 byte encoding
            vaddr1_l0.insert(vaddr1);
          case TYPE0_VADDR1:   // 0 byte encoding
            vaddr1_stride_pred.train(inst.instruction_addr, vaddr1);
            break;
          default:
            CBP_FATAL("invalid key");
            break;
        }

        return vaddr1;
    }
    
    void
    CBP_INST_STREAM::put_vaddr1(uint32_t output_vaddr1)
    {
        // try to encode the address in 0 bytes
        uint32_t vaddr1 = vaddr1_stride_pred.get_prediction(inst.instruction_addr);
        vaddr1_stride_pred.train(inst.instruction_addr, output_vaddr1);
        if (vaddr1 == output_vaddr1) {
            key |= TYPE0_VADDR1;
            return;
        }
    
        // try to encode the address in 1 byte
        typedef VADDR1_L0::id_type L0_ID_TYPE;
        L0_ID_TYPE l0_id = vaddr1_l0.find(output_vaddr1);
        vaddr1_l0.insert(output_vaddr1);
        if (/* found */ (VADDR1_L0::NOT_FOUND != l0_id)) {
            key |= TYPE1_VADDR1;
            uint8_t output_l0_id = l0_id;
            put_buffer(&output_l0_id);
            return;
        }
    
        // try to encode the address in 2 bytes
        typedef VADDR1_L1::id_type L1_ID_TYPE;
        L1_ID_TYPE l1_id = vaddr1_l1.find(output_vaddr1);
        vaddr1_l1.insert(output_vaddr1);
        if (/* found */ (VADDR1_L1::NOT_FOUND != l1_id)) {
            key |= TYPE2_VADDR1;
            uint16_t output_l1_id = l1_id;
            put_buffer(&output_l1_id);
            return;
        }
    
        // encode the address in 4 bytes
        key |= READ_VADDR1;
        put_buffer(&output_vaddr1);
    }

    uint32_t
    CBP_INST_STREAM::get_vaddr2(void)
    {
        uint32_t vaddr2 = /* vaddr1 */ inst.src_vaddr;
        if (key & READ_VADDR2) {
            uint32_t patch;
            get_buffer(&patch);
            vaddr2 ^= patch;
        }
        return vaddr2;
    }
    
    void
    CBP_INST_STREAM::put_vaddr2(uint32_t output_vaddr2)
    {
        uint32_t vaddr2 = /* vaddr1 */ inst.src_vaddr;
        if (vaddr2 != output_vaddr2) {
            key |= READ_VADDR2;
            uint32_t patch = (output_vaddr2 ^ vaddr2);
            put_buffer(&patch);
        }
    }

    inline void
    CBP_INST_STREAM::get_src_vaddr(void)
    {
        uint32_t src_vaddr = 0;
        if (inst.has_mem_src)
            src_vaddr = get_vaddr1();
        inst.src_vaddr = src_vaddr;
    }

    inline void
    CBP_INST_STREAM::put_src_vaddr(void)
    {
        uint32_t output_src_vaddr = output_inst->src_vaddr;
        inst.src_vaddr = output_src_vaddr;

        CBP_ASSERT(inst.has_mem_src || (0 == output_src_vaddr));
        if (inst.has_mem_src)
            put_vaddr1(output_src_vaddr);
    }

    inline void
    CBP_INST_STREAM::get_dst_vaddr(void)
    {
        uint32_t dst_vaddr = 0;
        if (inst.has_mem_dst)
            dst_vaddr = (inst.has_mem_src ? get_vaddr2() : get_vaddr1());
        inst.dst_vaddr = dst_vaddr;
    }

    inline void
    CBP_INST_STREAM::put_dst_vaddr(void)
    {
        uint32_t output_dst_vaddr = output_inst->dst_vaddr;
        inst.dst_vaddr = output_dst_vaddr;

        CBP_ASSERT(inst.has_mem_dst || (0 == output_dst_vaddr));
        if (inst.has_mem_dst) {
            if (inst.has_mem_src)
                put_vaddr2(output_dst_vaddr);
            else
                put_vaddr1(output_dst_vaddr);
        }

        // It's possible to reach this point without having called either put_vaddr1() or put_vaddr2(),
        // which set the key for vaddr1 and vaddr2.  However, the default value in the key is correct
        // for these cases, so there is no need to do anything more.
    }

    bool
    CBP_INST_STREAM::get_taken_prediction(void) const
    {
        if (!inst.is_branch)
            return false;
        else if (inst.is_conditional)
            return taken_cond_pred.get_prediction(inst.instruction_addr);
        else
            return true;
    }

    void
    CBP_INST_STREAM::update_taken_predictor(void)
    {
        if (inst.is_conditional) {
            CBP_ASSERT(inst.is_branch);
            taken_cond_pred.train(inst.instruction_addr, inst.taken);
        }
    }
    
    inline void
    CBP_INST_STREAM::get_taken(void)
    {
        bool taken = get_taken_prediction();
        if (key & FLIP_TAKEN)
            taken = !taken;
        inst.taken = taken;

        update_taken_predictor();
    }
    
    inline void
    CBP_INST_STREAM::put_taken(void)
    {
        bool output_taken = output_inst->taken;
        inst.taken = output_taken;

        bool taken = get_taken_prediction();
        if (taken != output_taken)
            key |= FLIP_TAKEN;

        update_taken_predictor();
    }

    uint32_t
    CBP_INST_STREAM::get_branch_target_prediction(void) const
    {
        CBP_ASSERT(!inst.is_return || inst.is_indirect);   // returns should be indirect
        if (!inst.is_branch)
            return 0;
        else if (!inst.is_indirect)
            return inst.branch_target;
        else if (inst.is_return)
            return branch_target_ret_pred.top();
        else
            return branch_target_ind_pred.get_prediction(inst.instruction_addr);
    }
    
    void
    CBP_INST_STREAM::update_branch_target_predictors(void)
    {
        if (!inst.is_branch)
            return;
        else if (inst.is_call)
            branch_target_ret_pred.push(inst.instruction_next_addr);
        if (inst.is_return)
            branch_target_ret_pred.pop();
        else if (inst.is_indirect)
            branch_target_ind_pred.train(inst.instruction_addr, inst.branch_target);
    }
    
    inline void
    CBP_INST_STREAM::get_branch_target(void)
    {
        // get branch target
        uint32_t branch_target = 0;
        uint8_t output_l0_id;
        uint16_t output_l1_id;
        uint32_t patch;
        switch (MASK_BRANCH_TARGET & key) {
          case TYPE0_BRANCH_TARGET:   // 0 byte encoding
            branch_target = get_branch_target_prediction();
            break;
          case TYPE1_BRANCH_TARGET:   // 1 byte encoding
            get_buffer(&output_l0_id);
            branch_target = branch_target_l0[output_l0_id];
            break;
          case TYPE2_BRANCH_TARGET:   // 2 byte encoding
            get_buffer(&output_l1_id);
            branch_target = branch_target_l1[output_l1_id];
            break;
          case READ_BRANCH_TARGET:    // 4 byte encoding
            get_buffer(&patch);
            branch_target = (inst.instruction_addr ^ patch);
            break;
          default:
            CBP_FATAL("invalid key");
            break;
        }
        inst.branch_target = branch_target;
    
        // update the predictors and the caches
        switch (MASK_BRANCH_TARGET & key) {
          case READ_BRANCH_TARGET:    // 4 byte encoding
          case TYPE2_BRANCH_TARGET:   // 2 byte encoding
            branch_target_l1.insert(branch_target);
          case TYPE1_BRANCH_TARGET:   // 1 byte encoding
            branch_target_l0.insert(branch_target);
          case TYPE0_BRANCH_TARGET:   // 0 byte encoding
            update_branch_target_predictors();
            break;
          default:
            CBP_FATAL("invalid key");
            break;
        }
    }
    
    inline void
    CBP_INST_STREAM::put_branch_target(void)
    {
        uint32_t output_branch_target = output_inst->branch_target;
        inst.branch_target = output_branch_target;
    
        // try to encode the branch target in 0 bytes
        uint32_t branch_target = get_branch_target_prediction();
        update_branch_target_predictors();
        if (branch_target == output_branch_target) {
            key |= TYPE0_BRANCH_TARGET;
            return;
        }
    
        // try to encode the branch target in 1 byte
        typedef BRANCH_TARGET_L0::id_type L0_ID_TYPE;
        L0_ID_TYPE l0_id = branch_target_l0.find(output_branch_target);
        branch_target_l0.insert(output_branch_target);
        if (/* found */ (BRANCH_TARGET_L0::NOT_FOUND != l0_id)) {
            key |= TYPE1_BRANCH_TARGET;
            uint8_t output_l0_id = l0_id;
            put_buffer(&output_l0_id);
            return;
        }
    
        // try to encode the branch target in 2 bytes
        typedef BRANCH_TARGET_L1::id_type L1_ID_TYPE;
        L1_ID_TYPE l1_id = branch_target_l1.find(output_branch_target);
        branch_target_l1.insert(output_branch_target);
        if (/* found */ (BRANCH_TARGET_L1::NOT_FOUND != l1_id)) {
            key |= TYPE2_BRANCH_TARGET;
            uint16_t output_l1_id = l1_id;
            put_buffer(&output_l1_id);
            return;
        }
    
        // encode the branch target in 4 bytes
        key |= READ_BRANCH_TARGET;
        uint32_t patch = (output_branch_target ^ inst.instruction_addr);
        put_buffer(&patch);
    }

    void
    CBP_INST_STREAM::update_statistics(void)
    {
        ++stat_cbp_inst;
        stat_two_byte_key += (0 != (TWO_BYTE_KEY & key));
        switch (MASK_DST_VAL & key) {
          case TYPE0_DST_VAL:
            ++stat_type0_dst_val;
            break;
          case TYPE1_DST_VAL:
            ++stat_type1_dst_val;
            break;
          case TYPE2_DST_VAL:
            ++stat_type2_dst_val;
            break;
          case READ_DST_VAL:
            ++stat_read_dst_val;
            break;
          default:
            CBP_FATAL("invalid key");
            break;
        }
        switch (MASK_VADDR1 & key) {
          case TYPE0_VADDR1:
            ++stat_type0_vaddr1;
            break;
          case TYPE1_VADDR1:
            ++stat_type1_vaddr1;
            break;
          case TYPE2_VADDR1:
            ++stat_type2_vaddr1;
            break;
          case READ_VADDR1:
            ++stat_read_vaddr1;
            break;
          default:
            CBP_FATAL("invalid key");
            break;
        }
        stat_read_src2_val += (0 != (READ_SRC2_VAL & key));
        stat_flip_taken += (0 != (FLIP_TAKEN & key));
        stat_read_static_info += (0 != (READ_STATIC_INFO & key));
        stat_read_instruction_addr += (0 != (READ_INSTRUCTION_ADDR & key));
        switch (MASK_BRANCH_TARGET & key) {
          case TYPE0_BRANCH_TARGET:
            ++stat_type0_branch_target;
            break;
          case TYPE1_BRANCH_TARGET:
            ++stat_type1_branch_target;
            break;
          case TYPE2_BRANCH_TARGET:
            ++stat_type2_branch_target;
            break;
          case READ_BRANCH_TARGET:
            ++stat_read_branch_target;
            break;
          default:
            CBP_FATAL("invalid key");
            break;
        }
        stat_read_src1_val += (0 != (READ_SRC1_VAL & key));
        stat_read_vaddr2 += (0 != (READ_VADDR2 & key));
    }
    
    inline
    CBP_INST_STREAM::CBP_INST_STREAM(FILE* stream_arg)
        : stream(stream_arg),
          stat_cbp_inst(0),
          stat_two_byte_key(0),
          stat_type0_dst_val(0),
          stat_type1_dst_val(0),
          stat_type2_dst_val(0),
          stat_read_dst_val(0),
          stat_type0_vaddr1(0),
          stat_type1_vaddr1(0),
          stat_type2_vaddr1(0),
          stat_read_vaddr1(0),
          stat_read_src2_val(0),
          stat_flip_taken(0),
          stat_read_static_info(0),
          stat_read_instruction_addr(0),
          stat_type0_branch_target(0),
          stat_type1_branch_target(0),
          stat_type2_branch_target(0),
          stat_read_branch_target(0),
          stat_read_src1_val(0),
          stat_read_vaddr2(0)
    {
        inst.instruction_addr = 0;
        inst.instruction_next_addr = 0;
        inst.op_class = /* op */ 2;
        inst.is_load = false;
        inst.is_store = false;
        inst.is_op = true;
        inst.is_branch = false;
        inst.is_fp = false;
        inst.is_indirect = false;
        inst.is_conditional = false;
        inst.is_call = false;
        inst.is_return = false;
        inst.read_flg = false;
        inst.writ_flg = false;
        inst.has_mem_src = false;
        inst.has_mem_dst = false;
        inst.src1 = 0;
        inst.src2 = 0;
        inst.dst = 0;
        inst.mem_src1 = 0;
        inst.mem_src2 = 0;
        inst.mem_src3 = 0;
        inst.branch_target = 0;
        inst.src1_val = 0;
        inst.src2_val = 0;
        inst.dst_val = 0;
        inst.src_vaddr = 0;
        inst.dst_vaddr = 0;
        inst.taken = false;
        static_info = get_static_info_prediction(inst.instruction_addr);
        fill_n(register_file, static_cast<size_t>(REG_MAX), 0);
    }
    
    inline bool
    CBP_INST_STREAM::read(CBP_INST* inst_arg)
    {
        size_t bytes_needed;
    
        // read the first byte
        if (fread(&buffer[0], sizeof(uint8_t), 1, stream) != 1)
            return /* failure */ false;
    
        // get the first byte of the key
        key = buffer[0];
    
        // compute the number of extra bytes needed for the first byte of the key
        bytes_needed = ((key & TWO_BYTE_KEY) ? 1 : 0);
        switch (MASK_DST_VAL & key) {
          case TYPE0_DST_VAL:   // 0 byte encoding
            break;
          case TYPE1_DST_VAL:   // 1 byte encoding
            bytes_needed += NBYTE(uint8_t);
            break;
          case TYPE2_DST_VAL:   // 2 byte encoding
            bytes_needed += NBYTE(uint16_t);
            break;
          case READ_DST_VAL:    // 4 byte encoding
            bytes_needed += NBYTE(inst.dst_val);
            break;
          default:
            CBP_FATAL("invalid key");
            break;
        }
        switch (MASK_VADDR1 & key) {
          case TYPE0_VADDR1:   // 0 byte encoding
            break;
          case TYPE1_VADDR1:   // 1 byte encoding
            bytes_needed += NBYTE(uint8_t);
            break;
          case TYPE2_VADDR1:   // 2 byte encoding
            bytes_needed += NBYTE(uint16_t);
            break;
          case READ_VADDR1:    // 4 byte encoding
            bytes_needed += NBYTE(inst.src_vaddr);
            break;
          default:
            CBP_FATAL("invalid key");
            break;
        }
        bytes_needed += ((READ_SRC2_VAL & key) ? NBYTE(inst.src2_val) : 0);
        static const size_t STATIC_INFO_SIZE = (0
            + NBYTE(static_info->src1)
            + NBYTE(static_info->src2)
            + NBYTE(static_info->dst)
            + NBYTE(static_info->mem_src1)
            + NBYTE(static_info->mem_src2)
            + NBYTE(static_info->mem_src3)
            + NBYTE(STATIC_INFO::BIT_FIELD_TYPE)
            + NBYTE(static_info->instruction_addr)
            + NBYTE(static_info->instruction_next_addr)
            + NBYTE(static_info->branch_target));
        bytes_needed += ((key & READ_STATIC_INFO) ? STATIC_INFO_SIZE : 0);
    
        // read the extra bytes needed for the first byte of the key
        if (fread(&buffer[1], sizeof(uint8_t), bytes_needed, stream) != bytes_needed)
            return /* failure */ false;
    
        // if the key is only one byte, we're done--go get the instruction
        if (/* 1-byte key */ (0 == (TWO_BYTE_KEY & key)))
            goto get_cbp_inst; 
    
        // get the second byte of the key
        key |= (KEY_TYPE(buffer[1]) << 8);
    
        // store the current buffer position in buffer_tail
        buffer_tail = (&buffer[1] + bytes_needed);
    
        // compute the number of extra bytes needed for the second byte of the key
        bytes_needed = ((READ_INSTRUCTION_ADDR & key) ? NBYTE(inst.instruction_addr) : 0);
        switch (MASK_BRANCH_TARGET & key) {
          case TYPE0_BRANCH_TARGET:   // 0 byte encoding
            break;
          case TYPE1_BRANCH_TARGET:   // 1 byte encoding
            bytes_needed += NBYTE(uint8_t);
            break;
          case TYPE2_BRANCH_TARGET:   // 2 byte encoding
            bytes_needed += NBYTE(uint16_t);
            break;
          case READ_BRANCH_TARGET:    // 4 byte encoding
            bytes_needed += NBYTE(inst.branch_target);
            break;
          default:
            CBP_FATAL("invalid key");
            break;
        }
        bytes_needed += ((READ_SRC1_VAL & key) ? NBYTE(inst.src1_val) : 0);
        bytes_needed += ((READ_VADDR2 & key) ? NBYTE(inst.dst_vaddr) : 0);
    
        // read the extra bytes needed for the second byte of the key
        if (fread(buffer_tail, sizeof(uint8_t), bytes_needed, stream) != bytes_needed)
            return /* failure */ false;
    
      get_cbp_inst:
        buffer_tail = ((TWO_BYTE_KEY & key) ? &buffer[2] : &buffer[1]);
    
        get_instruction_addr();
        get_static_info();
        get_src1_val();
        get_src2_val();
        get_dst_val();
        get_src_vaddr();
        get_dst_vaddr();
        get_taken();
        get_branch_target();
    
        *inst_arg = inst;
    
        update_statistics();
    
        return /* success */ true;
    }
    
    inline bool
    CBP_INST_STREAM::write(const CBP_INST* inst_arg)
    {
        buffer_tail = &buffer[2];   // leave room for the key
        key = 0;
        output_inst = inst_arg;
    
        put_instruction_addr();
        put_static_info();
        put_src1_val();
        put_src2_val();
        put_dst_val();
        put_src_vaddr();
        put_dst_vaddr();
        put_taken();
        put_branch_target();
    
        // compute the key size 
        uint8_t key_byte_0 = static_cast<uint8_t>(key);
        uint8_t key_byte_1 = static_cast<uint8_t>(key >> 8);
        if (/* 2-byte key */ (0 != key_byte_1)) {
            key |= TWO_BYTE_KEY;
            key_byte_0 = static_cast<uint8_t>(key);
        }
    
        // insert the key
        uint8_t* buffer_head = buffer;
        if (TWO_BYTE_KEY & key) {
            buffer_head[0] = key_byte_0;
            buffer_head[1] = key_byte_1;
        } else {
            ++buffer_head;
            buffer_head[0] = key_byte_0;
        }
    
        // write the buffer
        size_t buffer_size = (buffer_tail - buffer_head);
        bool success = (fwrite(buffer_head, sizeof(uint8_t), buffer_size, stream) == buffer_size);
    
        update_statistics();
    
        return success;
    }
    
    inline string
    CBP_INST_STREAM::get_statistics_string(void) const
    {
        ostringstream stream;
    
        stream << "----------------------------------------";
        stream << "----------------------------------------\n";
        stream << "CBP_INST              " << stat_cbp_inst << "\n";
        stream << "TWO_BYTE_KEY          " << stat_two_byte_key << "\n";
        stream << "TYPE0_DST_VAL         " << stat_type0_dst_val << "\n";
        stream << "TYPE1_DST_VAL         " << stat_type1_dst_val << "\n";
        stream << "TYPE2_DST_VAL         " << stat_type2_dst_val << "\n";
        stream << "READ_DST_VAL          " << stat_read_dst_val << "\n";
        stream << "TYPE0_VADDR1          " << stat_type0_vaddr1 << "\n";
        stream << "TYPE1_VADDR1          " << stat_type1_vaddr1 << "\n";
        stream << "TYPE2_VADDR1          " << stat_type2_vaddr1 << "\n";
        stream << "READ_VADDR1           " << stat_read_vaddr1 << "\n";
        stream << "READ_SRC2_VAL         " << stat_read_src2_val << "\n";
        stream << "FLIP_TAKEN            " << stat_flip_taken << "\n";
        stream << "READ_STATIC_INFO      " << stat_read_static_info << "\n";
        stream << "READ_INSTRUCTION_ADDR " << stat_read_instruction_addr << "\n";
        stream << "TYPE0_BRANCH_TARGET   " << stat_type0_branch_target << "\n";
        stream << "TYPE1_BRANCH_TARGET   " << stat_type1_branch_target << "\n";
        stream << "TYPE2_BRANCH_TARGET   " << stat_type2_branch_target << "\n";
        stream << "READ_BRANCH_TARGET    " << stat_read_branch_target << "\n";
        stream << "READ_SRC1_VAL         " << stat_read_src1_val << "\n";
        stream << "READ_VADDR2           " << stat_read_vaddr2 << "\n";
        stream << "----------------------------------------";
        stream << "----------------------------------------\n";

        return stream.str();
    }
    
        /* **************************************** */
    
    CBP_INST_STREAM*
    cbp_inst_open(FILE* stream)
    {
        return new CBP_INST_STREAM(stream);
    }
    
    FILE*
    cbp_inst_close(CBP_INST_STREAM* stream)
    {
        FILE* return_value = stream->get_stream();
        delete stream;
        return return_value;
    }
    
    bool
    cbp_inst_read(CBP_INST_STREAM* stream, CBP_INST* inst)
    {
        return stream->read(inst);
    }
    
    bool
    cbp_inst_write(CBP_INST_STREAM* stream, const CBP_INST* inst)
    {
        return stream->write(inst);
    }
    
    bool
    cbp_inst_print_statistics(FILE* stream, const CBP_INST_STREAM* cbp_inst_stream)
    {
        return (fprintf(stream, cbp_inst_stream->get_statistics_string().c_str()) >= 0);
    }
} // namespace cbp
    
