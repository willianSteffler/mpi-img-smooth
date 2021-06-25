#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>


#pragma pack(1)

//-----------------------------------------/
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

//------------------Declaração das funções-----------------------------
RGB calcula_mediana(RGB ** imagem, int mascara, int total_mascara, int altura, int largura, int i, int j);
//------------------------------------------------------------------------
int main(int argc, char **argv){
	FILE *fin, *fout;
	CABECALHO c;
	RGB pixel;
	RGB ** imagem_transito;
	RGB *dados_transito;
	
	int i, j, id, nproc, mascara, total_mascara;
	char entrada[100] = "borboleta.bmp", saida[100] = "convertido.bmp";
	

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &nproc);
	
	if (argc != 2){
		printf("%s <num_threadds> \n", argv[0]);
		exit(0);
	}
	
	mascara = atoi(argv[1]);
	total_mascara = mascara * mascara;

					
	fin = fopen(entrada, "rb");
	
	if ( fin == NULL ){
		printf("Erro ao abrir o arquivo %s\n", entrada);
		exit(0);
	}

	fout = fopen(saida, "wb");

	if ( fout == NULL ){
		printf("Erro ao abrir o arquivo %s\n", saida);
		exit(0);
	}

///////////////////////// Estrutura da imagem original /////////////////////////////
	fread(&c, sizeof(CABECALHO), 1, fin);
	fwrite(&c, sizeof(CABECALHO), 1, fout);

	RGB * imagem = (RGB *)malloc(c.altura * sizeof(RGB *));
	RGB *dados = (RGB *)malloc(c.largura * c.altura  * sizeof(RGB));
	
    for(i=0; i<c.altura; i++){
        imagem[i] = &dados[i * c.largura];
    }  
   
/////////////////// Estrutura de dados da imagem em transito //////////////////////
    int tamanho = c.altura/nproc; 
    
    if (nproc > 1){
		imagem_transito = (RGB **)malloc(tamanho * sizeof(RGB *));
		dados_transito = (RGB *)malloc(c.largura * tamanho  * sizeof(RGB));
		
		for(i=0; i<tamanho; i++){
			imagem_transito[i] = &dados_transito[i * c.largura];
		} 
	}
//////////////////////// Carrega a imagem em memória ///////////////////////////
	if ( id == 0 ){
		for(i=0; i<c.altura; i++){
			for(j=0; j<c.largura; j++){
				fread(&imagem[i][j], sizeof(RGB), 1, fin);
			}
		}
	}
//////////////////////////// Distribuição da imagem /////////////////////////////
	
	MPI_Bcast(dados,c.altura*c.largura*sizeof(RGB),MPI_BYTE,0,MPI_COMM_WORLD);
	
//////////////////////// Processamento da imagem ////////////////////////////////	
	if ( id != 0 ){	
		int indice = 0;
		
		for(i=(id-1)*tamanho; i<id*tamanho ; i++){
			for(j=0; j<c.largura ; j++){
				imagem_transito[indice][j] = calcula_mediana(imagem, mascara, total_mascara, c.altura, c.largura, i, j);
			}
			indice++;
		}
		MPI_Send(dados_transito, tamanho*c.largura*sizeof(RGB), MPI_BYTE, 0, 200, MPI_COMM_WORLD);
	}
	else{
		int p, laux, caux, resto = 0, proc = 0;
		float maux;
		
		MPI_Status s;
		
		if(nproc > 1){
			resto = ((c.altura/nproc) * (nproc-1))+(c.altura%nproc) - 1;
		}
		
		for(i=resto; i<c.altura ; i++){
			for(j=0; j<c.largura ; j++){
				imagem[i][j] = calcula_mediana(imagem, mascara, total_mascara, c.altura, c.largura, i, j);
			}
		}

		for(p=1; p<nproc; p++){
			MPI_Recv(dados_transito, tamanho*c.largura*sizeof(RGB), MPI_BYTE, p, 200, MPI_COMM_WORLD, &s);
			
			for(i=0; i<tamanho ; i++){
				for(j=0; j<c.largura ; j++){
					fwrite(&imagem_transito[i][j], sizeof(RGB), 1, fout);
				}
			}
		}
		
		for(i=resto; i<c.altura ; i++){
			for(j=0; j<c.largura ; j++){
				fwrite(&imagem[i][j], sizeof(RGB), 1, fout);
			}
		}
	}
	
	free(dados_transito);
	free(imagem_transito);
	
	free(dados);
	free(imagem);

	fclose(fin);
	fclose(fout);
	MPI_Finalize();
			
}
//---------------------------Corpo das funções-----------------------------------------------
RGB calcula_mediana(RGB ** imagem, int mascara, int total_mascara, int altura, int largura, int i, int j){
	unsigned char mediana;
	unsigned char medianas_red[total_mascara];
	unsigned char medianas_green[total_mascara];
	unsigned char medianas_blue[total_mascara];			
	unsigned char aux;
	int cont = 0;
	int k, l, m, n;
	int meio;
	
	meio = mascara / 2;
	
	for(k=i-meio; k<=i+meio; k++){
		for(l=j-meio; l<=j+meio; l++){
			if(i > meio && j > meio && i <= altura - mascara && j <= largura - mascara){
				medianas_red[cont] = imagem[k][l].r;
				medianas_green[cont] = imagem[k][l].g;
				medianas_blue[cont] = imagem[k][l].b;
				cont++;
			}
			
		}	
	}
	
	for(m=0; m<total_mascara; m++){
		for(n=0; n<total_mascara; n++){
			if ( medianas_red[n] > medianas_red[n+1]){
				aux = medianas_red[n];
				medianas_red[n] = medianas_red[n+1];
				medianas_red[n+1] = aux;
			}
			if ( medianas_green[n] > medianas_green[n+1]){
				aux = medianas_green[n];
				medianas_green[n] = medianas_green[n+1];
				medianas_green[n+1] = aux;
			}
			if ( medianas_blue[n] > medianas_blue[n+1]){
				aux = medianas_blue[n];
				medianas_blue[n] = medianas_blue[n+1];
				medianas_blue[n+1] = aux;
			}
		}	
	}
	
	RGB pixel;
	
	if(i > meio && j > meio && i <= altura - mascara && j <= largura - meio){
		
		pixel.b = medianas_blue[total_mascara / 2];
		pixel.g = medianas_green[total_mascara / 2];
		pixel.r = medianas_red[total_mascara / 2];
	}
	else{
		pixel  = imagem[i][j];
	}
	
	return pixel;
}