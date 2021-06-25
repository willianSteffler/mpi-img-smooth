#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>

// #define DEBUG
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
	RGB* slice_s;
	RGB** imagem;
	RGB** slice;
	int i, j, id, nproc, mask,slice_sz,rem,offset,end;
	int debug = 1;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &nproc);

	#ifdef DEBUG
	
	while(debug == 1){
		sleep(2);
	}

	#endif

	if ( argc != 4){
		printf("%s <mascara> <entrada> <saida>\n", argv[0]);
		exit(0);
	}
							
	fin = fopen(argv[2], "rb");
	
	if ( fin == NULL ){
		printf("Erro ao abrir o arquivo %s\n", argv[2]);
		exit(0);
	}

	mask = atoi(argv[1]);

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
		if(id == 0){
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
	}

	//distribui imagem para os processos
	MPI_Bcast(imagem_s,c.altura*c.largura*sizeof(RGB),MPI_BYTE,0,MPI_COMM_WORLD);

	slice_sz = c.altura / nproc;

	//sobra da divisão dos processos
	rem = c.altura % nproc;

	offset = id * slice_sz;
	end = offset + slice_sz;

	slice = (RGB **)malloc(slice_sz * sizeof(RGB*));
	slice_s = (RGB*)malloc(sizeof(RGB) * slice_sz * c.largura);

	// aplica filtro no pedaço da imagem 
	for (i = offset; i < end; i++)
	{
		slice[(i-offset)] = &slice_s[(i-offset)*c.largura];
		for (j = 0; j < c.largura; j++)
		{
			apply_median_filter(imagem,mask,c.altura,c.largura,i,j);
			// salva pedaço filtrado (não é permitido utilizar a mesma variavel imagem)
			slice[i - offset][j] = imagem[i][j];
		}
	}

	#ifdef DEBUG_FILE
	char debug_name[20];
	sprintf(debug_name, "debug_%d.bmp", id);
	if(!writeFile(c,imagem,ali,debug_name)){
		printf("Erro salvar imagem no arquivo %s\n", debug_name);
	}
	#endif

	//recebe pedaços dos processos
	MPI_Gather(slice_s,sizeof(RGB) * slice_sz * c.largura,MPI_BYTE,imagem_s,sizeof(RGB) * slice_sz * c.largura,MPI_BYTE,0,MPI_COMM_WORLD);

	if(id == 0){
		if(rem > 0){
			for(i = slice_sz * nproc;i< c.altura;i++){
				for (j = 0; j < c.largura; j++)
				{
					apply_median_filter(imagem,mask,c.altura,c.largura,i,j);
				}
			}
		}

		if(!writeFile(c,imagem,ali,argv[3])){
			printf("Erro salvar imagem no arquivo %s\n", argv[3]);
		}
	}

	free(imagem_s);
	fclose(fin);
	MPI_Finalize();
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