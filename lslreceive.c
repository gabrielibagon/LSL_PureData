/*
* lsl_receive object for Pure Data.
*
* Captures an LSL stream from the network and outputs the values received.
*
* Adapted from Max/MSP modules by John Iverson, Grace Leslie, and Christian Kothe.
*
*
*/

#include "m_pd.h"      //pd header file
#include "lsl_c.h"     //LSL header file
#include <stdio.h>
#include <string.h>

#define MAX_NCHAN 2000          //some unreaonably large value
#define DEFAULT_STREAM_NAME "pd"
#define MAX_STREAM_NAME_LENGTH 200
#define DEFAULT_DATA_TYPE "string"
#define MAX_DATA_TYPE_LENGTH 32
#define POLLING_INTERVAL_MS 1   //poll stream this often (Q: is there any way to specify a callback?)
 

//typedef is used to give a type a new name
///seems like our type here is the _helloworld type

static t_class *lslreceive_class;

typedef struct _lslreceive{
	t_object x_obj;


	/* Stream Attributes */
	char  lsl_stream_name[MAX_STREAM_NAME_LENGTH]; /* Stream Name */
	lsl_streaminfo* info;					/*streaminfo returned by the lsl_resolve_byprop call*/
	
	lsl_channel_format_t lsl_channel_format;

	lsl_inlet inlet;
	int errcode;


	t_outlet *out_data, *out_timestamp; 	/* outlets */



	void * x_clock;
    char* cursample_string[MAX_NCHAN];     /* array to hold our current sample string*/
    float cursample_float[MAX_NCHAN];      /* array to hold our current sample float*/
    t_atom myList[MAX_NCHAN];
	
    int lsl_nchan;              /* number of channels in the stream (speacified when creating object) */
      /* name of stream */
    char data_type[MAX_STREAM_NAME_LENGTH]; /* ui specified data type */
	lsl_streaminfo lsl_info;	 //the streaminfo returned by the resolve call 
	lsl_inlet lsl_inlet;		/* a stream inlet to get samples from */
	int lsl_errcode;			/* error code (lsl_lost_error or timeouts) */
    float lsl_timestamp;		/* time stamp of the current sample (in sender time) */
    double lsl_local_timestamp; /* tim estamp of receipt in local time */

} t_lslreceive;



void *lslreceive_new(t_symbol* s, long argc, t_atom* argv);
void lslreceive_bang(t_lslreceive *x);
void lslreceive_free(t_lslreceive *x);
void lslreceive_assist(t_lslreceive* x, void* b, long m, long a, char* s);
void lslreceive_getSample(t_lslreceive *x);


 
void *lslreceive_new(t_symbol* s, long argc, t_atom* argv){
    t_lslreceive *x = (t_lslreceive *)pd_new(lslreceive_class);

    /*Stream name*/	
    if (argc>=1 && argv[0].a_type==A_SYMBOL){
    	strncpy(x->lsl_stream_name,atom_getsymbol(&argv[0])->s_name,MAX_STREAM_NAME_LENGTH);
    }else{
        strncpy(x->lsl_stream_name, DEFAULT_STREAM_NAME, MAX_STREAM_NAME_LENGTH);
    }
    post(x->lsl_stream_name);
	
    x->lsl_nchan = 1;

    post("Listening for an stream named '%s' with %d channels of %s...",x->lsl_stream_name,x->lsl_nchan, x->data_type);
	x->lsl_info = lsl_create_streaminfo("MyMarkerStream","Markers",x->lsl_nchan,0,cft_string,"");
	// char pred[256];

 //    int n_found = 0;
 //    post("finding stream");
    // while (~n_found) {
    //     n_found = lsl_resolve_byprop(&x->lsl_info, 1, "name", "MyMarkerStream", 1,LSL_FOREVER);
    // }
    // post("found %d",n_found);


    x->lsl_inlet = lsl_create_inlet(x->lsl_info, 300, LSL_NO_PREFERENCE, 1);
	x->lsl_channel_format = cft_string;

 	if (x->lsl_inlet) {

        x->out_timestamp = outlet_new(&x->x_obj, &s_float);
        x->out_data = outlet_new(&x->x_obj, &s_list);
        

        x->x_clock  = clock_new((t_object *)x, (t_method)lslreceive_getSample);
        clock_delay(x->x_clock, POLLING_INTERVAL_MS);

    } else {
        post("No matching stream was found. Be sure to specify the number of channels e.g. [lslreceive 10], and that it matches the source.");
    }


	return (void *)x;
}


// Load the "lslreceive" library
void lslreceive_setup(void) {  
  lslreceive_class = class_new(gensym("lslreceive"),  
							    (t_newmethod)lslreceive_new,  
							    (t_method)lslreceive_free,
							    sizeof(t_lslreceive),  
							    CLASS_DEFAULT,
							    A_GIMME,
							   	0);  
  	
  //bangs aren't really needed right now
  class_addbang(lslreceive_class, (t_method)lslreceive_bang);  
}


void lslreceive_bang(t_lslreceive *x){
	double timestamp;		/* time stamp of the current sample (in sender time) */
	char *cursample;		/* array to hold our current sample */

}

void lslreceive_getSample(t_lslreceive *x){
	int errcode;

    switch (x->lsl_channel_format) {
	    case cft_string:
	        x->lsl_timestamp = lsl_pull_sample_str(x->lsl_inlet,x->cursample_string,1,0.0,&errcode);
	        break;
	    case cft_float32:
	        x->lsl_timestamp = lsl_pull_sample_f(x->lsl_inlet,x->cursample_float,x->lsl_nchan,0,&errcode);
	        break;
	        
	    default:
	        x->lsl_timestamp = 0; //should never reach
	        break;
		}

	while (x->lsl_timestamp>0)	{
        //post("%f", x->lsl_timestamp);
		// post ("%s  %s  %s  %s  %s  %s  %s  %s",x->cursample[0],x->cursample[1],x->cursample[2],x->cursample[3],x->cursample[4],x->cursample[5],x->cursample[6],x->cursample[7]);

        
        // create list depending on data type received
        switch (x->lsl_channel_format) {
            case cft_string:
                // return list of strings, for flexibility, and consumer can use [fromsymbol] to convert to numbers
                for (int k=0; k < 1; ++k) {
                    SETSYMBOL(x->myList+k,gensym(x->cursample_string[k]));
                    
                }
                post("%s\n",gensym(*x->cursample_string));
                break;
                
            case cft_float32:
                // return list of strings, for flexibility, and consumer can use [fromsymbol] to convert to numbers
                for (int k=0; k < x->lsl_nchan; ++k) {
                    SETFLOAT(x->myList+k,x->cursample_float[k]);
                }
                break;
                
            default:
                break;
        }

        post("%f",x->lsl_timestamp);

        outlet_float(x->out_timestamp, x->lsl_timestamp);
		outlet_list(x->out_data,0L,x->lsl_nchan,x->myList);
        // post("%s\n",x->myList);
        
        switch (x->lsl_channel_format) {
            case cft_string:
                x->lsl_timestamp = lsl_pull_sample_str(x->lsl_inlet,x->cursample_string,x->lsl_nchan,0,&errcode);
                break;
                
            case cft_float32:
                x->lsl_timestamp = lsl_pull_sample_f(x->lsl_inlet,x->cursample_float,x->lsl_nchan,0,&errcode);
                break;
                
            default:
                x->lsl_timestamp = 0; //should never reach
                break;
        }
	}
	clock_delay(x->x_clock, POLLING_INTERVAL_MS);

}


void lslreceive_free(t_lslreceive* x)
{
	/* Do any deallocation needed here. */
    // clock_unset(x->m_clock);
    // object_free(x->m_clock);
    outlet_free(x->out_data);
    outlet_free(x->out_timestamp);	
    // lsl_destroy_inlet(x->lsl_inlet);
}

// void lslreceive_assist(t_lslreceive* x, void* b, long m, long a, char* s)
// {
// 	if (0) {
// 		switch (a) {
// 			case 0:
// 				sprintf(s, "Unused");
// 				break;
// 		}
// 	} 
// 	else {
// 		switch (a) {
// 			case 0:
// 				sprintf(s, "List of %d %s values received from LSL stream '%s'",x->lsl_nchan, x->data_type, x->lsl_stream_name);
// 				break;
// 			case 1:
// 				sprintf(s, "LSL timestamp (sender)");
// 				break;
// 			case 2:
// 				sprintf(s, "LSL timestamp (local)");
// 				break;
//                 /*
// 			case 3:
// 				sprintf(s, "Volume (1-128)");
// 				break;
// 			case 4:
// 				sprintf(s, "Azimuth (0-360) Clockwise from Northwest Corner");
// 				break;
// 			case 5:
// 				sprintf(s, "Loop On/Off (0/1)");
// 				break;
// 			case 6:
// 				sprintf(s, "On/Off (0/1) ");
// 				break;
// 			case 7:
// 				sprintf(s, "File Tag Number (0-9999)");
// 				break;
//                  */
// 		}		
// 	}
// }

// //polling function to pull a sample
// void lslreceive_getSample(t_lslreceive *x)
// {
// 	int errcode; //we don't do anything with this
    
//     switch (x->lsl_channel_format) {
//         case cft_string:
//             x->lsl_timestamp = lsl_pull_sample_str(x->lsl_inlet,x->cursample_string,x->lsl_nchan,0,&errcode);
//             break;
            
//         case cft_float32:
//             x->lsl_timestamp = lsl_pull_sample_f(x->lsl_inlet,x->cursample_float,x->lsl_nchan,0,&errcode);
//             break;
            
//         default:
//             x->lsl_timestamp = 0; //should never reach
//             break;
//     }

//     while (x->lsl_timestamp>0)	{
//         //post("%f", x->lsl_timestamp);
// 		//post ("%s  %s  %s  %s  %s  %s  %s  %s",x->cursample[0],x->cursample[1],x->cursample[2],x->cursample[3],x->cursample[4],x->cursample[5],x->cursample[6],x->cursample[7]);

        
//         // create list depending on data type received
       
//         switch (x->lsl_channel_format) {
//             case cft_string:
//                 // return list of strings, for flexibility, and consumer can use [fromsymbol] to convert to numbers
//                 for (int k=0; k < x->lsl_nchan; ++k) {
//                     atom_setsym(x->myList+k,gensym(x->cursample_string[k]));
//                 }
//                 break;
                
//             case cft_float32:
//                 // return list of strings, for flexibility, and consumer can use [fromsymbol] to convert to numbers
//                 for (int k=0; k < x->lsl_nchan; ++k) {
//                     atom_setfloat(x->myList+k,x->cursample_float[k]);
//                 }
//                 break;
                
//             default:
//                 break;
//         }

//         x->lsl_local_timestamp = lsl_local_clock(); //time of our output
// 		outlet_list(x->m_outlet[0],0L,x->lsl_nchan,x->myList);
//         outlet_float(x->m_outlet[1], x->lsl_timestamp);
//         outlet_float(x->m_outlet[2], x->lsl_local_timestamp);
        
//         switch (x->lsl_channel_format) {
//             case cft_string:
//                 x->lsl_timestamp = lsl_pull_sample_str(x->lsl_inlet,x->cursample_string,x->lsl_nchan,0,&errcode);
//                 break;
                
//             case cft_float32:
//                 x->lsl_timestamp = lsl_pull_sample_f(x->lsl_inlet,x->cursample_float,x->lsl_nchan,0,&errcode);
//                 break;
                
//             default:
//                 x->lsl_timestamp = 0; //should never reach
//                 break;
//         }
// 	}
	
// 	clock_fdelay(x->m_clock, POLLING_INTERVAL_MS);
// }

