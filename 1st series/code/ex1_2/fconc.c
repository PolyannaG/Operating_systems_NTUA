#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

//function to write the content of a given buffer to a given output file
void doWrite(int fd, const char *buff, int len) {
        int writevalue;
        int idx=0;
        do {
                writevalue=write(fd, buff+idx, len-idx);
                if (writevalue==-1) {   //an error has occurred
                        perror("write");    //print descripitve error message
                        exit(1);
                }
                idx+=writevalue;
        }
        while (idx<len);        //loop until the end of the buffer's content
        return;
}
//function to read given input file using a buffer and call doWrite to write to the given output file:
void write_file(int fd, const char *infile) {
        int inf;        //file descriptor of input file will be saved here
        inf=open(infile, O_RDONLY);     //input file opened for read only
        if (inf==-1) {          //if an error occurs, open returns -1
                perror(infile);         //print descriptive error message
                exit(1);
        }
        ssize_t value;
        char buff[1024];
        for (;;) {      //loop until the whole file is read
                value=read(inf, buff, sizeof(buff)-1);  //read fron input file, as many bytes as the buffer size -1 allows
                if (value==0) break;    //the whole file is read/EOF is encountered
                if (value==-1) {        //an erroe has occurred
                        perror("read");
                        exit(1);
                }
                buff[value]='\0';       //end of string character (marks end of words to be coppied from thw buffer)
                int len=strlen(buff);
                doWrite(fd, buff, len);         //write content of buffer to output file
        }
        close(inf);
        return;
}


int main (int argc, char **argv) {
        if (argc<3 || argc>4) {
                printf("Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)\n");
                return 1;
        }
        char **infile=argv;

        //open (or create and open) output file:
        char *outfile;          //outfile will keep the name of the output file
        if (argc==4) {                  //outfile is the given output file
                outfile=argv[3];
        }
        else outfile="fconc.out";       //if no output file is given, outfile becomes fconc.out
        int outf, oflags, mode;         //create variable of file descriptor of output file, create and set mode and flags for fconc.out
        oflags=O_APPEND | O_CREAT | O_WRONLY | O_TRUNC;
        mode= S_IRUSR | S_IWUSR;
        outf=open(outfile, oflags, mode);       //if no output file is given outfile="fconc.out" is created (O_CREAT)

        write_file(outf, infile[1]);    //call write_file function to copy the first and the second file to the output file
        write_file(outf, infile[2]);
        close(outf);
        return 0;
}
