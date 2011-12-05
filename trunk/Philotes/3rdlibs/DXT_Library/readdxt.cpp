#include <direct.h>
#include "string"
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <windows.h>


#include "dxtlib.h"

#include <sys/stat.h>

static FILE * fpin;

HFILE file;


std::string ddsname;

// read .dds, write .tga

void writetga_from_memory(int w, int h, int bpp, const unsigned char * data,  
                          const unsigned int& theStrideInBytes, const char * filename,
                          ColorFormat color_format);



int main(int argc, char * argv[])
{
    
    if (argc != 2)
    {
        return 0;
    }

    ddsname = argv[1];

    file = _open( ddsname.c_str(), _O_RDONLY | _O_BINARY ,  _S_IREAD );

    if (file == -1)
    {
        fprintf(stderr, "Can't open output file\n", ddsname.c_str());
        return 0;
    }
        

    int width;
    int height;
    int planes;
    int lTotalWidth; 
    int rowBytes;
    int src_format;
        
    unsigned char * data = nvDXTdecompress(width, height, planes, lTotalWidth, rowBytes, src_format);
    
    //unsigned char * pDest; 
    //unsigned char * pSrc; 

    if (planes == 3)
        writetga_from_memory(width, height, 24, data,  
        lTotalWidth * planes, "test.tga", COLOR_BGR);
    else
        writetga_from_memory(width, height, 32, data,  
        lTotalWidth * planes, "test.tga", COLOR_BGRA);



    /*for( int row = 0; row < height; row ++ )
    {
        pSrc = data + (row * lTotalWidth * depth );
        
        for( int i = 0 ; i < rowBytes; i++)
        {
            pDest = pSrc[i];
            //put data where you want it

        }
    }  */


        
    delete [] data;


    _close(file);

    //delete [] raw_data;

    return 0;
}
void WriteDTXnFile (DWORD count, void *buffer, void * userData)
{

}


void ReadDTXnFile (DWORD count, void *buffer, void * userData)
{
        
    _read(file, buffer, count);

}

