#!/usr/bin/env bash
# ==============================================================================
# release.sh - Otonom SemVer Sürüm ve Release Yönetim Betiği
# Kullanım: ./release.sh [patch|minor|major]
# ==============================================================================

set -eo pipefail

# ANSI Renk Kodları
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

CHANGELOG_FILE="CHANGELOG.md"
TMP_BLOCK=""
CHANGELOG_BAK=""

# Temizlik ve Hata Yakalama Trap'i
cleanup() {
    local exit_code=$?
    if [ -n "$TMP_BLOCK" ] && [ -f "$TMP_BLOCK" ]; then
        rm -f "$TMP_BLOCK"
    fi
    if [ -n "$CHANGELOG_BAK" ] && [ -f "$CHANGELOG_BAK" ]; then
        # Eğer işlem başarıyla tamamlanmadıysa ve yedek duruyorsa geri yükle
        if [ $exit_code -ne 0 ]; then
            echo -e "\n${RED}❌ Hata oluştu! CHANGELOG.md eski haline geri yükleniyor...${NC}"
            mv "$CHANGELOG_BAK" "$CHANGELOG_FILE"
        else
            rm -f "$CHANGELOG_BAK"
        fi
    fi
}
trap cleanup EXIT INT TERM

# 1. Argüman Kontrolü
BUMP_TYPE="${1:-}"
if [[ ! "$BUMP_TYPE" =~ ^(patch|minor|major)$ ]]; then
    echo -e "${RED}${BOLD}HATA: Geçersiz veya eksik sürüm tipi!${NC}"
    echo -e "Kullanım: ${CYAN}./release.sh [patch | minor | major]${NC}"
    echo -e "  ${BOLD}patch${NC} : Hata düzeltmeleri ve küçük güncellemeler (v1.0.0 -> v1.0.1)"
    echo -e "  ${BOLD}minor${NC} : Geriye uyumlu yeni özellikler          (v1.0.0 -> v1.1.0)"
    echo -e "  ${BOLD}major${NC} : Geriye uyumsuz köklü değişiklikler      (v1.0.0 -> v2.0.0)"
    exit 1
fi

# 2. Git ve Dosya Kontrolleri
if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo -e "${RED}HATA: Bu dizin geçerli bir Git deposu değil!${NC}"
    exit 1
fi

if [ ! -f "$CHANGELOG_FILE" ]; then
    echo -e "${YELLOW}UYARI: $CHANGELOG_FILE bulunamadı. Yeni bir şablon oluşturuluyor...${NC}"
    cat << 'EOF' > "$CHANGELOG_FILE"
# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

EOF
fi

# 3. Mevcut En Son Sürümü Bul (Önce Git Tag'leri, Yoksa CHANGELOG)
LATEST_TAG=$(git tag --sort=-v:refname 2>/dev/null | grep -E '^v[0-9]+\.[0-9]+\.[0-9]+' | head -n 1 || true)

if [ -z "$LATEST_TAG" ]; then
    LATEST_TAG=$(grep -E -m 1 '^## \[v?[0-9]+\.[0-9]+\.[0-9]+\]' "$CHANGELOG_FILE" 2>/dev/null | sed -E 's/^## \[v?([0-9]+\.[0-9]+\.[0-9]+)\].*/v\1/' || true)
fi

if [ -z "$LATEST_TAG" ]; then
    LATEST_TAG="v0.0.0"
fi

# 4. Yeni Sürüm Numarasını Hesapla (SemVer)
RAW_VER="${LATEST_TAG#v}"
IFS='.' read -r MAJOR MINOR PATCH <<< "$RAW_VER"

# Sayısal değer kontrolü
MAJOR=${MAJOR:-0}
MINOR=${MINOR:-0}
PATCH=${PATCH:-0}

case "$BUMP_TYPE" in
    patch)
        PATCH=$((PATCH + 1))
        ;;
    minor)
        MINOR=$((MINOR + 1))
        PATCH=0
        ;;
    major)
        MAJOR=$((MAJOR + 1))
        MINOR=0
        PATCH=0
        ;;
esac

NEXT_TAG="v${MAJOR}.${MINOR}.${PATCH}"
RELEASE_DATE=$(date +%Y-%m-%d)

echo -e "\n${BOLD}${CYAN}====================================================${NC}"
echo -e "${BOLD}       🚀 OTONOM SEMVER RELEASE SİSTEMİ${NC}"
echo -e "${BOLD}${CYAN}====================================================${NC}"
echo -e "📦 Mevcut Sürüm  : ${YELLOW}${BOLD}${LATEST_TAG}${NC}"
echo -e "🎯 Hedef Sürüm   : ${GREEN}${BOLD}${NEXT_TAG}${NC} (${BUMP_TYPE})"
echo -e "📅 Yayın Tarihi  : ${BLUE}${RELEASE_DATE}${NC}"
echo -e "${CYAN}----------------------------------------------------${NC}\n"

# 5. İnteraktif Değişiklik Maddelerini Kullanıcıdan Al
echo -e "${BOLD}Lütfen yeni sürüme ait değişiklik maddelerini girin:${NC}"
echo -e "${YELLOW}(Her satıra bir madde yazın. Bitirmek için boş bir satırda [ENTER] tuşuna basın)${NC}"

ENTRIES=()
while IFS= read -r -p "  > " line; do
    # Boş satır girilirse döngüden çık
    if [ -z "$line" ]; then
        break
    fi
    # Maddenin başında tire (-) yoksa otomatik ekle
    if [[ ! "$line" =~ ^[[:space:]]*[-*] ]]; then
        line="- $line"
    fi
    ENTRIES+=("$line")
done

# Eğer kullanıcı hiç madde girmediyse varsayılan bir madde ata
if [ ${#ENTRIES[@]} -eq 0 ]; then
    echo -e "${YELLOW}Hiç madde girilmedi. Varsayılan başlık kullanılıyor.${NC}"
    ENTRIES+=("- Release ${NEXT_TAG}")
fi

# 6. Yeni CHANGELOG Bloğunu Oluştur
TMP_BLOCK=$(mktemp)
cat << EOF > "$TMP_BLOCK"
## [${NEXT_TAG}] - ${RELEASE_DATE}

EOF

for entry in "${ENTRIES[@]}"; do
    echo "$entry" >> "$TMP_BLOCK"
done

# 7. CHANGELOG.md Dosyasını Güncelle (Yedek Alarak)
CHANGELOG_BAK=$(mktemp)
cp "$CHANGELOG_FILE" "$CHANGELOG_BAK"

TMP_RESULT=$(mktemp)
awk '
  NR == FNR {
    block = (block == "" ? "" : block "\n") $0
    next
  }
  /^## \[/ && !inserted {
    print block "\n"
    inserted = 1
  }
  { print }
  END {
    if (!inserted) {
      if (block != "") print "\n" block
    }
  }
' "$TMP_BLOCK" "$CHANGELOG_FILE" > "$TMP_RESULT"

mv "$TMP_RESULT" "$CHANGELOG_FILE"

# 8. Değişikliklerin ve Git Durumunun Önizlemesi
echo -e "\n${BOLD}${CYAN}📋 CHANGELOG.MD ÖNİZLEMESİ:${NC}"
echo -e "${BLUE}----------------------------------------------------${NC}"
cat "$TMP_BLOCK"
echo -e "${BLUE}----------------------------------------------------${NC}\n"

echo -e "${BOLD}${CYAN}📂 GİT ÇALIŞMA ALANI DURUMU:${NC}"
git status --short

# 9. Kullanıcı Onayı Al
echo ""
read -r -p "Bu sürümü onaylayıp GitHub'a göndermek istiyor musunuz? [y/N]: " CONFIRM
CONFIRM=$(echo "$CONFIRM" | tr '[:upper:]' '[:lower:]')

if [[ "$CONFIRM" != "y" && "$CONFIRM" != "yes" ]]; then
    echo -e "\n${YELLOW}⚠️  İşlem kullanıcı tarafından iptal edildi.${NC}"
    mv "$CHANGELOG_BAK" "$CHANGELOG_FILE"
    CHANGELOG_BAK=""
    echo -e "CHANGELOG.md eski haline getirildi. Hiçbir Git işlemi yapılmadı.\n"
    exit 0
fi

# Onaylandı: Yedeği temizle
rm -f "$CHANGELOG_BAK"
CHANGELOG_BAK=""

# 10. Git Commit, Annotated Tag ve Push
echo -e "\n${CYAN}📦 Git değişiklikleri paketleniyor...${NC}"
git add "$CHANGELOG_FILE"

# Eğer başka bekleyen dosyalar varsa onları da dahil edelim mi?
STAGED_OR_UNTRACKED=$(git status --porcelain | grep -v "$CHANGELOG_FILE" || true)
if [ -n "$STAGED_OR_UNTRACKED" ]; then
    git add .
fi

git commit -m "chore(release): ${NEXT_TAG}"
echo -e "${GREEN}✔ Commit oluşturuldu: chore(release): ${NEXT_TAG}${NC}"

echo -e "${CYAN}🏷️  Git Annotated Tag oluşturuluyor: ${NEXT_TAG}...${NC}"
git tag -a "${NEXT_TAG}" -m "Release ${NEXT_TAG}"
echo -e "${GREEN}✔ Tag oluşturuldu.${NC}"

echo -e "${CYAN}🚀 GitHub'a gönderiliyor (git push origin HEAD --follow-tags)...${NC}"
git push origin HEAD --follow-tags

REMOTE_URL=$(git config --get remote.origin.url || true)
REPO_PATH=$(echo "$REMOTE_URL" | sed -E 's/.*github\.com[:\/]([^\.]+)(\.git)?/\1/')

echo -e "\n${BOLD}${GREEN}====================================================${NC}"
echo -e "${BOLD}${GREEN}🎉 SÜRÜM ${NEXT_TAG} BAŞARIYLA YAYINLANDI!${NC}"
echo -e "${BOLD}${GREEN}====================================================${NC}"
if [ -n "$REPO_PATH" ]; then
    echo -e "🔗 GitHub Actions Takibi : ${CYAN}https://github.com/${REPO_PATH}/actions${NC}"
    echo -e "📦 GitHub Releases Sayfası: ${CYAN}https://github.com/${REPO_PATH}/releases/tag/${NEXT_TAG}${NC}"
fi
echo ""
