#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>


#define SUCCESS 0
#define FAIL    -1

#define IMG_NAME         1
#define MAX_FILENAME_LEN 64
#define MAX_DIM_DIGITS   10

#define P_SIMD 2^0  // SIMD optimisation flag


typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t padding;  // force 32-bit / 4-byte sized structs
} pixel_t;

typedef struct {
    uint32_t size_x;
    uint32_t size_y;
    pixel_t *pixels;  // should be size_x * size_y pixels long
    uint32_t max_val;
} img_t;


void destroy_img(img_t *img_ptr);

img_t* new_img(void);

int get_file_metadata(img_t *img, FILE *img_file);

void _get_digit(char *dest_string, FILE *img_file);

int load_file(img_t *img, FILE *img_file);

int process_image(img_t *img, const char *filename, uint32_t FLAGS);

void greyscale(img_t *dest_img, img_t *src_img);

void invert(img_t *dest_img, img_t *src_img);

void brightness(img_t *dest_img, img_t *src_img, double multiplier);

void greyscale_SIMD(img_t *dest_img, img_t *src_img);

void invert_SIMD(img_t *dest_img, img_t *src_img);

void brightness_SIMD(img_t *dest_img, img_t *src_img, double multiplier);


int main(int argc, char **argv) {
    char img_filename[MAX_FILENAME_LEN];
    FILE *img_file;
    img_t *img;

    if (argc != 2) {
        fprintf(stderr, "Usage:\n%s <filename>\n\n", argv[0]);
        return FAIL;
    }

    strncpy(img_filename, argv[IMG_NAME], MAX_FILENAME_LEN);

    img_file = fopen(img_filename, "r");

    if (img_file == NULL) {
        fprintf(stderr, "Could not open file <%s>. Program exited.\n", img_filename);
        return FAIL;
    }

    img = new_img();

    if (NULL == img) {
        fprintf(stderr, "Image memory allocation failed\n");
        return FAIL;
    }

    if (load_file(img, img_file) == FAIL) {
        fprintf(stderr, "Could not parse file <%s>. Program exited.\n", img_filename);
        destroy_img(img);
        return FAIL;
    }

    destroy_img(img);

    fclose(img_file);

    return SUCCESS;
}


void destroy_img(img_t *img_ptr) {
    if (img_ptr->pixels != NULL) free(img_ptr->pixels);  // NULL implies no pixels were allocated
    free(img_ptr);
}

img_t* new_img(void) {
    img_t *img_ptr = (img_t*)malloc(sizeof(img_t));
    if (img_ptr != NULL) {
        img_ptr->pixels = NULL;  // this will be used in cleanup to determine
                                 // whether pixels were allocated or not
    } 
    return img_ptr;
}

int load_file(img_t *img, FILE *img_file) {
    get_file_metadata(img, img_file);

    return SUCCESS;
}


int get_file_metadata(img_t *img, FILE *img_file) {
    const char valid_ppm_version = '6';
    const uint32_t valid_max_rgb_value = 255;

    char digit_buffer[MAX_DIM_DIGITS + 1] = { 0 };

    fgetc(img_file);  // skip to second character in the file

    if (fgetc(img_file) != valid_ppm_version) {
        fprintf(stderr, "Program detected file that was not P6 .ppm file.\nThis program only accepts P6 .ppm files.\n");
        return FAIL;
    }

    fgetc(img_file);  // skip newline character after 'P6'

    // load first digit (size_x)
    _get_digit(digit_buffer, img_file);

    if ((img->size_x = (uint32_t)atoi(digit_buffer)) == 0) {
        fprintf(stderr, "Was not able to parse P6 .ppm file width.\n");
        return FAIL;
    }

    // load second digit (size_y)
    _get_digit(digit_buffer, img_file);

    if ((img->size_y = (uint32_t)atoi(digit_buffer)) == 0) {
        fprintf(stderr, "Was not able to parse P6 .ppm file height.\n");
        return FAIL;
    }

    // it is assumed below that MAX_DIM_DIGITS is larger than
    // the number of digits used for the max RGB colour value
    // load final digit (max_val)
    _get_digit(digit_buffer, img_file);

    if ((img->max_val = (uint32_t)atoi(digit_buffer)) == 0) {
        fprintf(stderr, "Was not able to parse P6 .ppm max colour value.\n");
        return FAIL;
    }

    if (img->max_val != valid_max_rgb_value) {
        fprintf(stderr, "This program only supports P6 files with maximum RGB value of %d.\n", valid_max_rgb_value);
        return FAIL;
    }

    return SUCCESS;
}


void _get_digit(char *dest_string, FILE *img_file) {
    int char_buffer;
    int i = 0;
    while ((char_buffer = fgetc(img_file)) != EOF && !isspace(char_buffer)) {
        if (i < MAX_DIM_DIGITS) {
            dest_string[i++] = (char)char_buffer;
        }
    }

    dest_string[i] = '\0';
}


int process_image(img_t *img, const char *filename, uint32_t flags) {
    img_t* new_img;

    if (flags & P_SIMD) {
        greyscale_SIMD(new_img, img);
        invert_SIMD(new_img, img);
        brightness_SIMD(new_img, img, 1.5);
    }

    greyscale(new_img, img);
    invert(new_img, img);
    brightness(new_img, img, 1.5);

    return SUCCESS;
}


void greyscale(img_t *dest_img, img_t *src_img) {
}


void invert(img_t *dest_img, img_t *src_img) {
}


void brightness(img_t *dest_img, img_t *src_img, double multiplier) {
}

void greyscale_SIMD(img_t *dest_img, img_t *src_img) {
}


void invert_SIMD(img_t *dest_img, img_t *src_img) {
}


void brightness_SIMD(img_t *dest_img, img_t *src_img, double multiplier) {
}
