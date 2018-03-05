/* Author: Chris Wilkerson;   Created: Thu Aug 12 16:19:58 PDT 2004
 * Description: This file defines a structure for holding the information for
 * one branch in a trace and also defines a trace reader class.
*/

#ifndef TREAD_H_SEEN
#define TREAD_H_SEEN

#include <cstdio>
#include "cbp_inst.h"

typedef unsigned int uint;

class op_record_c;
class op_state_c;

class branch_record_c
{
public:
    branch_record_c();
    ~branch_record_c();
    void init();                   // init the branch record
    void debug_print();            // print the information in the branch record (for debugging)
    uint   instruction_addr;       // the branch's PC (program counter)                      
    uint   branch_target;          // this is the target of the branch if it's taken; branches that aren't conditional are always taken
    uint   instruction_next_addr;  // the PC of the static instruction following the branch  
    bool   is_indirect;            // true if the target is computed; false if it's PC-rel; returns are also considered indirect
    bool   is_conditional;         // true if the branch is conditional; false otherwise     
    bool   is_call;                // true if the branch is a call; false otherwise          
    bool   is_return;              // true if the branch is a return; false otherwise       
    int pred_value; //where else should I add? 
};

class cbp_trace_reader_c
{
private:
    bool is_branch_tkn;                             // the holy grail, this should never be used in a predictor algorithm 
    bool predict_branch_tkn_copy;                   // the treader tucks away the prediction made by predictor    
    bool predict_valid;                             // is the current prediction in predict_branch_tkn_copy valid

    // stats 
    uint stat_num_branches;                         // stat that tracks the number of branches observed during trace processing           
    uint stat_num_insts;                            // stat that tracks the number insts executed during a trace
    uint stat_num_cc_branches;                      // stat that tracks the number of cc (conditional) branches observed during trace processing           
    uint stat_num_predicts;                         // stat that tracks the number of branches predicted during trace processing          
    uint stat_num_correct_predicts;                 // stat that tracks the number of branches correctly predicted during trace processing

    cbp::CBP_INST cbp_inst;
    std::FILE* from_cbp_trace_file; 
    cbp::CBP_INST_STREAM *from_cbp_inst_stream;

public:
    // op_state
    op_state_c *osptr;
    // cbp_trace_reader_c is passed a string specifying the name of the trace file
    cbp_trace_reader_c(char *trace_name);
    ~cbp_trace_reader_c();
    // call this to let the trace reader know what your prediction is; after it's called the prediction 
    // will get tucked away internally in predict_branch_tkn_copy and predict_valid is set; 
    // returns true if the branch is actually taken and false if the branch is actually not taken;
    // if the branch isn't conditional, this function behaves as a NOP and returns an undefined value
    // NOTE: you should only be able to call this once since the true branch direction is returned after it is called
    bool predict_branch(bool predict_branch_tkn);
    // returns true if there is still another branch record in the trace.  false if the end of the branch trace 
    // has been reached.
    bool get_branch_record(branch_record_c *branch_record); 
};

#endif // TREAD_H_SEEN

