#!/bin/bash

# Caminho para a pasta de arquivos de áudio
audio_folder="audioprocessor_files"

# Verifica se a pasta existe
if [ ! -d "$audio_folder" ]; then
    echo "A pasta $audio_folder não existe."
    exit 1
fi

# Captura o tempo de início
start_time=$(date +%s%3N)

# Itera sobre todos os arquivos .wav na pasta
for audio_file in "$audio_folder"/*.wav; do
    if [ -f "$audio_file" ]; then
        echo "Processando $audio_file..."
        ./audioprocessor.out "$audio_file"
    else
        echo "No .wav file in $audio_folder."
    fi
done

# Captura o tempo de término
end_time=$(date +%s%3N)

# Calcula o tempo total de execução em milissegundos
execution_time_ms=$((end_time - start_time))

# Converte o tempo total de execução para segundos
execution_time_s=$(echo "scale=3; $execution_time_ms / 1000" | bc)

echo "Total execution time: $execution_time_s seconds."