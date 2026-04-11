# Variabili del compilatore
CC = gcc
# -Isrc permette al compilatore di trovare gli header usando i path relativi alla cartella src
CFLAGS = -Wall -Wextra -g -Isrc

# Nome dell'eseguibile finale
TARGET = shellby

# Lista dei file sorgente (percorsi relativi alla root)
SRCS = src/main.c \
       src/virtual_fat/fat.c \
       src/shell/my_shell.c

# Genera automaticamente la lista dei file oggetto (.o) dai sorgenti (.c)
OBJS = $(SRCS:.c=.o)

# Regola di default: compila l'eseguibile
all: $(TARGET)

# Fase di linking: unisce i file oggetto per creare l'eseguibile
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Fase di compilazione: trasforma ogni .c in un .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pulisce i file compilati (esegui 'make clean' per ricominciare da zero)
clean:
	rm -f $(OBJS) $(TARGET)

# Dichiariamo all e clean come target fittizi per sicurezza
.PHONY: all clean