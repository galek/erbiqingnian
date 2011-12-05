#include <stdio.h>
#include <windows.h>

/*
    Author: Krassimir Touevsky,
        Treyarch Corp.
 */

FILE* file = 0;

void ReadDTXnFile( DWORD count, void * buffer, void * userData )
{
    fread( buffer, count, 1, file );
}

void WriteDTXnFile(unsigned long,void *, void * userData)
{
}

#include "ulMem.h"
#include "dxtlib.h"

int main( int argc, char* argv[] )
{
    file = fopen( argv[1], "rb" );
    char tga_filename[260];
    char c;
    short n;
    long l;

    if ( ! file )
        return -1;

    int w, h, depth, total_width, row_bytes, src_format;
    unsigned char* data = nvDXTdecompress( w, h, depth, total_width, row_bytes, src_format );
    fclose( file );

    strcpy( tga_filename, argv[1] );
    strcat( tga_filename, ".tga" );
    file = fopen( tga_filename, "wb" );

    fwrite( &(n=0), 1, 2, file );
    fwrite( &(c=2), 1, 1, file );
    fwrite( &(l=0), 4, 1, file );
    fwrite( &(c=0), 1, 1, file );
    fwrite( &(n=0), 2, 1, file );
    fwrite( &(n=0), 2, 1, file );
    fwrite( &(n=w), 2, 1, file );
    fwrite( &(n=h), 2, 1, file );
    fwrite( &(c=depth*8), 1, 1, file );
    fwrite( &(c=0), 1, 1, file );

    int image_pitch = total_width * depth;

    for ( l = h; l--; )
    {
        fwrite( data + l * image_pitch, row_bytes, 1, file );
    }

    fclose( file );

    mem_free( data );

    return 0;
}