/* Author: Chris Wilkerson;   Created: Thu Aug 12 16:19:58 PDT 2004
 * Description: Branch predictor driver.
*/

#include <cstdio>
#include <cstdlib>
#include "tread.h"

// include and define the predictor
#include "predictor.h"
PREDICTOR predictor;

// usage: predictor <trace>
int
main(int argc, char* argv[])
{
    using namespace std;

    if (2 != argc) {
        printf("usage: %s <trace>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    cbp_trace_reader_c cbptr = cbp_trace_reader_c(argv[1]);
    branch_record_c br;

    // read the trace, one branch at a time, placing the branch info in br
    while (cbptr.get_branch_record(&br)) {

        // ************************************************************
        // Competing predictors must have the following methods:
        // ************************************************************

        // get_prediction() returns the prediction your predictor would like to make
        bool predicted_taken = predictor.get_prediction(&br, cbptr.osptr);

        // predict_branch() tells the trace reader how you have predicted the branch
        bool actual_taken    = cbptr.predict_branch(predicted_taken);
            
        // finally, update_predictor() is used to update your predictor with the
        // correct branch result
        predictor.update_predictor(&br, cbptr.osptr, actual_taken);
    }
}



