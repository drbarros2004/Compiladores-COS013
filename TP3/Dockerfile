FROM ubuntu:20.04

# Evita que a instalação peça input interativo
ENV DEBIAN_FRONTEND=noninteractive

# Instala as ferramentas de compilação essenciais (g++, make, flex, e AGORA bison)
RUN apt-get update && apt-get install -y \
    build-essential \
    flex \
    bison \
    && rm -rf /var/lib/apt/lists/*

# Define o diretório de trabalho dentro do contêiner
WORKDIR /app

# ==============================================================================
# CHEATSHEET DE COMANDOS DOCKER PARA ESTE PROJETO
# Ambiente de Compilação C++ (g++, flex, bison) para a disciplina.
# ==============================================================================
#
# --- PASSO 1: Construir a Imagem Docker ---
# Você só precisa fazer isso uma vez ou quando modificar este Dockerfile.
# Execute este comando no terminal, na mesma pasta onde este arquivo está (TP3).
#
# docker build -t meu-ambiente-cpp .
#
#
# --- PASSO 2: Entrar no Contêiner para Trabalhar ---
# Use este comando para criar um contêiner, entrar nele e "espelhar"
# a pasta atual (TP3) para o diretório /app dentro do contêiner.
#
# docker run --rm -it -v "$(pwd)":/app meu-ambiente-cpp /bin/bash
#
# Detalhes das flags:
#   --rm   -> Remove o contêiner automaticamente ao sair com "exit". (Recomendado)
#   -it    -> Permite que você tenha um terminal interativo.
#   -v     -> "Volume": espelha uma pasta do Mac para dentro do contêiner.
#             "$(pwd)":/app significa: pasta_atual -> /app
#
#
# --- Comandos Úteis para Limpeza (Fora do Contêiner) ---
#
# Para ver todas as imagens Docker que você tem no seu sistema:
# docker images
#
# Para apagar imagens "órfãs" que não estão mais sendo usadas e liberar espaço:
# docker image prune
#
# Para ver todos os contêineres (até os parados, se você não usar --rm):
# docker ps -a
#
# ==============================================================================

