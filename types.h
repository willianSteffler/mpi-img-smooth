#ifndef TYPES_H
# define TYPES_H

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

#endif