#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>

// #define DEBUG_FILE

#pragma pack(1)

typedef struct cabecalho{
	unsigned short tipo;
	unsigned int tamanho_arquivo;
	unsigned short reservado1;
	unsigned short reservado2;
	unsigned int offset;
	unsigned int tamanho_cabecalho;
	unsigned int largura;
	unsigned int altura;
	unsigned short planos;
	unsigned short bits;
	unsigned int compressao;
	unsigned int tamanho_imagem;
	unsigned int resolucaox;
	unsigned int resolucaoy;
	unsigned int cores_usadas;
	unsigned int cores_imp;
}CABECALHO;

typedef struct rgb{
	unsigned char b;
	unsigned char g;
	unsigned char r;
}RGB;

/*-----------------------------------------*/
// Declaração de funções
void apply_median_filter(RGB** image,int mask,int h, int w,int i, int j);
int writeFile(CABECALHO c,RGB** imagem_s,int ali,char* fileName);

/*-----------------------------------------*/
int main(int argc, char **argv){
	FILE *fin;
	CABECALHO c;
	RGB* imagem_s;
	RGB** imagem;
	int i, j, id, nth, mask,slice_sz,rem,offset,end;
	int debug = 1;

	if ( argc != 5){
		printf("%s <threads> <mascara> <entrada> <saida>\n", argv[0]);
		exit(0);
	}

	nth = atoi(argv[1]);
	if ( nth <= 0 ){
		printf("Numero de threads não inválido ! %d\n", nth);
		exit(0);
	}
							
	fin = fopen(argv[3], "rb");
	
	if ( fin == NULL ){
		printf("Erro ao abrir o arquivo %s\n", argv[3]);
		exit(0);
	}

	mask = atoi(argv[2]);

	if( mask % 2 == 0){
		printf("Mascara deve ser ímpar ! \n");
		exit(0);
	}


	fread(&c, sizeof(CABECALHO), 1, fin);
	imagem = (RGB **)malloc(c.altura * sizeof(RGB*));
	imagem_s = (RGB *)malloc(c.altura * c.largura * sizeof(RGB));

	int ali = (c.largura * sizeof(RGB)) % 4;
	if(ali != 0){
		ali = 4 - ali;
	}
	
	for(i=0; i<c.altura; i++){
		imagem[i] = &imagem_s[i*c.largura];
		// ler imagem
		for (j = 0; j < c.largura; j++){
			//le de forma serializada
			fread(&imagem[i][j], sizeof(RGB),1, fin);
		}

		// ler bytes do alinhamento
		for (j = 0; j < ali; j++){
			unsigned char b;
			fread(&b, sizeof(unsigned char),1, fin);
		}
	}

	omp_set_num_threads(nth);

	//tamanho do pedaço (altura)
	slice_sz = c.altura / nth;
	//sobra da divisão dos processos
	rem = c.altura % nth;

	
	#pragma omp parallel private (id,offset,end,i,j)
	{
		id = omp_get_thread_num();
		offset = id * slice_sz;
		end = offset + slice_sz;

		// aplica filtro no pedaço da imagem 
		for (i = offset; i < end; i++)
		{
			for (j = 0; j < c.largura; j++)
			{
				apply_median_filter(imagem,mask,c.altura,c.largura,i,j);
			}
		}

		#ifdef DEBUG_FILE
		char debug_name[20];
		sprintf(debug_name, "debug_%d.bmp", id);
		if(!writeFile(c,imagem,ali,debug_name)){
			printf("Erro salvar imagem no arquivo %s\n", debug_name);
		}
		#endif
	}

	if(rem > 0){
		for(i = slice_sz * nth;i< c.altura;i++){
			for (j = 0; j < c.largura; j++)
			{
				apply_median_filter(imagem,mask,c.altura,c.largura,i,j);
			}
		}
	}

	if(!writeFile(c,imagem,ali,argv[4])){
		printf("Erro salvar imagem no arquivo %s\n", argv[4]);
	}

	free(imagem_s);
	fclose(fin);
	return 0;
}

int writeFile(CABECALHO c,RGB** imagem,int ali,char* fileName){
	int i,j;
	FILE *fout;
	fout = fopen(fileName, "wb");

	if ( fout == NULL ){
		return 0;
	}

	fwrite(&c, sizeof(CABECALHO), 1, fout);
	for (i = 0; i < c.altura; i++)
	{
		for ( j = 0; j < c.largura; j++)
		{
			fwrite(&imagem[i][j], sizeof(RGB) , 1, fout);
		}

		for ( j = 0; j < ali; j++)
		{
			unsigned char b;
			fwrite(&b, sizeof(unsigned char) , 1, fout);
		}
		
	}
	fclose(fout);
	return 1;
}

// insertion sort
void sort(unsigned char* array, int length){
    int i = 1;
    int j;
    int x;

    for (i=1; i<length; i++){
        x = array[i];
        for(j = i-1; j >= 0 && array[j]>x; j--){
            array[j+1] = array[j];
        }
        array[j+1] = x;
    }
}

//calcula o pixel mediano
RGB median(RGB* pixels,int length){

    RGB result;

    unsigned char* pixels_r = (unsigned char *)malloc(length);
    unsigned char* pixels_g = (unsigned char *)malloc(length);
    unsigned char* pixels_b = (unsigned char *)malloc(length);

    for (int i = 0; i < length; i++)
    {
        pixels_r[i] = pixels[i].r;
        pixels_g[i] = pixels[i].g;
        pixels_b[i] = pixels[i].b;
    }
    
    sort(pixels_r,length);
    sort(pixels_g,length);
    sort(pixels_b,length);

    result.r = pixels_r[length/2];
    result.g = pixels_g[length/2];
    result.b = pixels_b[length/2];

	free(pixels_r);
	free(pixels_g);
	free(pixels_b);

    return result;
}

// aplica filtro de mediana
void apply_median_filter(RGB** image,int mask,int h, int w,int i, int j){
    // raio do centro
    int r = mask - (mask / 2) - 1;
    int l;
    int k;
    int sz = 0;

    RGB* pixels = (RGB *)malloc(sizeof(RGB) * mask * mask);

    for (l = i-r < 0 ? 0 : i-r; l <= i+r && l < h; l++)
    {
        for (k = j-r < 0 ? 0 : j-r; k <= j+r && k < w ; k++)
        {
            pixels[sz++] = image[l][k];
        }
    }

    image[i][j] = median(pixels,sz);
	free(pixels);
}