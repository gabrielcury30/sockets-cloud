#!/bin/bash

HOST_IPV4="127.0.0.1"
HOST_IPV6="::1"
PORT="8080"
RESULT_FILE="benchmark.csv"
NUM_REPETICOES=10    
QTD_ARQUIVOS=50      

echo "Protocolo,Repeticao_ID,Tamanho_Nominal(Bytes),Qtd_Mensagens,Tempo_Total(s),Throughput(MB/s)" > $RESULT_FILE

run_test() {
    local protocol=$1
    local host=$2
    
    echo "=== Iniciando testes para $protocol ==="

    for i in {0..8}
    do
        size=$((2**i))
        
        if [ $size -gt 255 ]; then
            size=255
        fi

        echo "  -> Testando tamanho: $size bytes..."

        for (( run=1; run<=NUM_REPETICOES; run++ ))
        do
            test_dir="temp_bench_${size}"
            rm -rf $test_dir
            mkdir -p $test_dir
            
            for (( f=1; f<=QTD_ARQUIVOS; f++ ))
            do
                suffix="${f}"
                len_suffix=${#suffix}

                len_padding=$(( size - len_suffix ))

                if [ $len_padding -le 0 ]; then
                    filename="${suffix}"
                else
                    padding=$(python3 -c "print('x' * $len_padding)")
                    filename="${padding}${suffix}"
                fi

                touch "${test_dir}/${filename}"
            done

            output=$(./client $host $PORT $test_dir)
            
            tempo=$(echo "$output" | grep "Tempo total" | awk '{print $3}')
            
            tput=$(echo "$output" | grep "MB/s" | awk '{ print $(NF-1) }')

            if [ -z "$tempo" ]; then tempo="0"; fi
            if [ -z "$tput" ]; then tput="0"; fi

            echo "$protocol,$run,$size,$QTD_ARQUIVOS,$tempo,$tput" >> $RESULT_FILE
            
            rm -rf $test_dir
            
            sleep 0.1
        done
    done
}

run_test "IPv4" $HOST_IPV4
run_test "IPv6" $HOST_IPV6

echo ""
echo "Finalizado. Verifique o arquivo '$RESULT_FILE'."