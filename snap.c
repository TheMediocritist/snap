#include <errno.h>
#include <fcntl.h>
#include <png.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
    char *program = argv[0];
    char *fbdevice = "/dev/fb1";
    char *pngname = "fb.png";

    int opt = 0;

    //--------------------------------------------------------------------

    while ((opt = getopt(argc, argv, "d:p:")) != -1)
    {
        switch (opt)
        {
        case 'd':
            fbdevice = optarg;
            break;

        case 'p':
            pngname = optarg;
            break;

        default:
            fprintf(stderr,
                    "Usage: %s [-d device] [-p pngname]\n",
                    program);
            exit(EXIT_FAILURE);
            break;
        }
    }

    //--------------------------------------------------------------------

    int fbfd = open(fbdevice, O_RDONLY);

    if (fbfd == -1)
    {
        fprintf(stderr,
                "%s: cannot open framebuffer - %s",
                program,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        fprintf(stderr,
                "%s: reading framebuffer variable information - %s",
                program,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (vinfo.bits_per_pixel != 8)
    {
        fprintf(stderr, "%s: only 8 bits per pixel supported\n", program);
        exit(EXIT_FAILURE);
    }

    size_t framebuffer_size = vinfo.xres * vinfo.yres;
    uint8_t *fbp = (uint8_t *)malloc(framebuffer_size / 8);

    if (fbp == NULL)
    {
        fprintf(stderr, "%s: failed to allocate framebuffer\n", program);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read = read(fbfd, fbp, framebuffer_size / 8);
    close(fbfd);

    if (bytes_read != framebuffer_size / 8)
    {
        fprintf(stderr,
                "%s: failed to read framebuffer - %s",
                program,
                strerror(errno));
        free(fbp);
        exit(EXIT_FAILURE);
    }

    //--------------------------------------------------------------------

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                  NULL,
                                                  NULL,
                                                  NULL);

    if (png_ptr == NULL)
    {
        fprintf(stderr,
                "%s: could not allocate PNG write struct\n",
                program);
        free(fbp);
        exit(EXIT_FAILURE);
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);

    if (info_ptr == NULL)
    {
        fprintf(stderr,
                "%s: could not allocate PNG info struct\n",
                program);
        free(fbp);
        png_destroy_write_struct(&png_ptr, NULL);
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        fprintf(stderr, "%s: error during PNG creation\n", program);
        free(fbp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        exit(EXIT_FAILURE);
    }

    FILE *pngfp = fopen(pngname, "wb");

    if (pngfp == NULL)
    {
        fprintf(stderr, "%s: Unable to create %s\n", program, pngname);
        free(fbp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        exit(EXIT_FAILURE);
    }

    png_init_io(png_ptr, pngfp);

    png_set_IHDR(
        png_ptr,
        info_ptr,
        vinfo.xres,
        vinfo.yres,
        1,                      // Bit depth of 1
        PNG_COLOR_TYPE_GRAY,    // Grayscale image
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    png_bytep png_row = (png_bytep)malloc((vinfo.xres / 8) + 1); // Buffer for a single row of the PNG image

    if (png_row == NULL)
    {
        fprintf(stderr, "%s: Unable to allocate buffer\n", program);
        free(fbp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(pngfp);
        exit(EXIT_FAILURE);
    }

    //--------------------------------------------------------------------
    
    char buffer_chunk[8];
    uint16_t fb_chunk; 			// chunk counter (8 bits of fb1 per chunk)
    uint8_t fb_chunk_bit;		// bit counter 
    
    // loop through pixels
	for (fb_chunk = 0; fb_chunk < 12000; fb_chunk++)
    {
        // copy data chunk _from_ framebuffer 
		memcpy(buffer_chunk, fbp + (fb_chunk * 8), 8);
    }
    
    for (size_t y = 0; y < 240; y++)
    {
        png_bytep png_row_ptr = png_row;
        
        for (fb_chunk_bit = 0; fb_chunk_bit < 8; fb_chunk_bit++)
		{
            uint8_t oldbit = buffer_chunk[fb_chunk_bit];

            *png_row_ptr |= (oldbit << (7 - (x % 8)));

            if ((fb_chunk + 1) % 50 == 0)
            {
                png_row_ptr++;
                *png_row_ptr = 0;
            }
        }

        png_write_row(png_ptr, png_row);
    }

    png_write_row(png_ptr, png_row); // Write the last row

    //--------------------------------------------------------------------

    free(fbp);
    free(png_row);
    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(pngfp);

    //--------------------------------------------------------------------

    return 0;
}
