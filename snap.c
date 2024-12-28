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

    // Ensure the framebuffer is 32 bpp
    if (vinfo.bits_per_pixel != 32)
    {
        fprintf(stderr, "%s: only 32 bits per pixel supported\n", program);
        exit(EXIT_FAILURE);
    }

    size_t framebuffer_size = vinfo.xres * vinfo.yres * 4;  // 4 bytes per pixel (32 bpp)
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
        8,                      // Bit depth of 8 for grayscale
        PNG_COLOR_TYPE_GRAY,    // Grayscale image
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    png_bytep png_row = (png_bytep)malloc(vinfo.xres); // Buffer for a single row of the PNG image

    if (png_row == NULL)
    {
        fprintf(stderr, "%s: Unable to allocate buffer\n", program);
        free(fbp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(pngfp);
        exit(EXIT_FAILURE);
    }

    //--------------------------------------------------------------------

    uint32_t pixel_idx = 0;

    // Loop through framebuffer data and create PNG row by row
    for (uint32_t y = 0; y < vinfo.yres; ++y)
    {
        // Copy a row of framebuffer data
        uint32_t *row_data = (uint32_t *)(fbp + y * vinfo.xres * 4);  // 4 bytes per pixel for 32 bpp
        memset(png_row, 0, vinfo.xres);

        for (uint32_t x = 0; x < vinfo.xres; ++x)
        {
            uint32_t pixel = row_data[x];
            uint8_t red = (pixel >> 24) & 0xFF;
            uint8_t green = (pixel >> 16) & 0xFF;
            uint8_t blue = (pixel >> 8) & 0xFF;

            // Convert to grayscale
            uint8_t gray = (red + green + blue) / 3;

            png_row[x] = gray;  // Set pixel value in grayscale
        }

        png_write_row(png_ptr, png_row);  // Write the row to the PNG
    }

    //--------------------------------------------------------------------

    free(fbp);
    free(png_row);
    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(pngfp);

    //--------------------------------------------------------------------

    return 0;
}
