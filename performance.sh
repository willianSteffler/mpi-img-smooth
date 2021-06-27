#/bin/bash
sintaxe="$0 <caminho para imagem | diretorio> <saida> <programa omp> <programa mpi> <max mascara> <max proc>"

if [ "$#" -ne 6 ]; then
    echo "$sintaxe"
    exit 1
fi

saida=$2
omp=$3
mpi=$4
max_m=$5
max_p=$6

if [ ! -f "$saida" ]; then
    #escreve cabeçalho
    echo "método;arquivo;tamanho da imagem;parelização;mascara;tempo;speedup;eficiencia" >> $saida
fi

if [[ -d $1 ]]; then
    imagens=( $(find $1 -type f -regex ".*\.\(bmp\)") )
elif [[ -f $1 ]]; then
    imagens=( $1 )
else
    echo "por favor forneça os parametros corretos ! $sintaxe"
    exit 1
fi

for imagem in ${imagens[*]}; do
  if [ ! -f "$imagem" ]; then
    #escreve cabeçalho
    echo "o arquivo $imagem nao existe"
    continue
  fi

  filename=$(basename -- "$imagem")
  filesz=$(stat -c%s "$imagem")
  TIMEFORMAT='%3R'

  for j in $(seq 3 2 $max_m); do
    #excecuta até max_p
    for i in $(seq 1 $max_p); do
      echo "executando omp para $i threads com mascara $j na imagem $filename ..."
      tempo=$( { time $omp $i $j $imagem saida.bmp; } 2>&1 )
      tempo=${tempo/,/.}
      
      #calculos
      if [[ $i -eq 1 ]]; then tseq_omp=$tempo; fi
      speedup=$(echo "scale=4 ; $tseq_omp / $tempo" | bc)
      eficiencia=$(echo "scale=4 ; $speedup / $i" | bc)
      eficiencia=$(echo $eficiencia | sed 's/^\./0./')
      speedup=$(echo $speedup | sed 's/^\./0./')

      #escreve no arquivo
      linha="omp;$filename;$filesz;$i;$j;$tempo;$speedup;$eficiencia"
      linha=${linha//./,}
      echo "$linha" >> $saida

      echo "executando mpi para $i processos com mascara $j na imagem $filename ..."
      tempo=$( { time mpirun -np $i $mpi $j $imagem saida.bmp; } 2>&1 )
      tempo=${tempo/,/.}
      
      #calculos
      if [[ $i -eq 1 ]]; then tseq_mpi=$tempo; fi
      speedup=$(echo "scale=4 ; $tseq_mpi / $tempo" | bc)
      eficiencia=$(echo "scale=4 ; $speedup / $i" | bc )
      eficiencia=$(echo $eficiencia | sed 's/^\./0./')
      speedup=$(echo $speedup | sed 's/^\./0./')

      linha="mpi;$filename;$filesz;$i;$j;$tempo;$speedup;$eficiencia"
      linha=${linha//./,}
      echo "$linha" >> $saida

    done
  done
done

echo "pronto !"