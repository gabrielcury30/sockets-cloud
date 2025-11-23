# sockets-cloud

Projeto para comunicação cliente-servidor usando sockets TCP com suporte a IPv4 e IPv6.

**Resumo**

- **Objetivo:** demonstrar a construção de um cliente e servidor TCP simples em C que trocam mensagens e transferem listas de nomes de arquivos; também inclui um benchmark para comparar throughput entre IPv4 e IPv6.
- **Linguagem:** C
- **Autor:** Gabriel Cury
- **Licença:** MIT (veja o arquivo `LICENSE`)

**Conteúdo do repositório**

- `server.c` : servidor TCP que aceita conexões IPv6/IPv4, negocia um protocolo simples (`READY` / `READY ACK`), recebe o nome de um diretório e uma lista de nomes de arquivos, salva a lista em um arquivo de saída e encerra quando recebe `bye`.
- `client.c` : cliente TCP que conecta ao servidor, envia `READY`, envia o nome do diretório e a lista de arquivos encontrados no diretório (tratando byte-stuffing para o nome `bye`), mede tempo e throughput e imprime um relatório.
- `benchmark.sh` : script que cria diretórios temporários com 50 arquivos e executa o `client` repetidas vezes para gerar `benchmark.csv`.
- `benchmark.csv` : resultados gerados pelo `benchmark.sh` (amostras de throughput por tamanho de mensagem e protocolo).
- `Makefile` : regras para compilar `server` e `client`.

**Pré-requisitos**

- Sistema: Linux (testado em distribuições GNU/Linux). Pode rodar em macOS, mas algumas chamadas de rede/IPv6 podem variar.
- Ferramentas: `gcc`, `make`, `bash`, `python3` (usado pelo `benchmark.sh` para gerar padding).

**Compilar**

1. Abra um terminal no diretório do projeto.
2. Rode:

```bash
make
```

Isso gera os binários `server` e `client`.

**Executando o servidor**

- Por padrão o servidor escuta na porta `8080` (definida em `server.c`). Para iniciar:

```bash
./server
```

O servidor imprime mensagens ao aceitar conexões e cria um arquivo de saída com o nome `<hostname>_<dirnome>.txt` contendo a lista de arquivos recebida.

**Executando o cliente**
Sintaxe:

```bash
./client <host> <porta> <nome_do_diretorio>
```

Exemplo local usando IPv4:

```bash
./client 127.0.0.1 8080 ./temp_bench_16
```

O cliente envia `READY`, espera `READY ACK`, envia o nome do diretório e os nomes dos arquivos, envia `bye` e imprime um relatório com tempo total e throughput (MB/s).

**Rodando o benchmark**
O script `benchmark.sh` automatiza a criação de diretórios temporários com 50 arquivos e executa o `client` para vários tamanhos de nome (1,2,4,...,255) repetindo cada teste 10 vezes. Antes de rodar, certifique-se de que o `server` esteja em execução e que os binários tenham sido compilados.

Para rodar o benchmark (servidor deve estar rodando em outra janela):

```bash
bash benchmark.sh
```

O script gera/atualiza `benchmark.csv` com cabeçalho:

```
Protocolo,Repeticao_ID,Tamanho_Nominal(Bytes),Qtd_Mensagens,Tempo_Total(s),Throughput(MB/s)
```

Significado das colunas:

- `Protocolo`: `IPv4` ou `IPv6` (conforme testado pelo script).
- `Repeticao_ID`: número da repetição (1..N).
- `Tamanho_Nominal(Bytes)`: tamanho nominal do nome do arquivo testado (limitado a 255 no script).
- `Qtd_Mensagens`: quantidade de arquivos enviados por teste (configurado em `benchmark.sh`, default 50).
- `Tempo_Total(s)`: tempo total medido pelo cliente em segundos.
- `Throughput(MB/s)`: taxa de transferência medida em MB/s.

Observação: `benchmark.sh` chama `./client` e extrai do seu output o tempo e o throughput. Se quiser ajustar parâmetros (nº de repetições ou qtd de arquivos), edite as variáveis `NUM_REPETICOES` e `QTD_ARQUIVOS` no script.

**Dicas e resolução de problemas comuns**

- Se `getaddrinfo` falhar no cliente, verifique se o host e a porta estão corretos.
- Se a conexão for recusada no `connect`, confirme que o `server` está rodando e escutando na porta certa.
- `benchmark.sh` usa `python3` para gerar padding; instale `python3` se necessário.
- No servidor, o `AI_PASSIVE` com `AF_INET6` permite aceitar conexões IPv6 e, dependendo do SO, também IPv4 mapeado. Se quiser forçar somente IPv6 ou IPv4, ajuste `server.c`.

**Exemplo rápido (fluxo mínimo)**

1. Em um terminal, compile e inicie o servidor:

```bash
make
./server
```

2. Em outro terminal, crie um diretório de teste com alguns arquivos e rode o cliente:

```bash
mkdir -p ./temp_test
touch ./temp_test/file1 ./temp_test/file2
./client 127.0.0.1 8080 ./temp_test
```

3. Verifique que o servidor criou um arquivo `$(hostname)_temp_test.txt` com os nomes `file1` e `file2`.

Licença: veja `LICENSE` (MIT).
