/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
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
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");

    disk = NULL;
    size = 0;
    files = NULL;
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system form disk\n");

    //associates with disk
    disk = _disk;
    size = disk->size();
    
    //create files array
    const unsigned int arrSize = size / 512;
    files = new File*[arrSize];

    return true;
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) {
    Console::puts("formatting disk\n");

    //create empty buffer
    unsigned char * buf = new unsigned char[512];
    for(int i = 0; i < 512; i++)
    {
      buf[i] = 0;
    }

    //write empty buffer to each disk to wipe it
    for(unsigned long i = 0; i < _disk->size() / 512; i++)
    {    
      _disk->write(i, buf);
    }

    //deallocate buffer
    delete buf;
    return true;
}

File * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file\n");

    //return the file at the index of the file id (could be null)
    return files[_file_id];
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file\n");

    //check if file already exists with id and valid time to create
    if(files[_file_id] != NULL || disk == NULL || _file_id < 0 || _file_id >= size / 512)
    {
      return false;
    }

    //create file
    files[_file_id] = new File(_file_id, 0, disk);
    return true;
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file\n");

    //check if valid file system
    if(disk == NULL || _file_id < 0 || _file_id >= size / 512)
    {
      return false;
    }

    //if file doesnt exist we don't have to do anything
    if(files[_file_id] == NULL)
    {
      return true;
    }
    
    //delete the file
    files[_file_id]->Rewrite();
    delete files[_file_id];
    files[_file_id] = NULL;
    return true;
}
