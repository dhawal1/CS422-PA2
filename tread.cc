/* Author: Chris Wilkerson;   Created: Thu Aug 12 16:19:58 PDT 2004
 * Description: This file defines a structure for holding the information for
 * one branch in a trace and also defines a trace reader class.
*/

#include "tread.h"
#include <cassert>
#include <cstring>
#include "op_state.h"

using namespace cbp;
using namespace std;

branch_record_c::branch_record_c(){
    instruction_addr      = 0x0;
    branch_target         = 0x0;
    instruction_next_addr = 0x0;
    is_indirect           = false;
    is_conditional        = false;
    is_call               = false;
    is_return             = false;
}
branch_record_c::~branch_record_c(){
}
void branch_record_c::init(){
    memset((char *)this, 0, sizeof(branch_record_c));
}
void branch_record_c::debug_print(){
    //printf("jp-op(%2x)t(%1x)lip(%8x)tar(%8x)nlip(%8x)num(%8x)\n", jump_class, tkn, lip, tar, nlip, num_insts);
}
//predictor apsi.cbp_inst.jz
cbp_trace_reader_c::cbp_trace_reader_c(char *trace_name){
    char trace_name_copy[256];
    char trnm_cmdline[256];
    memset((char *) trace_name_copy,  0, 256);
    memset((char *) trnm_cmdline,  0, 256);
    // we need the name the name of the trace 
    assert(trace_name);
    strcpy(trace_name_copy, trace_name);
    sprintf(trnm_cmdline, "bzip2 -dc %s.bz2", trace_name_copy);
    from_cbp_trace_file = popen(trnm_cmdline, "r");
    from_cbp_inst_stream = cbp_inst_open(from_cbp_trace_file);
    // initialize op_state
    osptr = new op_state_c();
    osptr->init(osptr);
    // initialize some scalars 
    is_branch_tkn             = false;
    predict_branch_tkn_copy   = false;
    predict_valid             = false;
    stat_num_branches         = 0;
    stat_num_cc_branches      = 0;
    stat_num_predicts         = 0;
    stat_num_correct_predicts = 0;
    stat_num_insts            = 0;

    memset(&cbp_inst, 0, sizeof(cbp_inst));
    cbp_inst.instruction_next_addr = 0;
    cbp_inst.is_indirect = false;
    cbp_inst.is_conditional = false;
    cbp_inst.is_call = false;
    cbp_inst.is_return = false;
    cbp_inst.branch_target = 0;
    cbp_inst.taken = false;
}

cbp_trace_reader_c::~cbp_trace_reader_c(){
    printf("*********************************************************\n");
    int   mis_preds     = (stat_num_cc_branches - stat_num_correct_predicts);
    float mis_pred_rate = float(mis_preds)/(float(stat_num_insts) / 1000);
    printf("1000*wrong_cc_predicts/total insts: 1000 * %8d / %8d = %7.3f\n", mis_preds, stat_num_insts, mis_pred_rate);
    printf("total branches:                  %8d\n", stat_num_branches);
    printf("total cc branches:               %8d\n", stat_num_cc_branches);
    printf("total predicts:                  %8d\n", stat_num_predicts);
    printf("*********************************************************\n");
    cbp_inst_close(from_cbp_inst_stream);
    pclose(from_cbp_trace_file);
    delete osptr;
}

bool cbp_trace_reader_c::predict_branch(bool predict_branch_tkn){
    if(predict_valid){
        printf("*******Multiple predictions made, you've called predict_branch more than once for the same branch!*******\n");
    }
    else{
        predict_valid           = true;
        predict_branch_tkn_copy = predict_branch_tkn;
    }
    return is_branch_tkn;
}


bool cbp_trace_reader_c::get_branch_record(branch_record_c *branch_record){
    if(stat_num_branches != 0){
        if(!predict_valid){
            if(branch_record->is_conditional){
                printf("*******No prediction made, you should at least try!*******\n");
                stat_num_predicts++;
            }
        }
        else{
            if(branch_record->is_conditional){
                stat_num_predicts++;
                if(predict_branch_tkn_copy == is_branch_tkn){ // correct prediction
                    stat_num_correct_predicts++;
                }
            }
        }
    }
    // init cbp_inst
    memset(&cbp_inst, 0, sizeof(cbp_inst));
    cbp_inst.instruction_next_addr = 0;
    cbp_inst.is_indirect = false;
    cbp_inst.is_conditional = false;
    cbp_inst.is_call = false;
    cbp_inst.is_return = false;
    cbp_inst.branch_target = 0;
    cbp_inst.taken = false;
    // populate the cbp_inst record
    op_record_c *op = 0;
    while(!cbp_inst.is_branch){
        if(feof(from_cbp_trace_file)){
            return false;
        }
        if(!cbp_inst_read(from_cbp_inst_stream, &cbp_inst)){
            return false;
        }
        osptr->inc_clock();
        uint opl_ptr = osptr->op_list_ptr;
        op = osptr->op_list + opl_ptr;
        if(op->is_valid){
            //commit op
            if(op->dst != REG_NUL){
                assert(op->dst < osptr->num_regs);
                osptr->regs[op->dst]       = op->get_dst_val();
                osptr->regs_valid[op->dst] = true;
            }
        }
        op->init();
        op->is_valid         = true;
        op->op_class         = cbp_inst.op_class;
        op->instruction_addr = cbp_inst.instruction_addr;
        op->is_load          = cbp_inst.is_load;
        op->is_store         = cbp_inst.is_store;
        op->is_branch        = cbp_inst.is_branch;
        op->is_op            = cbp_inst.is_op;
        op->is_fp            = cbp_inst.is_fp;
        op->read_flg         = cbp_inst.read_flg;
        op->writ_flg         = cbp_inst.writ_flg;
        op->src1             = cbp_inst.src1;
        op->src2             = cbp_inst.src2;
        op->dst              = cbp_inst.dst;
        op->has_mem_src      = cbp_inst.has_mem_src;
        op->has_mem_dst      = cbp_inst.has_mem_dst;
        op->mem_src1         = cbp_inst.mem_src1;
        op->mem_src2         = cbp_inst.mem_src2;
        op->mem_src3         = cbp_inst.mem_src3;
        op->set_src1_val(cbp_inst.src1_val);
        op->set_src2_val(cbp_inst.src2_val);
        op->set_dst_val(cbp_inst.dst_val);
        op->set_src_vaddr(cbp_inst.src_vaddr);
        op->set_dst_vaddr(cbp_inst.src_vaddr);
        stat_num_insts++;
        //op->debug_print();
    }
    assert(cbp_inst.is_branch);
    // cbp_inst has been populated 
    // set branch record
    branch_record->init();
    assert(op);
    assert(op->instruction_addr == cbp_inst.instruction_addr);
    branch_record->instruction_addr      = cbp_inst.instruction_addr;
    branch_record->branch_target         = cbp_inst.branch_target;
    branch_record->instruction_next_addr = cbp_inst.instruction_next_addr;
    branch_record->is_indirect           = cbp_inst.is_indirect;
    branch_record->is_conditional        = cbp_inst.is_conditional;
    branch_record->is_call               = cbp_inst.is_call;
    branch_record->is_return             = cbp_inst.is_return;
    is_branch_tkn                        = cbp_inst.taken;
    predict_valid                        = false;
    stat_num_branches++;
    if(branch_record->is_conditional){
        stat_num_cc_branches++;
    }
    //printf("jp-op t(%1x)lip(%8x)tar(%8x)nlip(%8x)\n", cbp_inst.taken, cbp_inst.instruction_addr, cbp_inst.branch_target, cbp_inst.instruction_next_addr);
    return true;
}









