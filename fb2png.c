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
    uint8_t *fbp = (uint8_t *)malloc(framebuffer_size);

    if (fbp == NULL)
    {
        fprintf(stderr, "%s: failed to allocate framebuffer\n", program);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read = read(fbfd, fbp, framebuffer_size);
    close(fbfd);

    if (bytes_read != framebuffer_size)
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
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fprintf(stderr, "%s: error creating PNG\n", program);
        free(fbp);
        exit(EXIT_FAILURE);
    }

    FILE *pngfp = fopen(pngname, "wb");

    if (pngfp == NULL)
    {
        fprintf(stderr, "%s: Unable to create %s\n", program, pngname);
        free(fbp);
        exit(EXIT_FAILURE);
    }

    png_init_io(png_ptr, pngfp);

    png_set_IHDR(
        png_ptr,
        info_ptr,
        vinfo.xres,
        vinfo.yres,
        1,  // Bit depth of 1
        PNG_COLOR_TYPE_GRAY,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    png_bytep png_buffer = (png_bytep)malloc((vinfo.xres + 7) / 8 * vinfo.yres);

    if (png_buffer == NULL)
    {
        fprintf(stderr, "%s: Unable to allocate buffer\n", program);
        free(fbp);
        fclose(pngfp);
        exit(EXIT_FAILURE);
    }

    //--------------------------------------------------------------------

    size_t png_buffer_index = 0;

    for (size_t y = 0; y < vinfo.yres; y++)
    {
        size_t x;

        for (x = 0; x < vinfo.xres; x++)
        {
            size_t fb_offset = x + y * vinfo.xres;
            uint8_t pixel = (fbp[fb_offset / 8] >> (7 - (fb_offset % 8))) & 0x01;
            png_buffer[png_buffer_index / 8] |= (pixel << (7 - (png_buffer_index % 8)));
            png_buffer_index++;
        }
    }

    //--------------------------------------------------------------------

    free(fbp);
    png_write_image(png_ptr, &png_buffer);
    free(png_buffer);
    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(pngfp);

    //--------------------------------------------------------------------

    return 0;
}
