#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <wgslib.h>
#include <fcntl.h>
#include <console.h>
#include <termio.h>
#include <xmldom.h>
extern char *getappdir();

typedef struct panel_s {
  int firstrow;
  int cursoroffset;
  int totalnumofrows;

  char * path;

  DOMElement * xmldirtree;
  DOMElement * xmltreeptr;
} panel;

DOMElement * tempnode, * tempnode2;

panel * toppanel, * botpanel, * activepanel;

char * VERSION = "1.2";
int singleselect,extendedview;

int MainFG = COL_White;
int MainBG = COL_Blue;

int globaltioflags;

struct termios tio;

void prepconsole();
void drawpanel(panel * thepan);
void pressanykey();

void drawmessagebox(char * string1, char * string2, int pressakey) {
  int width, startcolumn, row, i, padding1, padding2;

  if(strlen(string1) < strlen(string2)) {
    width = strlen(string2);
    padding1 = width - strlen(string1);
    padding2 = 0;
  } else {
    width = strlen(string1);
    padding1 = 0;
    padding2 = width - strlen(string2);
  }
  width = width+6;

  row         = 10;
  startcolumn = (con_xsize - width)/2;

  con_gotoxy(startcolumn, row);
  putchar(' ');
  for(i = 0; i < width-2; i++)
    printf("_");
  putchar(' ');

  row++;

  con_gotoxy(startcolumn, row);
  printf(" |");
  for(i = 0; i < width-4; i++)
    printf(" ");
  printf("| ");

  row++;

  con_gotoxy(startcolumn, row);
  printf(" | %s", string1);

  for(i=0; i<padding1; i++)
    putchar(' ');

  printf(" | ");
    
  row++;

  if(strlen(string2) > 0) {
    con_gotoxy(startcolumn, row);
    printf(" | %s", string2);  
    
    for(i=0; i<padding2; i++)
      putchar(' ');
  
    printf(" | ");
  
    row++;
  }
  
  //Extra row... 
  /*
  con_gotoxy(startcolumn, row);
  printf(" |");
  for(i = 0; i < width-4; i++)
    printf(" ");
  printf("| "); 
    
  row++;
  */
  con_gotoxy(startcolumn, row);
  printf(" |");
  for(i = 0; i < width-4; i++) 
    printf("_");
  printf("| ");
      
  con_update();

  if(pressakey)
    pressanykey();
}
  
void movechardown(int x, int y, char c){
  con_gotoxy(x, y);
  putchar(' ');
  con_gotoxy(x, y+1);
  putchar(c);   
  con_update();
}   

void movecharup(int x, int y, char c){
  con_gotoxy(x, y);
  putchar(' ');
  con_gotoxy(x, y-1);
  putchar(c);  
  con_update();
}

void lineeditmode() {
  con_modeon(TF_ICANON);
} 

void onecharmode() {
  con_modeoff(TF_ICANON);
}

void pressanykey() { 
  int temptioflags;
  temptioflags = tio.flags;   
  onecharmode();
  getchar();
  tio.flags = temptioflags;
  settio(STDOUT_FILENO, &tio);
}

void drawframe(char * message) {
  char * titlestr;
  int i, xpos, ypos;

  titlestr = (char *)malloc(strlen(" File Manager Version  2003 ")+strlen(VERSION)+2);

  sprintf(titlestr, " File Manager Version %s 2003 ", VERSION);

  xpos = 0;
  ypos = 0;

  con_gotoxy(xpos,ypos);
  if(activepanel == toppanel)
    con_setfgbg(COL_Black,COL_Red);
  else
    con_setfgbg(COL_Black,COL_White);
  con_clrline(LC_End);

  for(i = 0;i < (con_xsize - strlen(titlestr))/2; i++) {
    con_gotoxy(xpos, ypos);
    putchar(' ');
    xpos++;
  }
	
  printf("%s",titlestr);
  xpos = xpos + strlen(titlestr);

  for(i = 0;i <= (con_xsize - strlen(titlestr))/2; i++) {
    con_gotoxy(xpos, ypos);
    putchar(' ');
    xpos++;
  }

  ypos = botpanel->firstrow - 1;
  xpos = 0;

  con_gotoxy(xpos,ypos);
  if(activepanel == botpanel)
    con_setfgbg(COL_Black,COL_Red);
  else
    con_setfgbg(COL_Black,COL_White);
  con_clrline(LC_End);

  for(i = 0;i < (con_xsize - strlen(message))/2; i++) {
    con_gotoxy(xpos, ypos);
    putchar(' ');
    xpos++;
  }
	
  printf("%s",message);
  xpos = xpos + strlen(message);

  for(i = 0;i <= (con_xsize - strlen(message))/2; i++) {
    con_gotoxy(xpos, ypos);
    putchar(' ');
    xpos++;
  }

  con_gotoxy(0,botpanel->firstrow+botpanel->totalnumofrows+1);
  con_setfgbg(COL_Black,COL_White);
  con_clrline(LC_End);

  if(extendedview) 
    printf(" TAB, SPACE, (n)ew dir, (r)ename, (m)ove, (c)opy, (D)elete, (S)elect and quit");
  else
    printf(" TAB/SPACE (n,r,m,c,D,S)");

  con_setfgbg(MainFG,MainBG);
}

void clearpanel(panel * thepan) {
  int i;

  for(i = thepan->firstrow; i<thepan->firstrow+thepan->totalnumofrows; i++) {
    con_gotoxy(0,i);
    con_clrline(LC_End);   
  }
}

void panelchange() {
  con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
  putchar(' ');

  if(activepanel == toppanel)
    activepanel = botpanel;    
  else 
    activepanel = toppanel;

  drawframe("Welcome to the WiNGs File Manager");

  con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
  putchar('>');

  con_setscroll(activepanel->firstrow, activepanel->firstrow+activepanel->totalnumofrows);
}

void drawpanelline(DOMElement * dirptr, int active) {
  long filesize;
  char * tag,* filetype,* filename;
  
  if(strcmp(XMLgetAttr(dirptr, "filesize"), " "))
    filesize = strtol(XMLgetAttr(dirptr, "filesize"), NULL, 10);
  else
    filesize = 0;

  tag      = XMLgetAttr(dirptr, "tag");
  filetype = XMLgetAttr(dirptr, "filetype");
  filename = XMLgetAttr(dirptr, "filename");

  if(filesize > 1048576) 
    printf(" %s %16s %6ld mb    %s", tag, filename, filesize/1048576, filetype);
  else if(filesize > 1024)
    printf(" %s %16s %6ld kb    %s", tag, filename, filesize/1024,    filetype);
  else if(filesize > 0)
    printf(" %s %16s %6ld bytes %s", tag, filename, filesize,         filetype);
  else 
    printf(" %s %16s %6s       %s",  tag, filename, " ",              filetype);
}

void drawpanel(panel * thepan) {
  int i, active;
  DOMElement * dirptr;

  dirptr = XMLgetNode(thepan->xmldirtree, "entry");

  if(thepan == activepanel)
    active = 1;
  else 
    active = 0;

  for(i=thepan->firstrow;i<thepan->firstrow+thepan->totalnumofrows;i++) {
    con_gotoxy(0,i);
    drawpanelline(dirptr, active);
    if(dirptr->NextElem->FirstElem)
      break;    
    dirptr = dirptr->NextElem;
  }
}

void builddir(panel * thepan) {
  char * fullname, * filesize, *ext;
  int i, root;
  DIR *dir;
  struct dirent *entry;
  struct stat buf;
  

  clearpanel(thepan);

  if(thepan->xmldirtree) {
    tempnode = XMLgetNode(thepan->xmldirtree,"entry");
    if(tempnode) {
      while(tempnode->NextElem != tempnode) {
        tempnode2 = tempnode->NextElem;
        XMLremNode(tempnode);
        tempnode = tempnode2;
      }
      XMLremNode(tempnode);
    }
  } else 
    thepan->xmldirtree = XMLnewNode(NodeType_Element, "xmldirtree", "");

  if(strcmp(thepan->path,"/")) {
    tempnode = XMLnewNode(NodeType_Element, "entry", "");
    XMLinsert(thepan->xmldirtree, NULL, tempnode);
    XMLsetAttr(tempnode, "filename", "Parent (../)");
    XMLsetAttr(tempnode, "tag", " ");
    XMLsetAttr(tempnode, "parent", "true");
    XMLsetAttr(tempnode, "filetype", "Directory");
    XMLsetAttr(tempnode, "filesize", " ");
    root = 0;
  } else
    root = 1;

  thepan->cursoroffset = 0;

  dir = opendir(thepan->path);
  if(!dir) { 
    con_end();
    con_clrscr();
    exit(EXIT_FAILURE);
  }
  
  while(entry = readdir(dir)) {
    //Hide virtual directories
    if(root) {
      if(!strcmp(entry->d_name, "boot"))
        continue;
      if(!strcmp(entry->d_name, "dev"))
        continue;
      if(!strcmp(entry->d_name, "sys"))
        continue;
    } 

    filesize = NULL;
    tempnode = XMLnewNode(NodeType_Element, "entry", "");
    XMLinsert(thepan->xmldirtree, NULL, tempnode);    
    XMLsetAttr(tempnode, "filename", entry->d_name);
    XMLsetAttr(tempnode, "parent", "false");

    ext = &entry->d_name[strlen(entry->d_name)-4];

    if(!strcmp(ext,".app")) {
      XMLsetAttr(tempnode, "filetype", "Application");
      XMLgetAttr(tempnode, "filename")[strlen(entry->d_name)-4] = 0; 
      filesize = strdup(" ");
    } else if(entry->d_type == 6) {
      XMLsetAttr(tempnode, "filetype", "Directory");
      filesize = strdup(" ");
    } else if(!strcmp(ext,".txt")) {
      XMLsetAttr(tempnode, "filetype", "TEXT Document");
    } else if(!strcmp(ext,".mod") ||
              !strcmp(ext,".s3m") ||
              !strcmp(&ext[1],".xm")) {
      XMLsetAttr(tempnode, "filetype", "Module Music");
    } else if(!strcmp(ext,".sid") ||
              !strcmp(ext,".dat")) {
      XMLsetAttr(tempnode, "filetype", "SID Music");
    } else if(!strcmp(ext,".tmp")) {
      XMLsetAttr(tempnode, "filetype", "Temporary File");
    } else if(!strcmp(ext,".jpg")) {
      XMLsetAttr(tempnode, "filetype", "JPEG Image");
    } else if(!strcmp(ext,".hbm")) {
      XMLsetAttr(tempnode, "filetype", "Highres BitMap");
    } else if(!strcmp(ext,".wav")) {
      XMLsetAttr(tempnode, "filetype", "WAVE Audio");
    } else if(!strcmp(ext,".mov")) {
      XMLsetAttr(tempnode, "filetype", "WiNGs Movie");
    } else if(!strcmp(ext,".rvd")) {
      XMLsetAttr(tempnode, "filetype", "Raw Video Data");
    } else if(!strcmp(ext,".drv")) {
      XMLsetAttr(tempnode, "filetype", "Driver");
    } else if(!strcmp(ext,"font")) {
      XMLsetAttr(tempnode, "filetype", "GUI Font");
    } else if(!strcmp(ext,"cfnt")) {
      XMLsetAttr(tempnode, "filetype", "Console Font");
    } else if(!strcmp(&ext[1],".so")) {
      XMLsetAttr(tempnode, "filetype", "Shared Object");
    } else if(!strcmp(ext,".bak")) {
      XMLsetAttr(tempnode, "filetype", "Backup File");
    } else if(!strcmp(ext,".zip") ||
              !strcmp(&ext[1], ".gz")) {
      XMLsetAttr(tempnode, "filetype", "ZIP Archive");
    } else if(!strcasecmp(ext,".prg")) {
      XMLsetAttr(tempnode, "filetype", "C64 Binary");
    } else if(!strcmp(ext,"html") ||
              !strcmp(ext,".htm")) {
      XMLsetAttr(tempnode, "filetype", "HTML Document");
    } else {
      XMLsetAttr(tempnode, "filetype", " ");
    }
    if(filesize == NULL) {
      fullname = fpathname(entry->d_name, thepan->path, 1);
      stat(fullname, &buf);
      filesize = (char *)malloc(25);
      sprintf(filesize, "%10ld", buf.st_size);
    }
    XMLsetAttr(tempnode, "filesize", filesize);
    XMLsetAttr(tempnode, "tag", " ");
  }

  thepan->xmltreeptr = tempnode->NextElem;

  closedir(dir);  

  //XMLsaveFile(thepan->xmldirtree, "/test/outfile.xml");

  drawpanel(thepan);
}

void launch(panel * thepan, int text) {
  char * ext, * filename, * tempstr, *nedstr, *tempstr2;
  char input;

  filename = (char *)malloc(17);
  tempstr  = (char *)malloc(strlen(thepan->path)+25);
  tempstr2 = NULL;

  sprintf(filename, "%s", XMLgetAttr(thepan->xmltreeptr, "filename"));
  sprintf(tempstr, "\"%s%s\"", thepan->path, filename);

  ext = &filename[strlen(filename)-4];

  if(!strcasecmp(ext, ".txt") || text) {
    con_end();
    //Rewrite the string sans outside quotes
    sprintf(tempstr, "%s%s", thepan->path, filename);
    spawnlp(S_WAIT, "ned", tempstr, NULL);
    prepconsole();
    con_clrscr();
    drawframe("Welcome to the WiNGs Filemanager");
    clearpanel(toppanel);
    clearpanel(botpanel);
    drawpanel(toppanel);
    drawpanel(botpanel);
  } else if(!strcasecmp(ext, ".wav")) {
    tempstr2 = (char *)malloc(strlen(tempstr) + strlen("wavplay  >/dev/null "));
    sprintf(tempstr2, "wavplay %s >/dev/null", tempstr);
    system(tempstr2);
  } else if(!strcasecmp(ext, ".mod") || !strcasecmp(ext, ".s3m") || !strcasecmp(&ext[1], ".xm")) {
    tempstr2 = (char *)malloc(strlen(tempstr) + strlen("josmod -h 11000  >/dev/null "));
    sprintf(tempstr2, "josmod -h 11000 %s >/dev/null", tempstr);
    system(tempstr2);
  } else if((!strcasecmp(ext, ".dat")) || (!strcasecmp(ext, ".sid"))) {
    tempstr2 = (char *)malloc(strlen(tempstr) + strlen("sidplay  >/dev/null "));
    sprintf(tempstr2, "sidplay %s >/dev/null", tempstr);
    system(tempstr2);
    resetsid();
  } else if(!strcasecmp(ext, ".zip") || !strcasecmp(&ext[1], ".gz")) {
    drawmessagebox("Are you sure you want to unzip this archive?","              (Y)es  or (n)o", 0);
    input = 0;
    while(input != 'Y' && input != 'n')
      input = con_getkey();
    if(input == 'Y') {
      drawmessagebox("Unzipping archive. Please wait.", "", 0);
      con_modeoff(TF_ECHO);
      tempstr2 = (char *)malloc(strlen(tempstr) + strlen("gunzip  2>/dev/null >/dev/null "));
      sprintf(tempstr2, "cd %s", thepan->path);
      system(tempstr2);
      sprintf(tempstr2, "gunzip %s 2>/dev/null >/dev/null", tempstr);
      system(tempstr2);
      con_clrscr();
      con_modeon(TF_ECHO);
      builddir(toppanel);
      builddir(botpanel);
    } else {
      clearpanel(toppanel);
      drawpanel(toppanel);
      clearpanel(botpanel);
      drawpanel(botpanel);
    }
    drawframe("Welcome to the WiNGs Filemanager");
  }

  free(filename);
  free(tempstr);
  if(tempstr2)
    free(tempstr2);
}

char * getmyline(int size, int x, int y) {
  int i,count = 0;
  char * linebuf;

  linebuf = (char *)malloc(size+1);

  onecharmode();

  /*  ASCII Codes

    32 is SPACE
    126 is ~
    47 is /
    8 is DEL

  */

  while(1) {
    i = con_getkey();
    if(i > 31 && i < 127 && i != 47  && count < size) {
      linebuf[count] = i;
      con_gotoxy(x+count,y);
      putchar(i);
      linebuf[++count] = 0;
      con_update();
    } else if(i == 8 && count > 0) {
      count--;
      con_gotoxy(x+count,y);
      putchar(' ');
      con_gotoxy(x+count,y);
      linebuf[count] = 0;
      con_update();
    } else if(i == '\n' || i == '\r') 
      break;
  }
  return(linebuf);
}

void prepconsole() {
  con_init();

  gettio(STDOUT_FILENO, &tio);
  tio.flags |= TF_ECHO|TF_ICRLF;
  tio.MIN = 1;  
  settio(STDOUT_FILENO, &tio);
}

void main() {
  FILE * tempout;
  int input,i,size = 0;
  char *tempstr, *tempstr2, *mylinebuf, *getbuf = NULL;

  prepconsole();

  if(con_xsize < 80) {
    extendedview = 0;

    //Until the 40 column scrolling is implemented properly.

    printf("Fileman requires an 80 column console.\n");
    con_update();
    con_end();
    exit(1);

  } else
    extendedview = 1;

  toppanel = (panel *)malloc(sizeof(panel)+1);
  botpanel = (panel *)malloc(sizeof(panel)+1);

  toppanel->xmldirtree = NULL;
  botpanel->xmldirtree = NULL;

  toppanel->totalnumofrows = (con_ysize - 3)/2;
  botpanel->totalnumofrows = (con_ysize - 3)/2;

  toppanel->firstrow = 1;
  botpanel->firstrow = toppanel->firstrow+toppanel->totalnumofrows+1;

  toppanel->path = (char *)malloc(256);
  botpanel->path = (char *)malloc(256);

  sprintf(toppanel->path, "/");
  sprintf(botpanel->path, "/");

  activepanel = toppanel;  

  con_setfgbg(MainFG,MainBG);
  con_clrscr();

  builddir(botpanel);
  builddir(activepanel);

  drawframe("Welcome to the WiNGs File Manager");

  panelchange();
  panelchange();

  con_update();

  input = 0;
  while(input != 'S' && input != 'Q') {
    input = con_getkey();

    switch(input) {
      case CURD:
        if(activepanel->xmltreeptr->NextElem->FirstElem)
          break;
        if(activepanel->cursoroffset < (activepanel->totalnumofrows)-1) {
          movechardown(0,activepanel->firstrow+activepanel->cursoroffset, '>');
          activepanel->xmltreeptr = activepanel->xmltreeptr->NextElem;
          activepanel->cursoroffset++;
        } else {
          putchar('\n');
          activepanel->xmltreeptr = activepanel->xmltreeptr->NextElem;
          drawpanelline(activepanel->xmltreeptr, 1);
          con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset-1);
          putchar(' ');
          con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
          putchar('>');
        }                
      break;
      case CURU:
        if(activepanel->xmltreeptr->FirstElem)
          break;
        if(activepanel->cursoroffset > 0) {
          movecharup(0,activepanel->firstrow+activepanel->cursoroffset, '>');
          activepanel->xmltreeptr = activepanel->xmltreeptr->PrevElem;
          activepanel->cursoroffset--;
        } else {
          printf("\x1b[1L");
          con_gotoxy(0,activepanel->firstrow);
          activepanel->xmltreeptr = activepanel->xmltreeptr->PrevElem;
          drawpanelline(activepanel->xmltreeptr, 1);
          con_gotoxy(0,activepanel->firstrow+1);
          putchar(' ');
          con_gotoxy(0,activepanel->firstrow);
          putchar('>');
        }
      break;

      case TAB:  
        panelchange();
      break;

      case ' ':
        if(strcmp(XMLgetAttr(activepanel->xmltreeptr,"parent"), "true")) {
          con_gotoxy(1, activepanel->firstrow+activepanel->cursoroffset);
          if(!strcmp(XMLgetAttr(activepanel->xmltreeptr,"tag"), " ")) {
            putchar('*');
            XMLsetAttr(activepanel->xmltreeptr,"tag","*");
          } else {
            putchar(' ');
            XMLsetAttr(activepanel->xmltreeptr,"tag"," ");
          }

          con_gotoxy(0, activepanel->firstrow+activepanel->cursoroffset);
          putchar('>');
        }
      break;

      case 'T':
        launch(activepanel, 1);
      break;

      case '\n':
      case '\r':
        if(!strcmp(XMLgetAttr(activepanel->xmltreeptr, "filetype"), "Directory")) {
          if(strcmp(XMLgetAttr(activepanel->xmltreeptr, "parent"), "true")) {
            sprintf(activepanel->path, "%s%s/", activepanel->path, XMLgetAttr(activepanel->xmltreeptr, "filename"));
            builddir(activepanel);
          } else {
            for(i=2;i<=strlen(activepanel->path);i++) {
              if(activepanel->path[strlen(activepanel->path)-i] == '/') {
                activepanel->path[strlen(activepanel->path)-i+1] = 0;
                break;
              }
            } 
            builddir(activepanel);
          }
        } else 
          launch(activepanel, 0);

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'r':
        i = 0;
        tempnode = activepanel->xmltreeptr;

        tempstr  = (char *)malloc(strlen("Rename:   ") + strlen(activepanel->path)+18);
        tempstr2 = (char *)malloc(strlen(activepanel->path)+17);

        do {
          if(!strcmp(XMLgetAttr(activepanel->xmltreeptr, "tag"), "*")) {
            sprintf(tempstr,"Rename: %s",XMLgetAttr(activepanel->xmltreeptr,"filename"));

            drawmessagebox(tempstr, "                         ",0);

            if(!strcmp(XMLgetAttr(activepanel->xmltreeptr,"filetype"),"Application")) {
              mylinebuf = getmyline(12,27,13);
              if(strlen(mylinebuf) > 0) {

                sprintf(tempstr,"%s%s.app",activepanel->path,XMLgetAttr(activepanel->xmltreeptr,"filename"));
                sprintf(tempstr2,"%s%s.app",activepanel->path,mylinebuf);

                spawnlp(S_WAIT,"mv","-f",tempstr,tempstr2,NULL);
              }
            } else {
              mylinebuf = getmyline(16,27,13);
              if(strlen(mylinebuf) > 0) {

                sprintf(tempstr,"%s%s",activepanel->path,XMLgetAttr(activepanel->xmltreeptr,"filename"));
                sprintf(tempstr2,"%s%s",activepanel->path,mylinebuf);

                spawnlp(S_WAIT,"mv","-f",tempstr,tempstr2,NULL);
              }
            }
            free(mylinebuf);
            i++;
          }
          activepanel->xmltreeptr = activepanel->xmltreeptr->NextElem;
        } while(activepanel->xmltreeptr != tempnode);

        free(tempstr);
        free(tempstr2);

        if(i) { 
          con_clrscr();
          if(!strcmp(toppanel->path, botpanel->path)) {
            builddir(toppanel);
            builddir(botpanel);
          } else {
            builddir(activepanel);
            if(activepanel == toppanel)
              drawpanel(botpanel);
            else
              drawpanel(toppanel);
          }
          drawframe(" Renaming Complete ");
          system("sync");
        } else {
          if(extendedview)
            drawframe(" Press SPACE to tag Files and Directories ");
          else
            drawframe(" Press SPACE to tag Files");
        }

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'D':
        drawmessagebox("Are you sure you want to delete all flagged items?","               (Y)es, (n)o",0);
        i = 'z';
        while(i != 'Y' && i != 'n') 
          i = con_getkey();

        if(i == 'Y') {
          drawframe(" Deleting tagged Files and Directories ");
          tempnode = activepanel->xmltreeptr;

          i = 0;
          tempstr = (char *)malloc(strlen(activepanel->path)+18);
          do {
            if(!strcmp(XMLgetAttr(activepanel->xmltreeptr, "tag"), "*")) {

              if(!strcmp(XMLgetAttr(activepanel->xmltreeptr, "filetype"),"Application"))
                sprintf(tempstr,"%s%s.app",activepanel->path,XMLgetAttr(activepanel->xmltreeptr,"filename"));
              else
                sprintf(tempstr,"%s%s",activepanel->path,XMLgetAttr(activepanel->xmltreeptr,"filename"));
 
             spawnlp(S_WAIT,"rm","-r","-f",tempstr,NULL);
              i++;
            }
            activepanel->xmltreeptr = activepanel->xmltreeptr->NextElem;
          } while(activepanel->xmltreeptr != tempnode);
          free(tempstr);

          if(i) { 
            builddir(toppanel);
            builddir(botpanel);
            drawframe(" Deleting Complete ");
          } else {
            drawframe("Welcome to the WiNGs File Manager");
          
            clearpanel(toppanel); 
            drawpanel(toppanel);
            clearpanel(botpanel);
            drawpanel(botpanel);
          }
        } else {
          drawframe("Welcome to the WiNGs File Manager");
          
          clearpanel(toppanel); 
          drawpanel(toppanel);
          clearpanel(botpanel);
          drawpanel(botpanel);
        }

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'm':
        i = 0;
        drawframe(" Moving tagged Files and Directories ");
        tempnode = activepanel->xmltreeptr;

        do {
          if(!strcmp(XMLgetAttr(activepanel->xmltreeptr, "tag"), "*")) {
            i++;
            tempstr = (char *)malloc(strlen(activepanel->path)+strlen(XMLgetAttr(activepanel->xmltreeptr,"filename"))+1);

            sprintf(tempstr,"%s%s",activepanel->path,XMLgetAttr(activepanel->xmltreeptr,"filename"));
            if(activepanel == toppanel)
              spawnlp(S_WAIT,"mv","-f",tempstr, botpanel->path,NULL);
            else
              spawnlp(S_WAIT,"mv","-f",tempstr, toppanel->path,NULL);
          }
          activepanel->xmltreeptr = activepanel->xmltreeptr->NextElem;
        } while(activepanel->xmltreeptr != tempnode);

        if(i) { 
          drawframe(" Moving Complete ");
          builddir(botpanel);
          builddir(toppanel);
          system("sync");
        } else {
          if(extendedview)
            drawframe(" Press SPACE to tag Files and Directories ");
          else
            drawframe(" Press SPACE to tag Files");
        }

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'c':
        i = 0;
        drawframe(" Copying tagged Files and Directories ");
        tempnode = activepanel->xmltreeptr;

        do {
          if(!strcmp(XMLgetAttr(activepanel->xmltreeptr, "tag"), "*")) {
            i++;
            tempstr = (char *)malloc(strlen(activepanel->path)+strlen(XMLgetAttr(activepanel->xmltreeptr,"filename"))+1);

            sprintf(tempstr,"%s%s",activepanel->path,XMLgetAttr(activepanel->xmltreeptr,"filename"));
            if(activepanel == toppanel)
              spawnlp(S_WAIT,"cp","-r","-f",tempstr, botpanel->path,NULL);
            else
              spawnlp(S_WAIT,"cp","-r","-f",tempstr, toppanel->path,NULL);
          }
          activepanel->xmltreeptr = activepanel->xmltreeptr->NextElem;
        } while(activepanel->xmltreeptr != tempnode);

        if(i) { 
          drawframe(" Copying Complete ");
          if(activepanel == toppanel)
            builddir(botpanel);
          else
            builddir(toppanel);
          system("sync");
        } else {
          if(extendedview)
            drawframe(" Press SPACE to tag Files and Directories ");
          else
            drawframe(" Press SPACE to tag Files");
        }

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'n':
        tempstr = (char *)malloc(strlen("New Directory Name:") +1);
        tempstr2 = (char *)malloc(strlen("mkdir  2>/dev/null >/dev/null") + strlen(activepanel->path)+18);

        sprintf(tempstr,"New Directory Name:");
        drawmessagebox(tempstr, "                         ",0);
        mylinebuf = getmyline(16,27,13);
        if(strlen(mylinebuf) > 0) {
          sprintf(tempstr2,"mkdir \"%s%s\" 2>/dev/null >/dev/null",activepanel->path,mylinebuf);
          system(tempstr2);
          con_clrscr();
          drawframe("New directory created");
          builddir(toppanel);
          builddir(botpanel);
        }

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');

      break;
    }
    con_gotoxy(0,0);
    con_update();
  } 

  con_end();
  printf("\x1b[0m"); //reset the term colors
  con_clrscr();
  tempout = fopen("/wings/attach.tmp", "w");
  fprintf(tempout,"%s%s",activepanel->path,XMLgetAttr(activepanel->xmltreeptr, "filename"));
  fclose(tempout);
  con_update();
}
