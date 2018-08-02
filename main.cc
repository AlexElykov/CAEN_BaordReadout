// -------- Data Transfer Rate Tests for CAEN V1724 Boards Readout ------------//  

// Provided one has the libraries below, this code will read the buffer from   //
// a single or multiple V1724 boards depending on the settings.                //

// General libraries not all of them necessary needed:
#include <stdio.h>
#include <cstdio>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <iostream>
#include <stdlib.h>
#include <cstdlib> 
#include <stdint.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <chrono>    // ns precision clock
#include <algorithm> // for_each summation
// CAEN libraries
#include <CAENVMElib.h>
#include <CAENVMEtypes.h>

// The board VME base address
#define gBOARD_VME 0x80000000       

// Number of boards linked to the PCIe - set manually
#define NBoards 2  //1,2,3,4...etc   
#define gCHANNEL_MASK 0xFF             

using namespace std;

// This function writes user option to a register
int WriteRegister(u_int32_t reg, u_int32_t val, int handle){
  int ret = CAENVME_WriteCycle(handle,gBOARD_VME+reg,&val,cvA32_U_DATA,cvD32);
  if(ret!=cvSuccess)
    cout<<"Failed to write register "<<hex<<reg<<" with value "<<val<<
      ", returned value: "<<dec<<ret<<endl;
  else 
    cout<<"Write register "<<hex<<reg<<" with value "<<val<<dec<<endl;
  return ret;
}

// This function reads the set options from a register
int ReadRegister(u_int32_t reg, u_int32_t &val, int handle){
  int ret = CAENVME_ReadCycle(handle,gBOARD_VME+reg,&val,cvA32_U_DATA,cvD32);
  if(ret!=cvSuccess)
    cout<<"Failed to read register "<<hex<<reg<<" with return value: "<<
      dec<<ret<<endl;
  else
    cout<<"Read register "<<hex<<reg<<" with value "<<val<<dec<<endl;
  return ret;
}

// -----------
// Some functions to allow an infinite loop for continous iteration/display of signal
void changemode(int dir){
  static struct termios oldt, newt;

  if ( dir == 1 ){
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
    }
  else
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}

int kbhit (void){
  struct timeval tv;
  fd_set rdfs;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);

  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);
}


// Function to initialise the board/s 
int InitBoard(int link, int board){
  CVBoardTypes BType = cvV2718;
  int temphandle = -1;                          // handle for addressing the board
  int cerror;
  if((cerror=CAENVME_Init(BType,link,board,&temphandle))!=cvSuccess){
    stringstream therror;
    therror<<"Error in CAEN initialization link "
           <<link<<" crate "<<board<<": "<<cerror;
    cout<<therror.str()<<endl;
    return -1;
  }
  if(temphandle<0)
     return temphandle;

  // Write some options to the digitizer
  // Descriptions of registers might not be accurate ;p
  int handle = temphandle;
  int r=0;
  r+= WriteRegister(0xEF24, 0x1, handle);      // sw reset
  r+= WriteRegister(0xEF1C, 0x1, handle);      // Bit event number
  r+= WriteRegister(0xEF00, 0x10, handle);     // VME control
  r+= WriteRegister(0x8120, 0xFF, handle);     // Channel enable mask 
  r+= WriteRegister(0x8000, 0x310, handle);    // Channel configuration
  r+= WriteRegister(0x8080, 0x310000, handle); // dpp ctrl ?!?
  r+= WriteRegister(0x800C, 0xA, handle);      // Buffer access
  r+= WriteRegister(0x8098, 0x1000, handle);   // Channel offset
  r+= WriteRegister(0x8020, 0x258, handle);    // Custom size  0x32
  r+= WriteRegister(0x811C, 0x110, handle);    // Front panel control
  r+= WriteRegister(0x8034, 0x0, handle);      // Input delay
  r+= WriteRegister(0x8060, 0x3E8, handle);    // Trigger threshold   0x32
  r+= WriteRegister(0x8078, 0x12C, handle);    // N samples before threshold 0x19
  r+= WriteRegister(0x8100, 0x0, handle);      // Acquisition control
  r+= WriteRegister(0x8038, 0x12C, handle);    // Pre-trigger - defines the num of samples pre trigger 0x0
  r+= WriteRegister(0x81A0, 0x200, handle);    // Status of the LVDS I/O for Busy signal from the digitiser
  //r+= WriteRegister(0x810C, 0x80000000, handle);  // Trigger source/enable mask?!? 
  
  if (r!=0){
    cerr<<"Quitting since registers not written."<<endl;
    exit(-1);
    }
  return temphandle;
 }



// Function to read the data from board/s
// uncomment the lines in this function below to get rate for the CAENVME_FIFOBLTReadCycle
int GetData(vector<int> handles){
   unsigned int blt_bytes, buff_size=8388608, blt_size=8388608; //~ 8MB
   // Starting a chrono clock and writing to file 
   // std::ofstream DataFile("Readout.txt", std::ofstream::out | std::ofstream::app);
   // typedef std::chrono::high_resolution_clock Clock;
   // if(DataFile.is_open()){ 

     for(unsigned int ihandle=0; ihandle<handles.size(); ihandle++){      
       int handle = handles[ihandle]; 
       int nb = 0, ret = -5;
       u_int32_t *buff = new u_int32_t[buff_size];     
     do{
       // Reading the buffer
       ret = CAENVME_FIFOBLTReadCycle(handle,gBOARD_VME,((unsigned char*)buff)+blt_bytes,
                                                         blt_size,cvA32_U_BLT,cvD32,&nb);
       if((ret!=cvSuccess) && (ret!=cvBusError)){
        cout<<"Board read error: "<<ret<<" for board "<<handle<<endl;
        delete[] buff;
        return -1;
        }

       blt_bytes+=nb;    //bytes
 
       // Outputting: board number, data transferred, time taken to transfer data, rate
       // auto end_func = Clock::now(); 
       // double time_span =  std::chrono::duration_cast<std::chrono::nanoseconds>(end_func - start_func).count(); //ns
       // double rate = blt_bytes/time_span;    // Bytes/s 
       // DataFile << handle << "\t" << blt_bytes << "\t" << time_span << "\t" << rate*1000 << "\n";


       if(blt_bytes>buff_size){
         cout << "Buffer size too small for readout!" << endl;
         delete[] buff;
         return -1;
         }

       }while(ret!=cvBusError);  
         delete[] buff;
    } //end loop through handles (boards)
    //else{
      //std::cout << "Can't open file to write output!";
          //}DataFile.close();

    return blt_bytes;
    return 0;
}

// Stupid countdown timer
int timer(int second) {
    for (int i = 0; i < second; i++){
        sleep(1);                  //delay by one second
        const int remain_sec = second - i;
        cout << "Starting in: " << remain_sec << endl;
      }
    return 0;
}            

// --------------------------------------------- //
// ------------------ MAIN --------------------- //
// --------------------------------------------- //
int main(int argc, char *argv[], char *envp[]){
  cout << "Starting the program - press and KEY to STOP!" << endl; usleep(1000000);

  vector<int> links;
  vector<int> boards;
  vector<int> handles;

  // Countdown timer
  timer(1);
  
  // Looping through the boards/links in the crate
  // When addressing bords in a crate via always start from 0, 
  // as the boards are numbered [0:N] for each link
  for(unsigned int x=0;x<NBoards;x++){
    links.push_back(x);         //daisy chained link = 0, multiple boards with multi links link = x             
    boards.push_back(0);        //daisy chained boards = x, multiple boards with multi links boards = 0     
    }printf("Set up for %zu boards \n",boards.size());

  // Getting the handles of boards
  for(unsigned int x=0;x<links.size();x++){
    int bid = InitBoard(links[x], boards[x]);
    if(bid<0){
      printf("Exiting: A board could not be initilaised \n");
      exit(1);
     }
    handles.push_back(bid);
    }printf("Configured %zu board handles \n", handles.size());

  // Initialise the configured boards
  for(unsigned int ihandle=0; ihandle<handles.size(); ihandle++){
    WriteRegister(0x8100, 0x4, handles[ihandle]);
    }
  
  // Starting a chrono clock
  typedef std::chrono::high_resolution_clock Clock;
  typedef std::chrono::milliseconds ms;
  typedef std::chrono::duration<float> fsec; 
  auto start = Clock::now();

  unsigned int bytes;
  double time_span;
  double sum_of_elems;
  vector<unsigned int> tot_bytes;

  changemode(1);
  std::ofstream DataFile("link2.txt", std::ofstream::out | std::ofstream::app);

  // Infinite while loop to keep reading out the boards:
  while(!kbhit()){
 
        bytes = GetData(handles); 
        tot_bytes.push_back(bytes);
       
        // Simple clock logic to get total amount of data per second
        auto end = Clock::now();  
        fsec fs = end - start; 
        time_span = std::chrono::duration_cast<ms>(fs).count()/1e3;  // ms        

        if(time_span >= 1){
             std::for_each(tot_bytes.begin(), tot_bytes.end(), [&] (int n) {
                sum_of_elems += n;
                }); // end summation loop

           start = end;
           DataFile << sum_of_elems/1e6 << "\n";  
           
           //std::cout << "Tot transfer: "<< sum_of_elems/1e6 << endl;
           
           tot_bytes.erase(tot_bytes.begin(), tot_bytes.end());
           sum_of_elems = 0;

          }// end clock if loop
      }// end inf while loop

  // Stop acquisition, close txt, shut down boards & close links
  getchar();
  changemode(0);
  cout << "Stopping acquisition" << endl;
  DataFile.close();

  for(unsigned int ihandle=0; ihandle<handles.size(); ihandle++){ 
       WriteRegister(0x8100, 0x0, handles[ihandle]); //shut down boards
       CAENVME_End(handles[ihandle]);
    }

  exit(0);

} // end main



