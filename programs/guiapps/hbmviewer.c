//These are the header includes that we need.

#include <winlib.h>
#include <wgslib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

//The primary function of all c programs. 

unsigned char app_icon[] = {
0,1,2,68,232,127,32,32,
0,128,64,34,23,254,4,4,
32,32,32,32,127,224,64,0,
4,4,4,4,254,7,2,0,
0x01,0x01,0x01,0x01
};

void main(int argc, char * argv[]) {
  JMeta * metadata;

  //These void pointers are the handles I will use for the widets I 
  //will create in the application.

  void * app, * window, * scr, * view, *bmp, *bmpdata;

  //this is the filepointer that will be used for loading the image off 
  //the disk. 

  FILE * fp;

  //and some miscellanious integer variables.

  int x, y;
  int imgsize;


  //Check to see if the user specified image dimensions... 

  if(argc > 2) {
    x = atoi(argv[2]);
    y = atoi(argv[3]);

  //otherwise we assume the default of 320x200.

  } else {
    x = 320;
    y = 200;
  }

  //Calculate how much ram is needed for the image. color mem + bitmap mem

  imgsize = (y*(x/8)) + ((x/8)*(y/8));

  //We initialize the JApp. app is it's handle.
  //We initialize a window, with a handle. Giving it a title, and
  //and the flags are for it's mode. In this case the window can be 
  //resized.

  app = JAppInit(NULL,0);

  metadata = malloc(sizeof(JMeta));
  metadata->launchpath = strdup(fpathname(argv[0],getappdir(),1));
  metadata->title      = "HBM Viewer";
  metadata->icon       = app_icon;
  metadata->showicon   = 1;
  metadata->parentreg  = -1;

  window = JWndInit(NULL, "HBM Viewer", JWndF_Resizable,metadata);

  //the window is attached to the JApp as the MAIN window. if the main
  //window is killed, the application quits. 

  JAppSetMain(app, window);

  //Every JW has minimum, maximum and prefered dimensions. 
  //JWSetBounds sets the preferred dimensions and default position on 
  //screen of the widget passed to it. In this case we set the 
  //position and size of the window. It will start in the top left 
  //corner, and be the same size as the image inside it.

  JWSetBounds(window, 0,0, x-32, y-32);

  //if they didn't specify a 2nd argument... then we don't have a file
  //to load in... so print out the help text to the console and exit.

  if(argc < 2) {
    fprintf(stderr, "USAGE: hbmviewer path/filename [x][y]\n");
    fprintf(stderr, "       Default X x Y is 320x200\n");
    exit(1);
  }

  //Try to open the second argument as a file for reading.

  fp = fopen(argv[1], "rb");

  //if the file could not be opened... just exit.

  if(!fp)
    exit(1);

  //Ask wings for a block of memory the size we figured out that we need.
  //and assign a pointer to the start of that block of memory.

  bmpdata = malloc(imgsize);
  
  //Read in from the file the EXACT amount it should be, and fill the
  //buffer we've allocated for. Even if the user put in the Wrong 
  //dimensions for the image... we aren't reading to the absolute end
  //of the file... so this could result in an incorrectly displaying 
  //bitmap, but it won't overrun the buffer. 

  fseek(fp, 2, SEEK_CUR);
  fread(bmpdata, 1, imgsize, fp);

  //Close the file, we're finished with it.

  fclose(fp);

  //Initialize a BitMap widget. Giving it the size dimensions, and the 
  //pointer to the start of the image data buffer.

  bmp  = JBmpInit(NULL, x, y, bmpdata);

  //Now... at this point the bitmap widget exists in memory, but it 
  //isn't connected to the window widget in any way.

  //Create a View Area widget, and give it the bitmap widget.

  view = JViewWinInit(NULL, bmp);

  //Create a area with scrollbars, and give it the view area widget. 
  //The flags being 0 is the default.. it means the scroll bars, either 
  //horizontal or vertical, will only show up if the view area is forced 
  //too small to show all of it's contents. This would happen if the 
  //window was resized to be smaller.

  scr  = JScrInit(NULL, view, 0);

  //A window is a type of container. There are other containers that 
  //we'll look at in a more advanced program. 

  //Use JCntAdd() to add one widget to a container widget. The scrollable
  //area, with the viewable area, with the bitmap, as added to the window.

  JCntAdd(window, scr);

  //Finally, Show the window, Push the console part of the program into
  //the background, and JAppLoop() the JApp to make the event loop active.  

  JWinShow(window);

  retexit(1);
  JAppLoop(app);
}
