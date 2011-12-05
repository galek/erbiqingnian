#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>

#include <windows.h>
#include "WriteTGA.h"

#define _R 0
#define _G 1
#define _B 2
#define _A 3

enum ColorFormat
{
	COLOR_RGB,
	COLOR_ARGB,
	COLOR_BGR,
	COLOR_BGRA,
	COLOR_RGBA,
	COLOR_ABGR,
};


static Byte id_length;                 /* 0 */
static Byte color_map_type;            /* 1 */
static Byte image_type;                /* 2 */
static int cm_index;                   /* 3 4 */
static int cm_length;                  /* 5 6 */
static Byte cm_entry_size;             /* 7 */
static int image_x_org;                /* 8 9 */
static int image_y_org;                /* 10 11 */
static int image_width;                /* 12 13 */
static int image_height;               /* 14 15 */
static Byte image_depth;               /* 16 */
static Byte image_desc;                /* 17 */

static Byte color_palette[256][3];

static char buffer[1024];

static FILE *fpin;
static FILE *fpout;
static FILE *cmfp;

static Byte pixel[4];

void tgaFetchColor(const unsigned char * data, int &a, int &r, int &g, int &b, ColorFormat fmt)
{  

	switch(fmt)
	{
	case COLOR_ARGB:
		a = *data++;
		r = *data++;
		g = *data++;
		b = *data++;
		break;

	case COLOR_RGBA:
		r = *data++;
		g = *data++;
		b = *data++;
		a = *data++;
		break;

    case COLOR_ABGR:
		a = *data++;
		b = *data++;
		g = *data++;
		r = *data++;
		break;
    
    case COLOR_RGB:
		r = *data++;
		g = *data++;
		b = *data++;
		a = 0xFF;
		break;
	case COLOR_BGR:
		b = *data++;
		g = *data++;
		r = *data++;
		a = 0xFF;
		break;
	case COLOR_BGRA:
		b = *data++;
		g = *data++;
		r = *data++;
		a = *data++;
		break;
	}
}


void 
wwrite(FILE * fp, unsigned short data)
{
    Byte h, l;
    
    l = data & 0xFF;
    h = data >> 8;
    fputc(l, fp);
    fputc(h, fp);
    
}

char id[] =
{"Created by Douglas H. Rogers"};

void 
__write_tga(const char *filename, const char *outname)
{
    int x, y;
    int a,r, g, b;
    unsigned char c;
    int i;
    
    if ((fpin = fopen(filename, "rb")) == NULL)
    {
        printf("Cannot open file '%s'\n", filename);
        exit(1);
    }
    if ((fpout = fopen(outname, "wb")) == NULL)
    {
        printf("Cannot open file '%s'\n", filename);
        exit(1);
    }
    
    //image_desc |= LR;   // left right
    //image_desc |= BT;   // top
    
    bwrite(fpout, id_length);
    bwrite(fpout, color_map_type);
    bwrite(fpout, image_type);
    
    wwrite(fpout, cm_index);
    wwrite(fpout, cm_length);
    
    bwrite(fpout, cm_entry_size);
    
    wwrite(fpout, image_x_org);
    wwrite(fpout, image_y_org);
    wwrite(fpout, image_width);
    wwrite(fpout, image_height);
    
    bwrite(fpout, image_depth);
    bwrite(fpout, image_desc);
    
    printf("id_length %d\n", id_length);
    printf("color_map_type %d\n", color_map_type);
    printf("image_type %d\n", image_type);
    printf("cm_index %d\n", cm_index);
    printf("cm_length %d\n", cm_length);
    printf("cm_entry_size %d\n", cm_entry_size);
    printf("image_x_org %d\n", image_x_org);
    printf("image_y_org %d\n", image_y_org);
    printf("image_width %d\n", image_width);
    printf("image_height %d\n", image_height);
    printf("image_depth %d\n", image_depth);
    printf("image_desc %02X\n", image_desc);
    
    if (image_depth == 8)
    {
        printf("reading color map\n");
        // eat first three lines
        fgets(buffer, 1024, cmfp);
        fgets(buffer, 1024, cmfp);
        fgets(buffer, 1024, cmfp);
        
        i = 0;
        while (fgets(buffer, 1024, cmfp))
        {
            sscanf(buffer, "%d %d %d\n", &r, &g, &b);
            //printf("r %d g %d b %d\n", r,g,b);
            color_palette[i][_R] = r;
            color_palette[i][_G] = g;
            color_palette[i][_B] = b;
            i++;
        }
        //fwrite (id, id_length, 1, fpout);
        fwrite ((char *) &color_palette, cm_length, cm_entry_size >> 3, fpout);
        for (y = 0; y < image_height; y++)
            for (x = 0; x < image_width; x++)
            {
                fread(&c, 1, 1, fpin);
                fputc(c, fpout);
            }
            
    }
    else
    {
        for (y = 0; y < image_height; y++)
            for (x = 0; x < image_width; x++)
            {
                fread((char *) pixel, 1, image_depth>> 3, fpin);
                r = pixel[_R];
                g = pixel[_G];
                b = pixel[_B];

                if (image_depth == 32)
                    a = pixel[3];
                
                fputc(b, fpout);
                fputc(g, fpout);
                fputc(r, fpout);

                if (image_depth == 32)
                    fputc(a, fpout);
            }
    }
    
    fclose(fpout);
    fclose(fpin);
    
}

int writetga(int w, int h, int bpp, const char * infile, 
             const char * outfile, const char * cmp_filename)
{
    //id_length = sizeof(id);
    id_length = 0;
    image_x_org = 0;
    image_y_org = 0;
    image_desc = 0x20;
    
    
    image_width = w;
    image_height = h;
    image_depth = bpp;
    
    if (image_depth == 8)
    {
        if (cmp_filename == 0)
        {
            printf("Need color map file for 8 bit mode\n");
            exit(1);
        }
        cmfp = fopen(cmp_filename, "r");
        if (cmfp == NULL)
        {
            printf("Can't open %s\n", cmp_filename);
            exit(1);
        }
        cm_index = 0;
        cm_length = 256;
        cm_entry_size = 24;
        color_map_type = 1;
        image_type = 1;
    }
    else if (image_depth == 24 || image_depth == 32)
    {
        cmfp = NULL;
        cm_index = 0;
        cm_length = 0;
        cm_entry_size = 0;
        color_map_type = 0;
        image_type = 2;
    }
    else
    {
        printf("bpp must be 8 or 24 or 32\n");
        exit(1);
    }
    
    __write_tga(infile, outfile);
    if (cmfp)
        fclose(cmfp);
    
    return 0;
}

/////////////////
void writetga_from_memory(int w, int h, int bpp, const unsigned char * data,  
                          const unsigned int& theStrideInBytes, const char * filename,
                          ColorFormat color_format)
{
    //id_length = sizeof(id);
    id_length = 0;
    image_x_org = 0;
    image_y_org = 0;
    image_desc = 0x20;
    
    
    image_width = w;
    image_height = h;
    image_depth = bpp;
    
    if (image_depth == 24 || image_depth == 32)
    {
        cmfp = NULL;
        cm_index = 0;
        cm_length = 0;
        cm_entry_size = 0;
        color_map_type = 0;
        image_type = 2;
    }
    else
    {
		return ;
    }

    int x, y;
    int a,r, g, b;
    //unsigned char c;
    //int i;
    
    if ((fpout = fopen(filename, "wb")) == NULL)
    {
        printf("Cannot open file '%s'\n", filename);
		return;
        
    }
    
    //image_desc |= LR;   // left right
    //image_desc |= BT;   // top
    
    bwrite(fpout, id_length);
    bwrite(fpout, color_map_type);
    bwrite(fpout, image_type);
    
    wwrite(fpout, cm_index);
    wwrite(fpout, cm_length);
    
    bwrite(fpout, cm_entry_size);
    
    wwrite(fpout, image_x_org);
    wwrite(fpout, image_y_org);
    wwrite(fpout, image_width);
    wwrite(fpout, image_height);
    
    bwrite(fpout, image_depth);
    bwrite(fpout, image_desc);
    
    unsigned char* pSource = const_cast< unsigned char* >(data);

	int pixel_depth = image_depth >> 3;
	for (y = 0; y < image_height; y++)
	{
		pSource = (unsigned char*)(data + ( y * theStrideInBytes ));
		for (x = 0; x < image_width; x++)
		{
			tgaFetchColor(pSource, a, r, g, b, color_format);
			
			fputc(b, fpout);
			fputc(g, fpout);
			fputc(r, fpout);

			if (image_depth == 32)
				fputc(a, fpout);
			pSource += pixel_depth;
		}
	}	
    fclose(fpout);
    
}

void bwrite2(HFILE hFile, unsigned char data) 
{
	_write(hFile, &data, 1);
}
void 
wwrite2(HFILE hFile, unsigned short data)
{
    Byte h, l;
    
    l = data & 0xFF;
    h = data >> 8;

	_write(hFile, &l, 1);
	_write(hFile, &h, 1);
    
}

void writetga_from_memory(int w, int h, int bpp, const unsigned char * data,  HFILE hFile,
			  ColorFormat color_format)
{
    //id_length = sizeof(id);
    id_length = 0;
    image_x_org = 0;
    image_y_org = 0;
    image_desc = 0x20;
    
    
    image_width = w;
    image_height = h;
    image_depth = bpp;
    
    if (image_depth == 24 || image_depth == 32)
    {
        cmfp = NULL;
        cm_index = 0;
        cm_length = 0;
        cm_entry_size = 0;
        color_map_type = 0;
        image_type = 2;
    }
    else
    {
		return ;
    }

    int x, y;
    int a,r, g, b;
    //unsigned char c;
    //int i;
    
    
    //image_desc |= LR;   // left right
    //image_desc |= BT;   // top
    
    bwrite2(hFile, id_length);
    bwrite2(hFile, color_map_type);
    bwrite2(hFile, image_type);
    
    wwrite2(hFile, cm_index);
    wwrite2(hFile, cm_length);
    
    bwrite2(hFile, cm_entry_size);
    
    wwrite2(hFile, image_x_org);
    wwrite2(hFile, image_y_org);
    wwrite2(hFile, image_width);
    wwrite2(hFile, image_height);
    
    bwrite2(hFile, image_depth);
    bwrite2(hFile, image_desc);
    
    
	int pixel_depth = image_depth >> 3;
	for (y = 0; y < image_height; y++)
		for (x = 0; x < image_width; x++)
		{

			tgaFetchColor(data, a, r, g, b, color_format);
			
			bwrite2(hFile, b);
			bwrite2(hFile, g);
			bwrite2(hFile, r);

			if (image_depth == 32)
				bwrite2(hFile, a);
			data += pixel_depth;
		}
    
}



