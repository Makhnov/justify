###############################
#####    Répertoires     ######
###############################

# Dossiers du projet
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
DOC_DIR = doc
BCH_DIR = benchmarks
INC_DIR = include
DAT_DIR = data
TEMP 	= $(OBJ_DIR) $(BIN_DIR) $(BCH_DIR)

# Dossiers temporaires
$(TEMP):
	mkdir -p $@

 $(DOC_DIR):
	mkdir -p $@

###############################
##### Outils et options  ######
###############################

CC       = gcc
VG       = valgrind
PERF 	?= $(shell which perf)

CFLAGS 	 = -I$(INC_DIR) -std=c2x -g -O0 -Wall -Wextra -Wpedantic -D_GNU_SOURCE -MMD -MP
LDFLAGS  =
LDLIBS 	 =
VGFLAGS  = --leak-check=full --show-leak-kinds=all --track-origins=yes
CGFLAGS  = --tool=cachegrind --cache-sim=yes --branch-sim=yes
# CGFLAGS += --main-stacksize=256M
BCH_TAG  = $(INFILE_BASE)-M$(M)
RM 		 = rm -rf

###############################
#####   Sources, objets   #####
#####   et dépendances.   #####
###############################

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))
DEP = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.d,$(SRC))
-include $(DEP)

###############################
##### Valeurs par défaut ######
###############################

# Exécutable
TARGET  = $(BIN_DIR)/AODjustify

# Arguments
R    = 5 # nombre de répétitions pour les benchs temporels
M    = 80 # largeur par défaut
ARGS = $(M) $(FILE)
FILE = data/ALaRechercheDuTempsPerdu-1paragraphe-2025-ISO-8859-1

M_LIST ?= 20 40 60 80 100 120 140 160 180 200 # Liste de M pour les tests de performance
P_LIST ?= cache-references,cache-misses,cycles,instructions
# Couleurs
RESET=\033[0m
RED=\033[0;31m
GREEN=\033[0;32m
YELLOW=\033[0;33m
CYAN=\033[0;36m
MAGENTA=\033[0;35m

###############################
#####     Compilation    ######
###############################

$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $(OBJ) $(LDLIBS) -o $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

###############################
##### Cibles principales ######
###############################
.PHONY: all run clean tests valgrind cache perf massif index clean-all clean-cache clean-bench
.DEFAULT_GOAL = all


help:
	@echo ""
	@echo "  $(GREEN)Usage$(RESET)            : $(GREEN)$(TARGET)$(CYAN) M FILE$(RESET)"
	@echo ""
	@echo "  make $(GREEN)index$(RESET)       : Génère la documentation du code source avec Doxygen (dans doc/index.html)"
	@echo ""
	@echo "  make $(GREEN)(all)$(RESET)       : Compile le programme (par défaut)"
	@echo "  make $(GREEN)run$(RESET)         : Compile et exécute le programme avec les arguments M et FILE spécifiés"
	@echo "  make $(GREEN)tests$(RESET)       : Compile et exécute la suite de tests automatisés"
	@echo ""
	@echo "  make $(CYAN)valgrind$(RESET)    : Compile et exécute le programme avec Valgrind pour détecter les fuites mémoire"
	@echo "  make $(CYAN)cache$(RESET)       : Compile et exécute le programme avec Cachegrind pour analyser les défauts de cache"
	@echo "  make $(CYAN)perf$(RESET)        : Compile et exécute le programme avec Perf pour analyser les performances"
	@echo "  make $(CYAN)massif$(RESET)      : Compile et exécute le programme avec Massif pour analyser l'utilisation (TODO)"
	@echo ""
	@echo "  make $(RED)clean$(RESET)           : Supprime les fichiers objets et l'exécutable"
	@echo "  make $(RED)clean-cache$(RESET)     : Supprime les fichiers temporaires de Cachegrind et Massif"
	@echo ""
	@echo "$(GREEN)Variables d'environnement optionnelles :$(RESET)"
	@echo "  $(CYAN)M$(RESET)       : Largeur de justification (Défaut : $(CYAN)[$(RESET)80$(CYAN)]$(RESET))"
	@echo "  $(CYAN)FILE$(RESET)    : Chemin vers le fichier texte (Défaut : $(CYAN)[$(RESET)data/ALaRechercheDuTempsPerdu-1paragraphe-2025-ISO-8859-1$(CYAN)]$(RESET))"
	@echo "  $(CYAN)R$(RESET)       : Nombre de répétitions pour les benchmarks temporels (Défaut : $(CYAN)[$(RESET)5$(CYAN)]$(RESET))"
	@echo "  $(CYAN)M_LIST$(RESET)  : Liste des largeurs M pour les tests de performance (Défaut : $(CYAN)[$(RESET)20 40 60 80 100 120 140 160 180 200$(CYAN)]$(RESET))"
	@echo "  $(CYAN)P_LIST$(RESET)  : Liste des événements Perf à mesurer. (Défaut : $(CYAN)[$(RESET)cache-references,cache-misses,cycles,instructions$(CYAN)]$(RESET))"
	@echo ""

# Compilation simple
all: $(TARGET)

# Exécution
run: $(TARGET)
	@echo "--------------------------------"
	@echo "Running: $(TARGET) $(ARGS)"
	@echo "--------------------------------"
	@env LC_ALL=C $(TARGET) $(ARGS) || true

tests: $(TARGET)
	@./tools/tests.sh
	
# Fuites mémoire
valgrind: $(TARGET)	
	@$(VG) $(VGFLAGS) $(TARGET) $(ARGS)

# Analyse des défauts de cache
cache: $(TARGET)	
	@$(VG) $(CGFLAGS) $(TARGET) $(ARGS)
	@echo "Pour visualiser les résultats :"
	@echo "kcachegrind $$(ls -tr cachegrind.out.* | tail -1)"

# Analyse des performances sur 5 batchs
perf: $(TARGET)
	@$(PERF) stat -e $(P_LIST) --repeat $(R) $(TARGET) $(ARGS)

massif: $(TARGET) # Todo

# Création de la documentation avec Doxygen, règle par défaut
index: Doxyfile | $(DOC_DIR)
	@echo "Génération de la documentation..."
	@doxygen Doxyfile
	@echo "Pour ouvrir la documentation : $(DOC_DIR)/index.html"


###############################
######     Nettoyage     ######
###############################

# Attention ! A n'utiliser que si vous avez doxygen installé pour régénérer la doc avec `make index`
clean-doc:
	@$(RM) $(DOC_DIR)

clean-cache:
	@rm -f cachegrind.out.* massif.out.* 2>/dev/null || true

clean:	
	$(RM) $(TEMP)