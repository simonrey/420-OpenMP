#include "lodepng.h"
#include "wm.h"

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

#define BYTES_PER_PIXEL 4

struct PNG_INFO * decodeOneStep(const char * filename);

struct PNG_INFO{
	unsigned char * PNG_START;
	unsigned PNG_SIZE,PNG_HEIGHT,PNG_WIDTH;
	unsigned PNG_SIZE_CONVOLVED,PNG_HEIGHT_CONVOLVED,PNG_WIDTH_CONVOLVED;
};

struct ARRAY_HOLDER{
	float ARRAY[9];
	int RESULT;
};

int main(int argc, char *argv[]){

//	if(argc<4){
//		printf("Too few arguments provided\n");
//		return(0);
//	}
//	if (argc==4){
//		printf("These are your arguments: %s, %s, %s\n",argv[1],argv[2],argv[3]);
//	}
//	else if (argc>4){
//		printf("Too many arguments provided\n");
//		return(0);
//	}
//
//
//
//  	int NO_THREADS = strtol(argv[3],NULL,10);
//  	char * OUTPUT_FILENAME = argv[2];
//  	char * INPUT_FILENAME = argv[1];
	int NO_THREADS = 1;
	char  OUTPUT_FILENAME[] = "new_test.png";
	char  INPUT_FILENAME[] = "test.png";
	struct PNG_INFO * IMAGE;

	if((IMAGE=decodeOneStep(INPUT_FILENAME))==NULL) return(0);

	printf("This is the image's size: %i\n",IMAGE->PNG_SIZE);
	printf("This is W: %i\nThis is H: %i\n",IMAGE->PNG_WIDTH, IMAGE->PNG_HEIGHT);
	printf("No threads: %i\n",NO_THREADS);

	unsigned char * NEW_IMAGE = malloc(IMAGE->PNG_SIZE_CONVOLVED);

	int j=0,k=0,l=0;
	float WEIGHT[9];

	for(j=0; j<3; j++){
		for(k=0;k<3;k++){
			WEIGHT[l] = w[j][k];
			l++;
		}
	}


#pragma omp parallel num_threads(NO_THREADS)
	{
		int tid = omp_get_thread_num();
		int chunk_size = IMAGE->PNG_SIZE / omp_get_num_threads();
		int chunk_size_convolved = IMAGE->PNG_SIZE_CONVOLVED /omp_get_num_threads();
		int start_idx = tid * chunk_size;
		int start_idx_convolved= tid* chunk_size_convolved;
		int end_idx = (tid == omp_get_num_threads()-1) ? IMAGE->PNG_SIZE : start_idx + chunk_size;
		int end_idx_convolved = (tid==omp_get_num_threads()-1) ? IMAGE->PNG_SIZE_CONVOLVED : start_idx + chunk_size_convolved;

		struct ARRAY_HOLDER ARR;
		ARR.RESULT = 0;

		printf("Thread %i working on chunk from %i to %i\n", tid, start_idx, end_idx);

		while(start_idx_convolved < end_idx_convolved){

			int i = 0;
			ARR.RESULT=0;

			while(i < 3){
				ARR.ARRAY[i] = IMAGE->PNG_START[4*i + start_idx];
				ARR.ARRAY[i+3] = IMAGE->PNG_START[4*i + start_idx + (BYTES_PER_PIXEL * (IMAGE->PNG_WIDTH)*sizeof(char))];
				ARR.ARRAY[i+6] = IMAGE->PNG_START[4*i + start_idx + (2 * (BYTES_PER_PIXEL * (IMAGE->PNG_WIDTH) * sizeof(char)))];
				i++;
			}

			i = 0;

			while(i < 9){
				ARR.RESULT += (WEIGHT[i] * ARR.ARRAY[i]);
				i++;
			}

			if(ARR.RESULT < 0){
				ARR.RESULT = 0;
			}
			else if(ARR.RESULT > 255){
				ARR.RESULT = 255;
			}

			NEW_IMAGE[start_idx_convolved] = (unsigned char) ARR.RESULT;

			start_idx++;
			start_idx_convolved++;

			if(start_idx+8+(2*IMAGE->PNG_WIDTH*BYTES_PER_PIXEL*sizeof(char))==IMAGE->PNG_SIZE){
				break;
			}

			if((start_idx + 8) % (IMAGE->PNG_WIDTH * BYTES_PER_PIXEL * sizeof(char))==0){
				start_idx+=IMAGE->PNG_WIDTH*BYTES_PER_PIXEL*sizeof(char);
			}
		}
	}

	lodepng_encode32_file(OUTPUT_FILENAME, NEW_IMAGE, IMAGE->PNG_WIDTH_CONVOLVED, IMAGE->PNG_HEIGHT_CONVOLVED);

	free(IMAGE->PNG_START);
	free(IMAGE);
	free(NEW_IMAGE);

	return(1);
}

struct PNG_INFO * decodeOneStep(const char* filename)
{
	struct PNG_INFO * PNG_DECODE = malloc(sizeof(struct PNG_INFO));
	unsigned ERROR;

	ERROR = lodepng_decode32_file(&PNG_DECODE->PNG_START,&PNG_DECODE->PNG_WIDTH,&PNG_DECODE->PNG_HEIGHT,filename);
	if(ERROR){
		printf("error %u: %s\n", ERROR, lodepng_error_text(ERROR));
		return(NULL);
	}
	else{
		PNG_DECODE->PNG_SIZE = PNG_DECODE->PNG_WIDTH * PNG_DECODE->PNG_HEIGHT * BYTES_PER_PIXEL * sizeof(char);
		PNG_DECODE->PNG_HEIGHT_CONVOLVED=PNG_DECODE->PNG_HEIGHT-2;
		PNG_DECODE->PNG_WIDTH_CONVOLVED=PNG_DECODE->PNG_WIDTH-2;
		PNG_DECODE->PNG_SIZE_CONVOLVED = ((PNG_DECODE->PNG_HEIGHT * PNG_DECODE->PNG_WIDTH) - (2 * PNG_DECODE->PNG_HEIGHT) - (2 * PNG_DECODE->PNG_WIDTH) + 4) * BYTES_PER_PIXEL * sizeof(char);
		return(PNG_DECODE);
	}
}
