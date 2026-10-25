#!/usr/bin/env bash
# Tests AODjustify — lisible + assertions + résumé
# Anti — Justify (AOD)

# --------- réglages ----------
DELAY=1.5               # pause entre tests (secondes)
MAX_STDERR=120          # lignes à montrer (stderr)
MAX_STDOUT=60           # lignes à montrer (stdout)
# -----------------------------

set -u  # pas de -e: on veut exécuter même si le binaire retourne ≠0

# === Trouver la racine (Makefile + src/)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$SCRIPT_DIR"
while [ ! -f "$ROOT/Makefile" ] || [ ! -d "$ROOT/src" ]; do
  PARENT="$(dirname "$ROOT")"
  [ "$PARENT" = "$ROOT" ] && { echo "Impossible de trouver la racine (Makefile + src/)"; exit 2; }
  ROOT="$PARENT"
done

BIN="$ROOT/bin/AODjustify"
DATA="$ROOT/data"
OUT="$ROOT/divers/out"
mkdir -p "$OUT" "$DATA"

# Build si besoin
if [ ! -x "$BIN" ]; then
  make -C "$ROOT" -s || { echo "Build raté"; exit 2; }
fi

# Fixtures
ISO_FILE="$DATA/iso_latin1.in"
UTF8_FILE="$DATA/utf8.in"
NOPERM_FILE="$DATA/noperm.in"
DIR_PATH="$DATA"

# Nettoyage
rm -f "$ISO_FILE" "$UTF8_FILE" "$NOPERM_FILE"
printf 'no access\n' > "$NOPERM_FILE"
chmod 000 "$NOPERM_FILE" || true

# Fichiers de test (robustes)
printf 'café crème — UTF8\n' > "$UTF8_FILE"
if command -v iconv >/dev/null 2>&1; then
  printf 'café crème en ISO-8859-1\n' | iconv -f UTF-8 -t ISO-8859-1 > "$ISO_FILE"
else
  printf 'caf\xE9 cr\xE8me en ISO-8859-1\n' > "$ISO_FILE"
fi
echo "no access" > "$NOPERM_FILE" && chmod 000 "$NOPERM_FILE" || true

# Couleurs
G="\033[0;32m"; R="\033[0;31m"; Y="\033[0;33m"; C="\033[0;36m"; B="\033[1m"; X="\033[0m"

# Résultats cumulés
TOTAL=0; PASS=0; FAIL=0

# run <name> <expected_rc> <expected_regex> [envprefix] [args...]
run() {
  local name="$1"; shift
  local exp_rc="$1"; shift
  local exp_rx="$1"; shift

  # récupère ENV... jusqu'à --
  local envprefix=()
  while [ $# -gt 0 ] && [ "$1" != "--" ]; do
    envprefix+=("$1")
    shift
  done
  [ "$1" = "--" ] && shift   # consomme le --

  # Le reste = ARGS...
  local args=("$@")

  # Normalise les arguments "fichier" : on enlève le .in (et .out au cas où)
  for i in "${!args[@]}"; do
    case "${args[$i]}" in
      *.in)  args[$i]="${args[$i]%.in}" ;;
      *.out) args[$i]="${args[$i]%.out}" ;;
    esac
  done


  local so="$OUT/${name}.stdout"
  local se="$OUT/${name}.stderr"

  echo -e "${B}================= $name =================${X}"
  echo -e "CMD: ${C}${envprefix[*]:+${envprefix[*]} }\"$BIN\" ${args[*]}${X}"

  if [ "${#envprefix[@]}" -gt 0 ]; then
    "${envprefix[@]}" "$BIN" "${args[@]}" 1>"$so" 2>"$se"
  else
    "$BIN" "${args[@]}" 1>"$so" 2>"$se"
  fi
  local rc=$?

  # Assertions
  local ok=1
  [ "$rc" -eq "$exp_rc" ] || ok=0
  if ! grep -Eaiq -- "$exp_rx" "$se" && ! grep -Eaiq -- "$exp_rx" "$so"; then
    ok=0
  fi

  echo -e "exit=$rc  | attendu=$exp_rc"
  echo -e "--- ${B}STDERR${X} ---"; [ -s "$se" ] && sed -n "1,${MAX_STDERR}p" "$se" || echo "(vide)"
  echo -e "--- ${B}STDOUT${X} ---"; [ -s "$so" ] && sed -n "1,${MAX_STDOUT}p" "$so" || echo "(vide)"

  if [ $ok -eq 1 ]; then
    echo -e "${G}PASS${X}"; PASS=$((PASS+1))
  else
    echo -e "${R}FAIL${X}  ${Y}(motif attendu: /$exp_rx/ ; rc=$exp_rc)${X}"
    FAIL=$((FAIL+1))
  fi
  TOTAL=$((TOTAL+1))
  sleep "$DELAY"
  echo
}

# On enleve le .in des fichiers


# ================== TESTS ==================
# Rem: on cherche un motif "attendu" réaliste dans stderr OU stdout
# Ajuste les regex si ton main change le format.

# Usage incorrect
run "T0 Test d'usage"                 1 '^AODJustify ERROR>.*[Uu]sage'                           --
# Fichier ISO-8859-1 correct
run "T1 Fichier valide"               0 '^AODJustify CORRECT>.*'                                 -- 40 "$ISO_FILE"
# Fichier UTF-8 incorrect
run "T2 Fichier mal encodé"           1 '^AODJustify ERROR>.*ISO-8859-1'                         -- 40 "$UTF8_FILE"
# Fichier manquant
run "T3 Fichier inexistant"           1 '^AODJustify ERROR>.*(No such file|cannot open)'         env LC_ALL=C -- 40 "$DATA/does_not_exist_$$"
# Fichier sans permission
run "T4 Fichier sans permission"      1 '^AODJustify ERROR>.*Permission denied'                  env LC_ALL=C -- 40 "$NOPERM_FILE"
# PATH incorrect
run "T5 Chemin introuvable"           1 '^AODJustify ERROR>.*(No such file or directory|not found)' env PATH=/nope LC_ALL=C -- 40 "$ISO_FILE"
# Chemin est un répertoire
run "T6 Le fichier est un répertoire" 1 '^AODJustify ERROR>.*'                                   -- 40 "$DIR_PATH"



# Restaure droits
chmod 644 "$NOPERM_FILE" 2>/dev/null || true

# ================== RÉCAP ==================
echo -e "${B}========= RÉCAP =========${X}"
echo -e "Total: $TOTAL  |  ${G}PASS: $PASS${X}  |  ${R}FAIL: $FAIL${X}"
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
