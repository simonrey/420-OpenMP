#include "lodepng.h"

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

#define BYTES_PER_PIXEL 4

struct PNG_INFO * decodeOneStep(const char * filename);

struct PNG_INFO{
	unsigned char * PNG_START;
	unsigned PNG_SIZE,PNG_HEIGHT,PNG_WIDTH;
	unsigned PNG_SIZE_POOL,PNG_HEIGHT_POOL,PNG_WIDTH_POOL;
};

struct ARRAY_HOLDER{
	float ARRAY[4];
	int MAX;
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
//  	int NO_THREADS = strtol(argv[3],NULL,10);
//  	char * OUTPUT_FILENAME = argv[2];
//  	char * INPUT_FILENAME = argv[1];
	struct PNG_INFO * IMAGE;

	int NO_THREADS = 4;
		char  OUTPUT_FILENAME[] = "new_test.png";
		char  INPUT_FILENAME[] = "test.png";

	if((IMAGE=decodeOneStep(INPUT_FILENAME))==NULL) return(0);

	printf("This is the image's size: %i\n",IMAGE->PNG_SIZE);
	printf("This is W: %i\nThis is H: %i\n",IMAGE->PNG_WIDTH, IMAGE->PNG_HEIGHT);
	printf("No threads: %i\n",NO_THREADS);

	unsigned char * NEW_IMAGE = malloc(IMAGE->PNG_SIZE_POOL);

	clock_t tic = clock();

#pragma omp parallel num_threads(NO_THREADS)
	{
		int tid = omp_get_thread_num();
		int chunk_size = IMAGE->PNG_SIZE / omp_get_num_threads();
		int chunk_size_pool = IMAGE->PNG_SIZE_POOL / omp_get_num_threads();
		int start_idx = tid * chunk_size;
		int start_idx_pool = tid*chunk_size_pool;
		int end_idx = (tid == omp_get_num_threads()-1) ? IMAGE->PNG_SIZE : start_idx + chunk_size;
		int end_idx_pool = (tid == omp_get_num_threads()-1) ? IMAGE->PNG_SIZE_POOL : start_idx_pool + chunk_size_pool;

		printf("Thread %i working on chunk from %i to %i\n", tid, start_idx, end_idx);

		struct ARRAY_HOLDER ARR;
		int i;

		while(start_idx_pool<end_idx_pool){
			i=0;

			ARR.ARRAY[0]=IMAGE->PNG_START[start_idx];
			ARR.ARRAY[1]=IMAGE->PNG_START[start_idx+4];
			ARR.ARRAY[2]=IMAGE->PNG_START[start_idx+(IMAGE->PNG_WIDTH*BYTES_PER_PIXEL*sizeof(char))];
			ARR.ARRAY[3]=IMAGE->PNG_START[start_idx+4+(IMAGE->PNG_WIDTH*BYTES_PER_PIXEL*sizeof(char))];
			ARR.MAX=ARR.ARRAY[0];
			while(i<4){
				if(ARR.ARRAY[i]>ARR.MAX) ARR.MAX=ARR.ARRAY[i];
				i++;
			}

			NEW_IMAGE[start_idx_pool] = (unsigned char) ARR.MAX;

			start_idx++;
			start_idx_pool++;

			if(start_idx_pool % 4 == 0){
				start_idx+=4;
				if(start_idx % (IMAGE->PNG_WIDTH*BYTES_PER_PIXEL*sizeof(char)) == 0){
					start_idx+=(IMAGE->PNG_WIDTH*BYTES_PER_PIXEL*sizeof(char));
				}
//				if(start_idx>end_idx){
//					break;
//				}
			}
		}
	}

	clock_t toc = clock();
	printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);

	lodepng_encode32_file(OUTPUT_FILENAME, NEW_IMAGE, IMAGE->PNG_WIDTH_POOL, IMAGE->PNG_HEIGHT_POOL);

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
		PNG_DECODE->PNG_SIZE=PNG_DECODE->PNG_WIDTH*PNG_DECODE->PNG_HEIGHT*BYTES_PER_PIXEL*sizeof(char);
		PNG_DECODE->PNG_HEIGHT_POOL=PNG_DECODE->PNG_HEIGHT/2;
		PNG_DECODE->PNG_WIDTH_POOL=PNG_DECODE->PNG_WIDTH/2;
		PNG_DECODE->PNG_SIZE_POOL=PNG_DECODE->PNG_HEIGHT_POOL*PNG_DECODE->PNG_WIDTH_POOL*BYTES_PER_PIXEL*sizeof(char);
		return(PNG_DECODE);
	}
}
