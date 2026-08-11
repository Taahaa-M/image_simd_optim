#include <stdio.h>
#include <stdint.h>
#include <string.h>


#define SUCCESS 0
#define FAIL    -1

#define IMG_NAME     1
#define MAX_FILENAME_LEN 64

#define P_SIMD 2**0  // SIMD optimisation flag


typedef struct {
    uint32_t size_x;
    uint32_t size_y;
    uint8_t* red;
    uint8_t* green;
    uint8_t* blue;
} img_t;


int get_file_info(FILE* img_file, uint32_t* size_x, uint32_t* size_y);

int load_parse_file(img_t* img, FILE* img_file, uint32_t size_x, uint32_t size_y);

int process_image(img_t* img, const char* filename, uint32_t FLAGS);

void greyscale(img_t* dest_img, img_t* src_img);

void invert(img_t* dest_img, img_t* src_img);

void brightness(img_t* dest_img, img_t* src_img, uint32_t multiplier);


int main(int argc, char** argv) {
    char img_filename[MAX_FILENAME_LEN];
    FILE* img_file;

    uint32_t size_x;
    uint32_t size_y;

    img_t* img;

    if (argc < 2) {
        fprintf(stderr, "Provide image file name as below:\n./img_process file_name.ppm\n\n");
        return FAIL;
    }

    strncpy(img_filename, argv[IMG_NAME], MAX_FILENAME_LEN);

    img_file = fopen(img_filename, "r");

    if (img_file == NULL) {
        fprintf(stderr, "Could not open file. Program exited.\n");
        return FAIL;
    }

    if (load_parse_file(img, img_file) == FAIL) {
        fprintf(stderr, "Could not parse file. Program exited.\n");
        return FAIL;
    }

    return SUCCESS;
}


int load_parse_file(img_t* img, FILE* img_file) {
    return SUCCESS;
}

int get_file_info(FILE* img_file, uint32_t* size_x, uint32_t* size_y, uint32_t* max_val) {
    return SUCCESS;
}


int process_image(img_t* img, const char* filename, uint32_t FLAGS) {
    img_t* new_img;

    return SUCCESS;
}

void greyscale(img_t* dest_img, img_t* src_img) {
}

void invert(img_t* dest_img, img_t* src_img) {
}

void brightness(img_t* dest_img, img_t* src_img, uint32_t multiplier) {
}
