#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "systemc.h"

#define VERBOSE 0

#define NOEDGE 255
#define POSSIBLE_EDGE 128
#define EDGE 0
#define BOOSTBLURFACTOR 90.0
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define SIGMA 0.6
#define TLOW  0.3
#define THIGH 0.8

#define COLS 2704
#define ROWS 1520
#define SIZE COLS*ROWS
#define VIDEONAME "Engineering_edges"
#define IMG_IN    "video/" VIDEONAME "%03d.pgm"
#define IMG_OUT   VIDEONAME "%03d.pgm"
#define IMG_NUM   30 /* number of images processed (1 or more) */
#define AVAIL_IMG 30 /* number of different image frames (1 or more) */
#define SET_STACK_SIZE set_stack_size(128*1024*1024);
/* upper bound for the size of the gaussian kernel
 *  *  *  *  *  *  *  *  *  * SIGMA must be less than 4.0
 *   *   *   *   *   *   *   *   *   * check for 'windowsize' below
 *    *    *    *    *    *    *    *    *    */
#define WINSIZE 21

typedef struct Image_s
{       
        unsigned char img[SIZE];
        
        Image_s(void)
        {  
           for (int i=0; i<SIZE; i++)
           {  
              img[i] = 0;
           }
        } 
 Image_s& operator=(const Image_s& copy)
        {  
           for (int i=0; i<SIZE; i++)
   {          
              img[i] = copy.img[i];
           }
           return *this;
        }
  operator unsigned char*()
        {
           return img;
        }

        unsigned char& operator[](const int index)
        {
           return img[index];
        }
} IMAGE;


typedef struct Image_i
{
        int img[SIZE];

        Image_i(void)
        {
           for (int i=0; i<SIZE; i++)
           {
              img[i] = 0;
           }
        }
 Image_i& operator=(const Image_i& copy)
        {
           for (int i=0; i<SIZE; i++)
   {
              img[i] = copy.img[i];
           }
           return *this;
        }

        operator int*()
        {
           return img;
        }

        int& operator[](const int index)
        {
           return img[index];
        }
} IIMAGE;



      typedef  struct Image_ss
{
        short int img[SIZE];

        Image_ss(void)
        {
           for (int i=0; i<SIZE; i++)
           {
 img[i] = 0;
           }
        }
 Image_ss& operator=(const Image_ss& copy)
        {
           for (int i=0; i<SIZE; i++)
   {
              img[i] = copy.img[i];
           }
           return *this;
        }

        operator short int*()
        {
           return img;
        }

        short int& operator[](const int index)
        {
           return img[index];
        }
} SIMAGE;

typedef  struct Image_sss
{
        float img[SIZE];

        Image_sss(void)
        {
           for (int i=0; i<SIZE; i++)
           {
              img[i] = 0;
           }
        }
 Image_sss& operator=(const Image_sss& copy)
        {
           for (int i=0; i<SIZE; i++)
   {
              img[i] = copy.img[i];
           }
           return *this;
        }

        operator float*()
        {
           return img;
        }

        float& operator[](const int index)
        {
           return img[index];
        }
} FIMAGE;

SC_MODULE(Stimulus){
        IMAGE imageout;
        sc_fifo_out<IMAGE> ImgOut;
        sc_fifo_out<sc_time> stimtime;
      sc_time stim;
 int read_pgm_image(const char *infilename, unsigned char *image, int rows, int cols)
        {
           FILE *fp;
           char buf[71];
           int r, c;
           if(infilename == NULL) fp = stdin;
           else{
              if((fp = fopen(infilename, "r")) == NULL){
                 fprintf(stderr, "Error reading the file %s in read_pgm_image().\n",
                    infilename);
                 return(0);
              }
           }
           fgets(buf, 70, fp);
           if(strncmp(buf,"P5",2) != 0){
              fprintf(stderr, "The file %s is not in PGM format in ", infilename);
              fprintf(stderr, "read_pgm_image().\n");
              if(fp != stdin) fclose(fp);
              return(0);
           }
           do{ fgets(buf, 70, fp); }while(buf[0] == '#');  /* skip all comment lines */
           sscanf(buf, "%d %d", &c, &r);
           if(c != cols || r != rows){
              fprintf(stderr, "The file %s is not a %d by %d image in ", infilename, cols, rows);
              fprintf(stderr, "read_pgm_image().\n");
              if(fp != stdin) fclose(fp);
              return(0);
           }
 do{ fgets(buf, 70, fp); }while(buf[0] == '#');
            if((unsigned)rows != fread(image, cols, rows, fp)){
              fprintf(stderr, "Error reading the image data in read_pgm_image().\n");
              if(fp != stdin) fclose(fp);
              return(0);
           }

           if(fp != stdin) fclose(fp);
           return(1);
        sc_stop();
        }

        void main(void)
        {
           
           int i=0, n=0;
           char infilename[40];

           for(i=0; i<IMG_NUM; i++)
           {
              n = i % AVAIL_IMG;
              sprintf(infilename, IMG_IN, n+1);
              stim=sc_time_stamp();
            cout<< stim<<": Stimulus sent frame"<<i+1<<".\n";
              if(VERBOSE) printf("Reading the image %s.\n", infilename);
              if(read_pgm_image(infilename, imageout, ROWS, COLS) == 0){
              fprintf(stderr, "Error reading the input image, %s.\n", infilename);
                 exit(1);
              }
              ImgOut.write(imageout);
              stimtime.write(stim);
           }
        }

        SC_CTOR(Stimulus)
        {
           SC_THREAD(main);
           set_stack_size(128*1024*1024);
        }
};

SC_MODULE(Monitor){
        IMAGE imagein;
        sc_fifo_in<IMAGE> ImgIn;
        sc_fifo_in<sc_time> stimtimein;
       sc_time stimin;
    sc_time mon;
double fps;
    sc_time monprev;
        int write_pgm_image(const char *outfilename, unsigned char *image, int rows,
            int cols, const char *comment, int maxval)
        {
           FILE *fp;
if(outfilename == NULL) fp = stdout;
           else{
              if((fp = fopen(outfilename, "w")) == NULL){
               fprintf(stderr, "Error writing the file %s in write_pgm_image().\n",
                    outfilename);
                 return(0);
              }
           }
fprintf(fp, "P5\n%d %d\n", cols, rows);
           if(comment != NULL)
              if(strlen(comment) <= 70) fprintf(fp, "# %s\n", comment);
           fprintf(fp, "%d\n", maxval);
           if((unsigned)rows != fwrite(image, cols, rows, fp)){
              fprintf(stderr, "Error writing the image data in write_pgm_image().\n");
              if(fp != stdout) fclose(fp);
              return(0);
           }

           if(fp != stdout) fclose(fp);
           return(1);
        }

        void main(void)
        {
           char outfilename[128];    /* Name of the output "edge" image */
           int i, n;
/**static sc_time monprev=0;**/            

           for(i=0; i<IMG_NUM; i++)
           {
              ImgIn.read(imagein);
              stimtimein.read(stimin);

n = i % AVAIL_IMG;
              sprintf(outfilename, IMG_OUT, n+1);
               mon=sc_time_stamp();
              cout<< mon<<": Monitor received frame"<<i+1<<" with "<<mon-stimin<< " delay\n";
/**       monprev=monprev-mon;**/
fps=mon.to_seconds()-monprev.to_seconds();
              cout<<mon<<" : "<<fps<< "seconds after previous frame, "<<1/fps << "FPS.\n";
monprev=mon;         

  if(VERBOSE) printf("Writing the edge image in the file %s.\n", outfilename);
              
              if(write_pgm_image(outfilename, imagein, ROWS, COLS,"", 255) == 0){
                 fprintf(stderr, "Error writing the edge image, %s.\n", outfilename);
                 exit(1);
              }
           }
           if(VERBOSE) printf("Monitor exits simulation.\n");
              sc_stop();        /**done testing, quit the simulation**/
        }

        SC_CTOR(Monitor)
        {
           SC_THREAD(main);
           set_stack_size(128*1024*1024);
        }
};
SC_MODULE(DataIn)
{
        sc_fifo_in<IMAGE> ImgIn;
        sc_fifo_out<IMAGE> ImgOut;
        IMAGE Image;

        void main()
        {
           while(1)
           {
              ImgIn.read(Image);
              ImgOut.write(Image);
           }
        }

        SC_CTOR(DataIn)
        {
           SC_THREAD(main);
           set_stack_size(128*1024*1024);
        }
};
SC_MODULE(DataOut)
{
        sc_fifo_in<IMAGE> ImgIn;
        sc_fifo_out<IMAGE> ImgOut;
        IMAGE Image;

        void main()
        {
           while(1)
           {
              ImgIn.read(Image);
              ImgOut.write(Image);
           }
        }

        SC_CTOR(DataOut)
        {
           SC_THREAD(main);
           set_stack_size(128*1024*1024);
        }
};

SC_MODULE(Receive_image)
{
        sc_fifo_in<IMAGE> ImgIn;
        sc_fifo_out<IMAGE> ImgOut;
        IMAGE Image;
                     void main()
        {
           while(1)
           {
              ImgIn.read(Image);
              ImgOut.write(Image);
           }
        }

        SC_CTOR(Receive_image)
        {
           SC_THREAD(main);
          set_stack_size(128*1024*1024);
        }
};
SC_MODULE(gaussian_kernal)
{
        sc_fifo_out<FIMAGE> Imgkernal;
        sc_fifo_out<IIMAGE> Imgwindowsize;
    sc_fifo_in<IMAGE> ImgIn;
    sc_fifo_out<IMAGE> ImgOut;
    IMAGE ImageIn;
    IMAGE ImageOut;
        FIMAGE kernal;
        IIMAGE windowsize;

        void make_gaussian_kernel(float sigma, float *kernel, int *windowsize)
        {

           int i, center;
           float x, fx, sum=0.0;

           *windowsize = 1 + 2 * ceil(2.5 * sigma);
           center = (*windowsize) / 2;

           if(VERBOSE) printf("      The kernel has %d elements.\n", *windowsize);

           for(i=0;i<(*windowsize);i++){
              x = (float)(i - center);
              fx = pow(2.71828, -0.5*x*x/(sigma*sigma)) / (sigma * sqrt(6.2831853));
              kernel[i] = fx;
              sum += fx;
           }
for(i=0;i<(*windowsize);i++) kernel[i] /= sum;

  if(VERBOSE){
              printf("The filter coefficients are:\n");
              for(i=0;i<(*windowsize);i++)
                printf("kernel[%d] = %f\n", i, kernel[i]);
           }
        }
        void main(void)
        {
           while(1)
           {
               ImgIn.read(ImageIn);
              if(VERBOSE) printf("   Computing the gaussian smoothing kernel.\n");
           make_gaussian_kernel(SIGMA, kernal, windowsize);
           wait(0.1997, SC_MS);
           Imgkernal.write(kernal);
           Imgwindowsize.write(windowsize);
               ImgOut.write(ImageOut);

           }
        }

        SC_CTOR(gaussian_kernal)
        {
           SC_THREAD(main);
           set_stack_size(128*1024*1024);
        }
};
SC_MODULE(Blur_x)
{
       sc_event e1,e2,e3,e4,data_received;
       
        sc_fifo_in<IMAGE> ImgIn;
        sc_fifo_in<FIMAGE> Imgkernal;
        sc_fifo_in<IIMAGE> Imgwindowsize;
        sc_fifo_out<FIMAGE> Imgtempim;
sc_fifo_out<FIMAGE> Imgkernaly;
sc_fifo_out<IIMAGE> Imgwindowsizey;
    /**sc_fifo_out<IMAGE> ImgOut;**/
FIMAGE kernaly;
IIMAGE windowsizey;
        IMAGE image;
  /**  IMAGE ImageOut;**/
        FIMAGE kernel;
        IIMAGE windowsize;
        FIMAGE tempim;

        void blur_x1()
        {
 while(1)
{
       wait(data_received);
        int r, c,rr, cc,center;
         float  dot,            /* Dot product summing variable. */
         sum;            /* Sum of the kernel weights variable. */
center = (*windowsize) / 2;/**center = (*windowsize) / 2;**/
      
 if(VERBOSE) printf("   Bluring the image in the X-direction.\n");
   for(r=0;r<ROWS/4;r++){
      for(c=0;c<COLS;c++){
         dot = 0.0;
         sum = 0.0;
         for(cc=(-center);cc<=center;cc++){
          if(((c+cc) >= 0) && ((c+cc) < COLS)){
               dot += (float)image[r*COLS+(c+cc)] * kernel[center+cc];
               sum += kernel[center+cc];
            }  
         }  
         tempim[r*COLS+c] = dot/sum;
         
   }}
   wait(256.93/4, SC_MS);
   e1.notify(SC_ZERO_TIME);
}   }  
   
   void blur_x2()
        {
 while(1)
{
       wait(data_received);
        int r, c,rr, cc,center;
         float  dot,            /* Dot product summing variable. */
         sum;            /* Sum of the kernel weights variable. */
center = (*windowsize) / 2;/**center = (*windowsize) / 2;**/
   
   if(VERBOSE) printf("   Bluring the image in the X-direction.\n");
   for(r=ROWS/4;r<ROWS/2;r++){
      for(c=0;c<COLS;c++){
         dot = 0.0;
         sum = 0.0;
         for(cc=(-center);cc<=center;cc++){
          if(((c+cc) >= 0) && ((c+cc) < COLS)){
               dot += (float)image[r*COLS+(c+cc)] * kernel[center+cc];
               sum += kernel[center+cc];
            }  
         }  
         tempim[r*COLS+c] = dot/sum;
         
   }}
   wait(256.93/4, SC_MS);
   e2.notify(SC_ZERO_TIME);
}   }  
   
   
   void blur_x3()
        {
 while(1)
{
       wait(data_received);
        int r, c,rr, cc,center;
         float  dot,            /* Dot product summing variable. */
         sum;            /* Sum of the kernel weights variable. */
center = (*windowsize) / 2;/**center = (*windowsize) / 2;**/
    
if(VERBOSE) printf("   Bluring the image in the X-direction.\n");
   for(r=ROWS/2;r<(ROWS/4)*3;r++){
      for(c=0;c<COLS;c++){
         dot = 0.0;
         sum = 0.0;
         for(cc=(-center);cc<=center;cc++){
          if(((c+cc) >= 0) && ((c+cc) < COLS)){
               dot += (float)image[r*COLS+(c+cc)] * kernel[center+cc];
               sum += kernel[center+cc];
            }  
         }  
         tempim[r*COLS+c] = dot/sum;
         
   }}
   wait(256.93/4, SC_MS);
   e3.notify(SC_ZERO_TIME);
   }  
}   
   void blur_x4()        {
       
while(1)
{
 wait(data_received);
        int r, c,rr, cc,center;
         float  dot,            /* Dot product summing variable. */
         sum;            /* Sum of the kernel weights variable. */
center = (*windowsize) / 2;/**center = (*windowsize) / 2;**/

       if(VERBOSE) printf("   Bluring the image in the X-direction.\n");
   for(r=(ROWS/4)*3;r<ROWS;r++){
      for(c=0;c<COLS;c++){
         dot = 0.0;
         sum = 0.0;
         for(cc=(-center);cc<=center;cc++){
          if(((c+cc) >= 0) && ((c+cc) < COLS)){
               dot += (float)image[r*COLS+(c+cc)] * kernel[center+cc];
               sum += kernel[center+cc];
            }  
         }  
         tempim[r*COLS+c] = dot/sum;
         
   }}
   wait(256.93/4, SC_MS);
   e4.notify(SC_ZERO_TIME);
 }  }  
   
   
    
   void main(void)
        {
           while(1)
           {
           ImgIn.read(image);
              Imgkernal.read(kernel);
              Imgwindowsize.read(windowsize);
          /** blur_x(ImageIn,kernal, windowsize,tempim);**/
           data_received.notify(SC_ZERO_TIME);
wait(e1&e2&e3&e4);
           Imgtempim.write(tempim);
Imgkernaly.write(kernel);
Imgwindowsizey.write(windowsize);
              /** ImgOut.write(ImageOut);**/

           }
        }  
        
        SC_CTOR(Blur_x)
        {
           SC_THREAD(main);
           set_stack_size(128*1024*1024);
           SC_THREAD(blur_x1);
           set_stack_size(128*1024*1024);
           SC_THREAD(blur_x2);
           set_stack_size(128*1024*1024);
           SC_THREAD(blur_x3);
           set_stack_size(128*1024*1024);
           SC_THREAD(blur_x4);
           set_stack_size(128*1024*1024);
        }  
};      

SC_MODULE(Blur_y)
{
sc_event e1,e2,e3,e4,data_received; 
       sc_fifo_in<FIMAGE> Imgkernaly;
        sc_fifo_in<IIMAGE> Imgwindowsizey;
        sc_fifo_in<FIMAGE> Imgtempimy;
    
        sc_fifo_out<SIMAGE> Imgsmoothdimy;
        FIMAGE kernel;
        IIMAGE windowsize;
        FIMAGE tempim;
        SIMAGE smoothedim;
    

        void blur_y1()       {
while(1)
{
wait(data_received); 
        int r, c,rr, cc,center;
         float  dot,            /* Dot product summing variable. */
         sum;            /* Sum of the kernel weights variable. */
center = (*windowsize) / 2;
 if(VERBOSE) printf("   Bluring the image in the Y-direction.\n");
   for(c=0;c<COLS/4;c++){
      for(r=0;r<ROWS;r++){
         sum = 0.0;
         dot = 0.0;
         for(rr=(-center);rr<=center;rr++){
            if(((r+rr) >= 0) && ((r+rr) < ROWS)){
               dot += tempim[(r+rr)*COLS+c] * kernel[center+rr];
                sum += kernel[center+rr];
            }
         }
         smoothedim[r*COLS+c] = (short int)(dot*BOOSTBLURFACTOR/sum + 0.5);
      }
   }
wait(473.28/4, SC_MS);
e1.notify(SC_ZERO_TIME);
}}
 

void blur_y2()       {

while(1)
{
wait(data_received);
        int r, c,rr, cc,center;
         float  dot,            /* Dot product summing variable. */
         sum;            /* Sum of the kernel weights variable. */
center = (*windowsize) / 2;
 if(VERBOSE) printf("   Bluring the image in the Y-direction.\n");
   for(c=COLS/4;c<COLS/2;c++){
      for(r=0;r<ROWS;r++){
         sum = 0.0;
         dot = 0.0;
         for(rr=(-center);rr<=center;rr++){
            if(((r+rr) >= 0) && ((r+rr) < ROWS)){
               dot += tempim[(r+rr)*COLS+c] * kernel[center+rr];
                sum += kernel[center+rr];
            }
         }
         smoothedim[r*COLS+c] = (short int)(dot*BOOSTBLURFACTOR/sum + 0.5);
      }
   }
wait(473.28/4, SC_MS);
e2.notify(SC_ZERO_TIME);
}
}
void blur_y3()
       {
while(1)
{
wait(data_received);
        int r, c,rr, cc,center;
         float  dot,            /* Dot product summing variable. */
         sum;            /* Sum of the kernel weights variable. */
center = (*windowsize) / 2;
 if(VERBOSE) printf("   Bluring the image in the Y-direction.\n");
   for(c=COLS/2;c<(COLS/4)*3;c++){
      for(r=0;r<ROWS;r++){
         sum = 0.0;
         dot = 0.0;
         for(rr=(-center);rr<=center;rr++){
            if(((r+rr) >= 0) && ((r+rr) < ROWS)){
               dot += tempim[(r+rr)*COLS+c] * kernel[center+rr];
                sum += kernel[center+rr];
            }
         }
         smoothedim[r*COLS+c] = (short int)(dot*BOOSTBLURFACTOR/sum + 0.5);
      }
   }
wait(473.28/4, SC_MS);
e3.notify(SC_ZERO_TIME);
}
}
void blur_y4()  
     {
while(1)
{
wait(data_received);
        int r, c,rr, cc,center;
         float  dot,            /* Dot product summing variable. */
         sum;            /* Sum of the kernel weights variable. */
center = (*windowsize) / 2;
 if(VERBOSE) printf("   Bluring the image in the Y-direction.\n");
   for(c=(COLS/4)*3;c<COLS;c++){
      for(r=0;r<ROWS;r++){
         sum = 0.0;
         dot = 0.0;
         for(rr=(-center);rr<=center;rr++){
            if(((r+rr) >= 0) && ((r+rr) < ROWS)){
               dot += tempim[(r+rr)*COLS+c] * kernel[center+rr];
                sum += kernel[center+rr];
            }
         }
         smoothedim[r*COLS+c] = (short int)(dot*BOOSTBLURFACTOR/sum + 0.5);
      }
   }
wait(473.28/4, SC_MS);
e4.notify(SC_ZERO_TIME);
}}


  void main(void)
        {
           while(1)
           {
              Imgkernaly.read(kernel);
              Imgwindowsizey.read(windowsize);
              Imgtempimy.read(tempim);
 /**          blur_y(kernaly, windowsizey,tempimy,smoothdimy);
 *  *  *  *            wait(2, SC_SEC);**/

           Imgsmoothdimy.write(smoothedim);


           }
        }

        SC_CTOR(Blur_y)
        {
           SC_THREAD(main);
           set_stack_size(128*1024*1024);
SC_THREAD(blur_y1);
           set_stack_size(128*1024*1024);
           SC_THREAD(blur_y2);
           set_stack_size(128*1024*1024);
           SC_THREAD(blur_y3);
           set_stack_size(128*1024*1024);
           SC_THREAD(blur_y4);
           set_stack_size(128*1024*1024);       

 }
};

   SC_MODULE(Gaussian_smooth)
{
        sc_fifo_in<IMAGE> ImgIn;
        sc_fifo_out<SIMAGE> ImgOut;
        sc_fifo<IMAGE> q1;
        sc_fifo<IMAGE> q2;
         sc_fifo<FIMAGE> q3;
          sc_fifo<IIMAGE> q4;
           sc_fifo<FIMAGE> q5;
            sc_fifo<IIMAGE> q6;
    sc_fifo<FIMAGE> q7;
             Receive_image recev;
        gaussian_kernal gk;
        Blur_x bx;
        Blur_y by;

        void before_end_of_elaboration(){
           recev.ImgIn.bind(ImgIn);
           recev.ImgOut.bind(q1);
            gk.ImgIn.bind(q1);
            gk.ImgOut.bind(q2);
            bx.ImgIn.bind(q2);
            gk.Imgkernal.bind(q3);
            bx.Imgkernal.bind(q3);
            gk.Imgwindowsize.bind(q4);
            bx.Imgwindowsize.bind(q4);
            bx.Imgkernaly.bind(q5);
            by.Imgkernaly.bind(q5);
            bx.Imgwindowsizey.bind(q6);
            by.Imgwindowsizey.bind(q6);
            bx.Imgtempim.bind(q7);
            by.Imgtempimy.bind(q7);
           by.Imgsmoothdimy.bind(ImgOut);
        }

        SC_CTOR(Gaussian_smooth)
        :q1("q1",1)
        ,q2("q2",1)
        ,q3("q3",1)
        ,q4("q4",1)
        ,q5("q5",1)
        ,q6("q6",1)
    ,q7("q7",1)
        ,recev("recev")
        ,gk("gk")
        ,bx("bx")
        ,by("by")
        {}
};

SC_MODULE(derivative)
{
        sc_fifo_in<SIMAGE> Imgsmoothdim;
        sc_fifo_out<SIMAGE> Imgdeltax;
        sc_fifo_out<SIMAGE> Imgdeltay;
        SIMAGE delta_x;
        SIMAGE delta_y;
        SIMAGE smoothdim;
        
        int derivative_x_y(short int *smoothedim, int rows, int cols,
                short int *delta_x, short int *delta_y)
                 {
           int r, c, pos;
if(VERBOSE) printf("   Computing the X-direction derivative.\n");
           for(r=0;r<rows;r++){
              pos = r * cols;
              delta_x[pos] = smoothedim[pos+1] - smoothedim[pos];
              pos++;
              for(c=1;c<(cols-1);c++,pos++){
                 delta_x[pos] = smoothedim[pos+1] - smoothedim[pos-1];
              }
              delta_x[pos] = smoothedim[pos] - smoothedim[pos-1];
           }
if(VERBOSE) printf("   Computing the Y-direction derivative.\n");
           for(c=0;c<cols;c++){
              pos = c;
              delta_y[pos] = smoothedim[pos+cols] - smoothedim[pos];
              pos += cols;
              for(r=1;r<(rows-1);r++,pos+=cols){
                 delta_y[pos] = smoothedim[pos+cols] - smoothedim[pos-cols];
                  }
              delta_y[pos] = smoothedim[pos] - smoothedim[pos-cols];
           }
        }
        void main(void)
        {
           while(1)
           {
              Imgsmoothdim.read(smoothdim);
              if(VERBOSE) printf("Computing the X and Y first derivatives.\n");
           derivative_x_y(smoothdim, ROWS, COLS, delta_x, delta_y);
           wait(145.95, SC_MS);
           Imgdeltax.write(delta_x);
           Imgdeltay.write(delta_y);
           }
        }

        SC_CTOR(derivative)
        {
           SC_THREAD(main);
           set_stack_size(128*1024*1024);
        }
};

SC_MODULE(magnitude)
{
        sc_fifo_in<SIMAGE> Imgdeltay;
        sc_fifo_in<SIMAGE> Imgdeltax;
         sc_fifo_out<SIMAGE> Imgmag1;/**output channel magnitude value to non_maxx_supp**/
sc_fifo_out<SIMAGE> Imggradx;
sc_fifo_out<SIMAGE> Imggrady;
/***sc_fifo_out<SIMAGE> Imgmag2;for apply hysteresis mag value passing**/
        SIMAGE deltax;
        SIMAGE deltay;

        SIMAGE mag1;/**output port magnitude value to non_maxx_supp**/

        void magnitude_x_y(short int *delta_x, short int *delta_y, int rows, int cols,
                short int *magnitude)
        {
           int r, c, pos, sq1, sq2;

           for(r=0,pos=0;r<rows;r++){
              for(c=0;c<cols;c++,pos++){
                 sq1 = (int)delta_x[pos] * (int)delta_x[pos];
                 sq2 = (int)delta_y[pos] * (int)delta_y[pos];
                 magnitude[pos] = (short)(0.5 + sqrt((float)sq1 + (float)sq2));
              }
                }

        }
        void main(void)
        {
           while(1)
           {
              Imgdeltax.read(deltax);
              Imgdeltay.read(deltay);
              if(VERBOSE) printf("Computing the magnitude of the gradient.\n");
           magnitude_x_y(deltax, deltay, ROWS, COLS, mag1);
wait(152.80, SC_MS);
           Imgmag1.write(mag1);
           Imggradx.write(deltax);
Imggrady.write(deltay);
/**Imgmag2.write(mag1);**/

  }
        }

        SC_CTOR(magnitude)
        {
           SC_THREAD(main);
           set_stack_size(128*1024*1024);
        }
};
SC_MODULE(non_max_sup)
{
        sc_fifo_in<SIMAGE> Imgmag;
        sc_fifo_in<SIMAGE> Imggradx;
        sc_fifo_in<SIMAGE> Imggrady;
        sc_fifo_out<IMAGE> Imgresult;
        sc_fifo_out<SIMAGE> Imgmagout;
        SIMAGE mag;
        SIMAGE magout;       
        SIMAGE gradx;
        SIMAGE grady;
        IMAGE nms;

  void non_max_supp(short *mag, short *gradx, short *grady, int nrows, int ncols
  , unsigned char *result)       {
   int rowcount, colcount,count;
            short *magrowptr,*magptr;
            short *gxrowptr,*gxptr;
            short *gyrowptr,*gyptr,z1,z2;
            short m00,gx,gy;
            int  mag1,mag2,xperp,yperp;
            unsigned char *resultrowptr, *resultptr;
             for(count=0,resultrowptr=result,resultptr=result+ncols*(nrows-1);
                count<ncols; resultptr++,resultrowptr++,count++){
                *resultrowptr = *resultptr = (unsigned char) 0;
            }

            for(count=0,resultptr=result,resultrowptr=result+ncols-1;
                count<nrows; count++,resultptr+=ncols,resultrowptr+=ncols){
                *resultptr = *resultrowptr = (unsigned char) 0;
            }
            for(rowcount=1,magrowptr=mag+ncols+1,gxrowptr=gradx+ncols+1,
              gyrowptr=grady+ncols+1,resultrowptr=result+ncols+1;
              rowcount<=nrows-2;        
              rowcount++,magrowptr+=ncols,gyrowptr+=ncols,gxrowptr+=ncols,
              resultrowptr+=ncols){
              for(colcount=1,magptr=magrowptr,gxptr=gxrowptr,gyptr=gyrowptr,
                 resultptr=resultrowptr;colcount<=ncols-2;      
                 colcount++,magptr++,gxptr++,gyptr++,resultptr++){
                 m00 = *magptr;
                 if(m00 == 0){
                    *resultptr = (unsigned char) NOEDGE;
                 }
                 else{
                   /**xperp = -(gx = *gxptr)/((float)m00);
                  yperp = (gy = *gyptr)/((float)m00);**/
                 gx=*gxptr;
gy=*gyptr;
xperp=-(gx<<16)/m00;
yperp=(gy<<16)/m00;
                 }

                 if(gx >= 0){
                    if(gy >= 0){
                            if (gx >= gy)
                             {
                                /* 111 */
                                /* Left point */
                                z1 = *(magptr - 1);
                                z2 = *(magptr - ncols - 1);

                                mag1 = (m00 - z1)*xperp + (z2 - z1)*yperp;

                                /* Right point */
                                z1 = *(magptr + 1);
                                z2 = *(magptr + ncols + 1);

                                mag2 = (m00 - z1)*xperp + (z2 - z1)*yperp;
                            }
                            else
                            {
                                /* 110 */
                                /* Left point */
                                z1 = *(magptr - ncols);
                                z2 = *(magptr - ncols - 1);

                                mag1 = (z1 - z2)*xperp + (z1 - m00)*yperp;

                                /* Right point */
                                z1 = *(magptr + ncols);
                                z2 = *(magptr + ncols + 1);
                                 mag2 = (z1 - z2)*xperp + (z1 - m00)*yperp;
                                 }
                        }
                        else
                        {
                            if (gx >= -gy)
                            {
                                /* 101 */
                                /* Left point */
                                z1 = *(magptr - 1);
                                z2 = *(magptr + ncols - 1);

                                mag1 = (m00 - z1)*xperp + (z1 - z2)*yperp;

                                /* Right point */
                                z1 = *(magptr + 1);
                                z2 = *(magptr - ncols + 1);
                                 mag2 = (m00 - z1)*xperp + (z1 - z2)*yperp;
                            }
                            else
                            {
                                /* 100 */
                                /* Left point */
                                z1 = *(magptr + ncols);
                                z2 = *(magptr + ncols - 1);

                                mag1 = (z1 - z2)*xperp + (m00 - z1)*yperp;

                                /* Right point */
                                z1 = *(magptr - ncols);
                                z2 = *(magptr - ncols + 1);

                                mag2 = (z1 - z2)*xperp  + (m00 - z1)*yperp;
                            }
                        }
                    }
                    else
                    {
                        if ((gy = *gyptr) >= 0)
                        {
                            if (-gx >= gy)
                            {
                                /* 011 */
                                /* Left point */
                                z1 = *(magptr + 1);
                                z2 = *(magptr - ncols + 1);

                                mag1 = (z1 - m00)*xperp + (z2 - z1)*yperp;
                                /* Right point */
                                z1 = *(magptr - 1);
                                z2 = *(magptr + ncols - 1);

                                mag2 = (z1 - m00)*xperp + (z2 - z1)*yperp;
                            }
                            else
                            {
                                /* 010 */
                                /* Left point */
                                z1 = *(magptr - ncols);
                                z2 = *(magptr - ncols + 1);

                                mag1 = (z2 - z1)*xperp + (z1 - m00)*yperp;
                                /* Right point */
                                z1 = *(magptr + ncols);
                                z2 = *(magptr + ncols - 1);

                                mag2 = (z2 - z1)*xperp + (z1 - m00)*yperp;
                            }
                        }
                        else
                        {
                            if (-gx > -gy)
                            {
                                /* 001 */
                                /* Left point */
                                z1 = *(magptr + 1);
                                z2 = *(magptr + ncols + 1);

                                mag1 = (z1 - m00)*xperp + (z1 - z2)*yperp;

                                /* Right point */
                                z1 = *(magptr - 1);
                                z2 = *(magptr - ncols - 1);

                                mag2 = (z1 - m00)*xperp + (z1 - z2)*yperp;
                            }
                            else
                            {
                                /* 000 */
                                /* Left point */
                                z1 = *(magptr + ncols);
                                z2 = *(magptr + ncols + 1);

                                mag1 = (z2 - z1)*xperp + (m00 - z1)*yperp;

                                /* Right point */
                                z1 = *(magptr - ncols);
                                z2 = *(magptr - ncols - 1);

                                mag2 = (z2 - z1)*xperp + (m00 - z1)*yperp;
                            }
                        }
                         }

                    /* Now determine if the current point is a maximum point */

 if ((mag1 > 0.0) || (mag2 > 0.0))
                    {
                        *resultptr = (unsigned char) NOEDGE;
                    }
                    else
                    {
                        if (mag2 == 0.0)
                            *resultptr = (unsigned char) NOEDGE;
                        else
                            *resultptr = (unsigned char) POSSIBLE_EDGE;
                    }
                }
            }
        }
         void main(void)
        {
           while(1)
           {
              Imgmag.read(mag);
              Imggradx.read(gradx);
              Imggrady.read(grady);
              if(VERBOSE) printf("Doing the non-maximal suppression.\n");
           non_max_supp(mag,gradx,grady, ROWS, COLS, nms);
           wait(228.65,SC_MS);
           Imgresult.write(nms);
           Imgmagout.write(magout);

           }
        }

        SC_CTOR(non_max_sup)
        {
           SC_THREAD(main);
           set_stack_size(128*1024*1024);
        }
};
SC_MODULE(apply_hys)
{
        sc_fifo_in<SIMAGE> Imgmag;
        sc_fifo_in<IMAGE> Imgnms;

        sc_fifo_out<IMAGE> Imgedge;
        SIMAGE mag;

       IMAGE nms;
        IMAGE edge;
        void follow_edges(unsigned char *edgemapptr, short *edgemagptr, short lowval, int cols)
                                                   {
           short *tempmagptr;
           unsigned char *tempmapptr;
           int i;
            int x[8] = {1,1,0,-1,-1,-1,0,1},
               y[8] = {0,1,1,1,0,-1,-1,-1};

           for(i=0;i<8;i++){
              tempmapptr = edgemapptr - y[i]*cols + x[i];
              tempmagptr = edgemagptr - y[i]*cols + x[i];

              if((*tempmapptr == POSSIBLE_EDGE) && (*tempmagptr > lowval)){
                 *tempmapptr = (unsigned char) EDGE;
                 follow_edges(tempmapptr,tempmagptr, lowval, cols);
              }
           }
        }

 void apply_hysteresis(short int *mag, unsigned char *nms, int rows, int cols,float tlow, float thigh, unsigned char *edge)        {
 int r, c, pos, numedges, highcount, lowthreshold, highthreshold, hist[32768];
           short int maximum_mag;
           for(r=0,pos=0;r<rows;r++){
              for(c=0;c<cols;c++,pos++){
                 if(nms[pos] == POSSIBLE_EDGE) edge[pos] = POSSIBLE_EDGE;
                 else edge[pos] = NOEDGE;
              }
           }

           for(r=0,pos=0;r<rows;r++,pos+=cols){
              edge[pos] = NOEDGE;
              edge[pos+cols-1] = NOEDGE;
           }
           pos = (rows-1) * cols;
           for(c=0;c<cols;c++,pos++){
            edge[c] = NOEDGE;
              edge[pos] = NOEDGE;
           }
 for(r=0;r<32768;r++) hist[r] = 0;
           for(r=0,pos=0;r<rows;r++){
              for(c=0;c<cols;c++,pos++){
                 if(edge[pos] == POSSIBLE_EDGE) hist[mag[pos]]++;
              }
           }
           for(r=1,numedges=0;r<32768;r++){
              if(hist[r] != 0) maximum_mag = r;
              numedges += hist[r];
           }

           highcount = (int)(numedges * thigh + 0.5);
           r = 1;
           numedges = hist[1];
           while((r<(maximum_mag-1)) && (numedges < highcount)){
              r++;
              numedges += hist[r];
           }
           highthreshold = r;
           lowthreshold = (int)(highthreshold * tlow + 0.5);
            if(VERBOSE){
              printf("The input low and high fractions of %f and %f computed to\n",
                 tlow, thigh);
              printf("magnitude of the gradient threshold values of: %d %d\n",
                 lowthreshold, highthreshold);
           }
           for(r=0,pos=0;r<rows;r++){
              for(c=0;c<cols;c++,pos++){
                 if((edge[pos] == POSSIBLE_EDGE) && (mag[pos] >= highthreshold)){
                    edge[pos] = EDGE;
                    follow_edges((edge+pos), (mag+pos), lowthreshold, cols);
                 }
              }
           }
for(r=0,pos=0;r<rows;r++){
              for(c=0;c<cols;c++,pos++) if(edge[pos] != EDGE) edge[pos] = NOEDGE;
           }
        }
        void main(void)
        {
           while(1)
            {
              Imgmag.read(mag);
              Imgnms.read(nms);

              if(VERBOSE) printf("Doing hysteresis thresholding.\n");
            apply_hysteresis(mag, nms, ROWS, COLS, TLOW, THIGH, edge);
            wait(238.17,SC_MS);
           Imgedge.write(edge);

           }
        }

        SC_CTOR(apply_hys)
        {
           SC_THREAD(main);
           set_stack_size(128*1024*1024);
        }
};
SC_MODULE(DUT)
{
        sc_fifo_in<IMAGE> ImgIn;
        sc_fifo_out<IMAGE> ImgOut;
        sc_fifo<SIMAGE> q1;
        sc_fifo<SIMAGE> q2;
        sc_fifo<SIMAGE> q3;
        sc_fifo<SIMAGE> q4;
        sc_fifo<SIMAGE> q5;
        sc_fifo<SIMAGE> q6;
        sc_fifo<IMAGE> q7;
        sc_fifo<SIMAGE> q8;

        Gaussian_smooth gs;
         derivative der;
        magnitude mag;
        non_max_sup nms;
        apply_hys ah;

        void before_end_of_elaboration(){
           gs.ImgIn.bind(ImgIn);
           gs.ImgOut.bind(q1);
           der.Imgsmoothdim.bind(q1);
           der.Imgdeltax.bind(q2);
           mag.Imgdeltax.bind(q2);
           der.Imgdeltay.bind(q3);
           mag.Imgdeltay.bind(q3);
           mag.Imgmag1.bind(q4);
           nms.Imgmag.bind(q4);
           mag.Imggradx.bind(q5);
           nms.Imggradx.bind(q5);
           mag.Imggrady.bind(q6);
           nms.Imggrady.bind(q6);
           nms.Imgresult.bind(q7);
           ah.Imgnms.bind(q7);
           nms.Imgmagout.bind(q8);
           ah.Imgmag.bind(q8);
           ah.Imgedge.bind(ImgOut);

        }

        SC_CTOR(DUT)
        :q1("q1",1)
        ,q2("q2",1)
        ,q3("q3",1)
        ,q4("q4",1)
        ,q5("q5",1)
        ,q6("q6",1)
        ,q7("q7",1)
        ,q8("q8",1)
        ,gs("gs")
        ,der("der")
        ,mag("mag")
        ,nms("nms")
        ,ah("ah")
        {}
};

SC_MODULE(Platform)
{
        sc_fifo_in<IMAGE> ImgIn;
        sc_fifo_out<IMAGE> ImgOut;
        sc_fifo<IMAGE> q1;
        sc_fifo<IMAGE> q2;

        DataIn din;
        DUT dut;
         DataOut dout;

        void before_end_of_elaboration(){
           din.ImgIn.bind(ImgIn);
           din.ImgOut.bind(q1);
           dut.ImgIn.bind(q1);
           dut.ImgOut.bind(q2);
           dout.ImgIn.bind(q2);
           dout.ImgOut.bind(ImgOut);
        }

        SC_CTOR(Platform)
        :q1("q1",1)
        ,q2("q2",1)
        ,din("din")
        ,dut("dut")
        ,dout("dout")
        {}
};

SC_MODULE(Top)
{
        sc_fifo<IMAGE> q1;
        sc_fifo<IMAGE> q2;
        sc_fifo<sc_time>testtime;
        Stimulus stimulus;
        Platform platform;
        Monitor monitor;

        void before_end_of_elaboration(){
           stimulus.ImgOut.bind(q1);
           platform.ImgIn.bind(q1);
           platform.ImgOut.bind(q2);
           monitor.ImgIn.bind(q2);
           stimulus.stimtime.bind(testtime);
           monitor.stimtimein.bind(testtime);
        }

        SC_CTOR(Top)
        :q1("q1",1)
        ,testtime("testtime",30)
        ,q2("q2",1)
        ,stimulus("stimulus")
        ,platform("platform")
        ,monitor("monitor")
 {}
};
int sc_main(int argc, char* argv[])
{
Top top("top");
   sc_start();
        return 0;
}







