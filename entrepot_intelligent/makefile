# Dossiers
CC = gcc
CLIENT_DIR = client
SERVER_DIR = serveur
INCLUDE_DIR = include

# Fichiers sources
CLIENT_SRC = $(CLIENT_DIR)/bras_robotique.c
SERVER_SRC = $(SERVER_DIR)/gestionnaire_outils.c

# Binaires
CLIENT_BIN = $(CLIENT_DIR)/cli
SERVER_BIN = $(SERVER_DIR)/serv

# Règle par défaut
all: $(CLIENT_BIN) $(SERVER_BIN)

# Compilation client
$(CLIENT_BIN): $(CLIENT_SRC) $(INCLUDE_DIR)/commun.h
	$(CC)  -o $@ $<

# Compilation serveur
$(SERVER_BIN): $(SERVER_SRC) $(INCLUDE_DIR)/commun.h
	$(CC)  -o $@ $<

# Nettoyage
clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN)

# Phony targets
.PHONY: all clean
