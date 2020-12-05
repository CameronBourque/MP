/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(unsigned long _file_id, unsigned int _size, SimpleDisk * _disk) {
    /* We will need some arguments for the constructor, maybe pointer to disk
     block with file management and allocation data. */
    Console::puts("In file constructor.\n");

    //set private variables
    file_id = _file_id;
    size = _size;
    disk = _disk;
    pos = 0;
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char * _buf) {
    Console::puts("reading from file\n");

    //read disk contents to buffer
    unsigned char * buf = new unsigned char[512];
    disk->read(file_id, buf);

    //get from the current position and increment    
    unsigned int i = 0;
    for(i = 0; i < _n; i++)
    {
      _buf[i] = (char)buf[pos];
      pos++;

      //if we reach eof then stop reading
      if(EoF())
      {
        break;
      }
    }

    //return amount read
    delete buf;
    return (int)i;
}


void File::Write(unsigned int _n, const char * _buf) {
    Console::puts("writing to file\n");

    //read buffer from disk
    unsigned char * buf = new unsigned char[512];
    disk->read(file_id, buf);

    //go through file and write to it
    unsigned int i = 0;
    for(i = 0; i < _n; i++)
    {
      //set current position to current index in passed in buffer
      buf[pos] = (unsigned char)_buf[i];
      pos++;

      //if eof increase the size
      if(EoF())
      {
        size++;
      }
    }

    //write updated buffer to disk
    disk->write(file_id, buf);
    delete buf;
}

void File::Reset() {
    Console::puts("reset current position in file\n");
    //set current position to start index
    pos = 0;
}

void File::Rewrite() {
    Console::puts("erase content of file\n");
    
    //set each spot in the buffer to 0
    unsigned char * buf = new unsigned char[512];
    unsigned long i = 0;
    for(i = 0; i < size; i++)
    {
      buf[i] = 0;
    }

    //write file to disk
    disk->write(file_id, buf);
    delete buf;
}


bool File::EoF() {
    Console::puts("testing end-of-file condition\n");
    //if current position is at size index
    return pos == size;
}
