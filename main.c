#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <simde/x86/avx2.h>


#define SUCCESS 0
#define FAIL    -1

#define IMG_NAME         1
#define MAX_FILENAME_LEN 64
#define MAX_DIM_DIGITS   10

#define I_SHAPE_ONLY (uint32_t)pow(2, 0)  // copy image flag

#define P_SIMD       (uint32_t)pow(2, 0)  // SIMD optimisation flag


typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} pixel_t;

typedef struct {
    uint32_t size_x;
    uint32_t size_y;
    pixel_t *pixels;  // should be size_x * size_y pixels long
    uint32_t max_val;
} img_t;

typedef enum {
    OP_GREYSCALE,
    OP_INVERT,
    OP_BRIGHTEN,

    OP_COUNT
} img_op_t;

typedef void (*img_op_func_t)(img_t *dest_img, img_t *src_img);

typedef struct {
    const char *name;
    img_op_func_t func;
    img_op_func_t simd_func;
} img_op_entry_t;


void destroy_img(img_t *img_ptr);

img_t* new_img(void);

img_t* copy_img(img_t *src_img, uint32_t flags);

int save_file(img_t *img, const char *filename);

int get_file_metadata(img_t *img, FILE *img_file);

void _get_digit(char *dest_string, FILE *img_file);

int load_file(img_t *img, FILE *img_file);

int process_img(img_t *img, char *filename, uint32_t flags);

void greyscale(img_t *dest_img, img_t *src_img);

void invert(img_t *dest_img, img_t *src_img);

void brighten(img_t *dest_img, img_t *src_img);

void greyscale_SIMD(img_t *dest_img, img_t *src_img);

void invert_SIMD(img_t *dest_img, img_t *src_img);

void brighten_SIMD(img_t *dest_img, img_t *src_img);

const char* get_basename(char *filename);


const img_op_entry_t operations[OP_COUNT] = {
    [OP_GREYSCALE] = {"greyscale", greyscale, greyscale_SIMD},
    [OP_INVERT] = {"invert", invert, invert_SIMD},
    [OP_BRIGHTEN] = {"brighten", brighten, brighten_SIMD},
};


int main(int argc, char **argv) {
    char img_filename[MAX_FILENAME_LEN];
    FILE *img_file;
    img_t *img;

    if (argc != 2) {
        fprintf(stderr, "Usage:\n%s <filename>\n\n", argv[0]);
        return FAIL;
    }

    strncpy(img_filename, argv[IMG_NAME], MAX_FILENAME_LEN);

    img_file = fopen(img_filename, "rb");

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

    process_img(img, img_filename, P_SIMD);
    process_img(img, img_filename, 0);

    destroy_img(img);

    fclose(img_file);

    return SUCCESS;
}


void destroy_img(img_t *img_ptr) {
    if (NULL == img_ptr) return;
    if (img_ptr->pixels != NULL) free(img_ptr->pixels);  // NULL implies no pixels were allocated
    free(img_ptr);
}


img_t* new_img(void) {
    img_t *img_ptr = (img_t*)malloc(sizeof(img_t));
    if (img_ptr != NULL) img_ptr->pixels = NULL;  // this will be used in cleanup (destroy_img) to
                                                  // determine whether pixels were allocated or not 
    return img_ptr;
}


img_t* copy_img(img_t *src_img, uint32_t flags) {
    img_t *img_ptr;
    pixel_t *pxls_ptr;
    uint32_t img_size = src_img->size_x * src_img->size_y;

    img_ptr = (img_t*)malloc(sizeof(img_t));
    if (NULL == img_ptr) return NULL;

    *img_ptr = *src_img;

    pxls_ptr = (pixel_t*)malloc(sizeof(pixel_t) * img_size);
    if (NULL == pxls_ptr) {
        free(img_ptr);
        return NULL;
    }

    img_ptr->pixels = pxls_ptr;
    if (!(flags & I_SHAPE_ONLY)) {
        memcpy(img_ptr->pixels, src_img->pixels, sizeof(pixel_t) * img_size);
    }

    return img_ptr;
}


int load_file(img_t *img, FILE *img_file) {
    uint32_t img_size;
    uint32_t pixels_read;

    get_file_metadata(img, img_file);
    // it is assumed file pointer/cursor is at the correct position to read pixel data
    
    img_size = img->size_x * img->size_y;

    img->pixels = (pixel_t*)malloc(sizeof(pixel_t) * img_size);
    if (NULL == img->pixels) {
        fprintf(stderr, "Memory allocation error. Exiting program.\n");
        return FAIL;
    }

    pixels_read = fread(img->pixels, sizeof(pixel_t), img_size, img_file);
    
    if (pixels_read != img_size) {
        fprintf(stderr, "Could not read all of file data correctly. Only read %u out of %u pixels\n", pixels_read, img_size);
        fprintf(stderr, "EOF: %s\n", feof(img_file) ? "true" : "false");
        fprintf(stderr, "File Error: %d\n", ferror(img_file));
        return FAIL;
    }

    return SUCCESS;
}


int save_file(img_t *img, const char *filename) {
    FILE *output_file = fopen(filename, "wb");
    if (NULL == output_file) {
        fprintf(stderr, "Could not open file <%s> to write to.\n", filename);
        perror(filename);
        return FAIL;
    }

    fprintf(output_file, "P6\n%u %u\n%u\n", img->size_x, img->size_y, img->max_val);
    fwrite(img->pixels, sizeof(pixel_t), img->size_x * img->size_y, output_file);

    fclose(output_file);

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


int process_img(img_t *img, char *filename, uint32_t flags) {
    img_t* img_buf = NULL;
    char new_filename[MAX_FILENAME_LEN + 1];
    char base_filename[MAX_FILENAME_LEN + 1];
    int file_save_check = SUCCESS;
    uint32_t f_ptr_offset;  // used to decide SIMD function or not
                           // measured in bytes - hence uint8_t
    img_op_func_t f_ptr_buffer;

    char *simd_label = (flags & P_SIMD) ? "SIMD_" : "";

    strncpy(base_filename, get_basename(filename), MAX_FILENAME_LEN);

    img_buf = copy_img(img, I_SHAPE_ONLY);
    if (NULL == img_buf) {
        fprintf(stderr, "Memory allocation failed.\n");
        return FAIL;
    }

    // choose either the SIMD function or std using ptr offsets inside the struct img_op_entry_t
    f_ptr_offset = (flags & P_SIMD) ? offsetof(img_op_entry_t, simd_func) : offsetof(img_op_entry_t, func);

    for (int i = 0; i < OP_COUNT; i++) {
        f_ptr_buffer = *(img_op_func_t*)((uint8_t*)(operations + i) + f_ptr_offset);
        f_ptr_buffer(img_buf, img);

        snprintf(
            new_filename, MAX_FILENAME_LEN + 1, "%s_%s%s.ppm",
            operations[i].name, simd_label, base_filename
        );
        file_save_check |= save_file(img_buf, new_filename);
    }

    destroy_img(img_buf);

    if (file_save_check != SUCCESS) {
        fprintf(stderr, "Saving new image files failed.\n");
        return FAIL;
    }

    return SUCCESS;
}


void greyscale(img_t *dest_img, img_t *src_img) {
    // it is assumed both images are of the same shape (via the copy_img_shape function)
    pixel_t *src_pxl;
    pixel_t *dest_pxl;
    uint32_t pxl_count = src_img->size_x * src_img->size_y;
    double brightness;
    
    for (src_pxl = src_img->pixels, dest_pxl = dest_img->pixels;
        (src_pxl - src_img->pixels) < pxl_count;
        src_pxl++, dest_pxl++) {
        // take the normalised brightness
        brightness = pow((src_pxl->r*src_pxl->r + src_pxl->g*src_pxl->g + src_pxl->b*src_pxl->b), 0.5) / sqrt(3);

        // should already be clamped to 255, but floating point stuff
        brightness = fmax(fmin(brightness, (double)src_img->max_val), 0.0);

        dest_pxl->r = dest_pxl->g = dest_pxl->b = (uint8_t)brightness;
    }
}


void invert(img_t *dest_img, img_t *src_img) {
    // it is assumed both images are of the same shape and max_val (via the copy_img_shape function)
    pixel_t *src_pxl;
    pixel_t *dest_pxl;
    uint32_t pxl_count = src_img->size_x * src_img->size_y;
    uint32_t max_val = src_img->max_val;
    
    for (src_pxl = src_img->pixels, dest_pxl = dest_img->pixels;
        (src_pxl - src_img->pixels) < pxl_count;
        src_pxl++, dest_pxl++) {
        dest_pxl->r = max_val - src_pxl->r;
        dest_pxl->g = max_val - src_pxl->g;
        dest_pxl->b = max_val - src_pxl->b;
    }
}


void brighten(img_t *dest_img, img_t *src_img) {
    // it is assumed both images are of the same shape (via the copy_img_shape function)
    const uint8_t boost = (uint8_t)src_img->max_val * 0.12;
    pixel_t *src_pxl;
    pixel_t *dest_pxl;
    uint32_t pxl_count = src_img->size_x * src_img->size_y;
    uint32_t max_val = src_img->max_val;
    
    for (src_pxl = src_img->pixels, dest_pxl = dest_img->pixels;
        (src_pxl - src_img->pixels) < pxl_count;
        src_pxl++, dest_pxl++) {

        dest_pxl->r = (src_pxl->r + boost) < max_val ? src_pxl->r + boost : max_val;
        dest_pxl->g = (src_pxl->g + boost) < max_val ? src_pxl->g + boost : max_val;
        dest_pxl->b = (src_pxl->b + boost) < max_val ? src_pxl->b + boost : max_val;
    }
}

void greyscale_SIMD(img_t *dest_img, img_t *src_img) {
    *dest_img = *src_img;
}


void invert_SIMD(img_t *dest_img, img_t *src_img) {
    // it is assumed both images are of the same shape and max_val (via the copy_img_shape function)
    uint8_t *src_byte = (uint8_t*)src_img->pixels;
    uint8_t *dest_byte = (uint8_t*)dest_img->pixels;
    uint32_t pxl_count = src_img->size_x * src_img->size_y;
    const uint32_t bytes_per_op = 256 / (sizeof(uint8_t) * 8);
    const uint32_t bytes_remainder = (sizeof(pixel_t) * pxl_count) % (256/8);

    simde__m256i src_pxls;

    simde__m256i max_val = simde_mm256_set1_epi8(src_img->max_val);
    
    for (uint32_t i = 0; i < pxl_count * sizeof(pixel_t); i+= bytes_per_op) {
        src_pxls = simde_mm256_loadu_si256(src_byte + i);
        simde_mm256_storeu_si256(
            dest_byte + i,
            simde_mm256_subs_epu8(max_val, src_pxls)
        );
    }

    // set src and dest to the end 256 bits of pixel array
    src_byte = (uint8_t*)src_img->pixels + pxl_count;  // do not dereference here
    src_byte -= bytes_remainder;

    dest_byte = (uint8_t*)dest_img->pixels + pxl_count;
    dest_byte -= bytes_remainder;

    src_pxls = simde_mm256_loadu_si256(src_byte);
    simde_mm256_storeu_si256(
        dest_byte,
        simde_mm256_subs_epu8(max_val, src_pxls)
    );
}


void brighten_SIMD(img_t *dest_img, img_t *src_img) {
    *dest_img = *src_img;
}


const char* get_basename(char *filename) {
    // assumes null-terminated string
    // mutates incoming string (kinda)
    uint32_t len_str = strlen(filename);
    uint32_t last_slash_offset = len_str + 1;
    // use +1 to flag it has not been set

    for (int i = len_str - 1; i >= 0; i--) {
        if (filename[i] == '.') {
            filename[i] = '\0';
            break;
        }
    }

    for (int i = len_str - 1; i > 0; i--) {
        if (filename[i] == '/' || filename[i] == '\\') {
            last_slash_offset = i;
            break;
        }
    }

    if (last_slash_offset > len_str) return filename;

    return filename + last_slash_offset + 1;
}
