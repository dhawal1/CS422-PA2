#ifndef PREDICTOR_H_SEEN
#define PREDICTOR_H_SEEN

#include <cstddef>
#include <inttypes.h>
#include <vector>
#include "op_state.h"   // defines op_state_c (architectural state) class 
#include "tread.h"      // defines branch_record_c class
#include <map>
#include <set>
#include <cmath>
#include <iostream>
using namespace std;


class PREDICTOR
{
  public:
    typedef uint32_t address_t;

  private:
    typedef uint32_t history_t;
    typedef uint8_t counter_t;

    address_t h(address_t x){	//future use
    	return x;
    	address_t an = 1;
    	for(int i=0;i<32;i++){
    		uint32_t tmp=x%10;
    		x/=10;
    		an = tmp+ (an << 6) + (an << 16) - an;
    	}
    	return an;
    }

    //pred 1 - for non taken branches
    static const int BHR_LENGTH = 13; 
    static const int nsets = 1; 
    static const history_t BHR_MSB = (history_t(1) << (BHR_LENGTH - 1));
    static const std::size_t PHT_SIZE = (std::size_t(1) << BHR_LENGTH);
    static const std::size_t PHT_INDEX_MASK = (PHT_SIZE - 1);
    static const counter_t PHT_INIT = /* weakly taken */1;

    history_t bhr[nsets];            
    std::vector<counter_t> pht;//2^BHR_LENGTH;  

    void update_bhr(int setnum, bool taken) { bhr[setnum] >>= 1; if (taken) bhr[setnum] |= BHR_MSB; }
    static std::size_t pht_index(address_t pc, history_t bhr) 
        { return (static_cast<std::size_t>(pc ^ bhr) & PHT_INDEX_MASK); }
    static bool counter_msb(/* 2-bit counter */ counter_t cnt) { return (cnt >= 2); }
    static counter_t counter_inc(/* 2-bit counter */ counter_t cnt)
        { if (cnt != 3) ++cnt; return cnt; }
    static counter_t counter_dec(/* 2-bit counter */ counter_t cnt)
        { if (cnt != 0) --cnt; return cnt; }

    //pred 2 - for taken branches
    static const int BHR_LENGTH2 = 13; //15
    static const int nsets2 = 1;
    static const history_t BHR_MSB2 = (history_t(1) << (BHR_LENGTH2 - 1));
    static const std::size_t PHT_SIZE2 = (std::size_t(1) << BHR_LENGTH2);
    static const std::size_t PHT_INDEX_MASK2 = (PHT_SIZE2 - 1);
    static const counter_t PHT_INIT2 = /* weakly taken */ 1;

    std::vector<counter_t> pht2;//2^BHR_LENGTH2;  

    std::size_t pht_index2(address_t pc, history_t bhr) 
        { return (static_cast<std::size_t>(pc^ bhr) & PHT_INDEX_MASK2); }
    static bool counter_msb2(/* 2-bit counter */ counter_t cnt) { return (cnt >= 2); }
    static counter_t counter_inc2(/* 2-bit counter */ counter_t cnt)
        { if (cnt != 3) ++cnt; return cnt; }
    static counter_t counter_dec2(/* 2-bit counter */ counter_t cnt)
        { if (cnt != 0) --cnt; return cnt; }


    //bias
    static const int bias_cache_size = 4096*4+249;
    int bias_cache[bias_cache_size]; //will store just tag of 22 bits, not full pc

    void update_bias(address_t pc, bool taken){
    	int index = pc%bias_cache_size;
		int tmp2 = bias_cache[index];
		if(taken)tmp2++;
		else tmp2--;
		tmp2=max(0, min(3, tmp2));
		bias_cache[index] = tmp2;


    }
    bool predict_bias(address_t pc){
    	int index = pc%bias_cache_size;
    	return (bias_cache[index]>=2?true:false);
    }

    

  public:
    PREDICTOR(void)  { 
    	for(int i=0;i<nsets;i++){
    		bhr[i]=0;
    	}  
        for(std::size_t j=0;j<PHT_SIZE;j++)pht.push_back(counter_t(PHT_INIT));
        for(std::size_t j=0;j<PHT_SIZE2;j++)pht2.push_back(counter_t(PHT_INIT2));
        for(int i=0;i<bias_cache_size;i++)bias_cache[i]=1;
    }

    // uses compiler generated copy constructor
    // uses compiler generated destructor
    // uses compiler generated assignment operator

    // get_prediction() takes a branch record (br, branch_record_c is defined in
    // tread.h) and architectural state (os, op_state_c is defined op_state.h).
    // Your predictor should use this information to figure out what prediction it
    // wants to make.  Keep in mind you're only obligated to make predictions for
    // conditional branches.
    bool get_prediction(branch_record_c* br, const op_state_c* os)
        {
            bool prediction = false, prediction2 = false;
            if (/* conditional branch */ br->is_conditional) {
                address_t pc = br->instruction_addr;
				bool tmp = predict_bias(pc);

                std::size_t index = pht_index(pc, bhr[h(pc)%nsets]);
                counter_t cnt = pht[index];
                prediction = counter_msb(cnt);

				std::size_t index2 = pht_index2(pc, bhr[h(pc)%nsets2]);
                counter_t cnt2 = pht2[index2];
                prediction2 = counter_msb2(cnt2);

                bool final = tmp?prediction2:prediction;
                br->pred_value = 2*(int)tmp+(int)final;

                return final;
            }

            return prediction;   // true for taken, false for not taken
        }

    // Update the predictor after a prediction has been made.  This should accept
    // the branch record (br) and architectural state (os), as well as a third
    // argument (taken) indicating whether or not the branch was taken.
    void update_predictor(const branch_record_c* br, const op_state_c* os, bool taken)
        {
            if (/* conditional branch */ br->is_conditional) {
                address_t pc = br->instruction_addr;
				bool tmp = predict_bias(pc);

				int tmp2 = br->pred_value;
				int xx=tmp2/2, yy=tmp2%2;

				//when not to update bias table
				if(!((taken and !xx and yy) or (!taken and xx and !yy)))
                	update_bias(pc, taken);

                if(!tmp){
	                std::size_t index = pht_index(pc, bhr[h(pc)%nsets]);
	                counter_t cnt = pht[index];
	                if (taken)
	                    cnt = counter_inc(cnt);
	                else
	                    cnt = counter_dec(cnt);
	                pht[index] = cnt;
	            }
	            else{
	                std::size_t index2 = pht_index2(pc, bhr[h(pc)%nsets2]);
	                counter_t cnt2 = pht2[index2];
	                if (taken)
	                    cnt2 = counter_inc2(cnt2);
	                else
	                    cnt2 = counter_dec2(cnt2);
	                pht2[index2] = cnt2;
	            }

	            update_bhr(h(pc)%nsets, taken);
            }
        }
};

#endif // PREDICTOR_H_SEEN


