#!/bin/sh
# Usage: ./build-repo.sh http://repo.yourdomain.com
# Needs dpkg-scanpackages (Linux: apt install dpkg-dev | macOS: brew install dpkg)
set -e
cd "$(dirname "$0")"
URL="${1:-http://repo.example.com}"

mkdir -p debs
dpkg-scanpackages -m debs /dev/null > Packages
bzip2 -kf Packages
gzip -kf Packages

cat > index.html <<HTML
<!DOCTYPE html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>TimeFix Repo</title></head>
<body style="font-family:-apple-system,Helvetica;text-align:center;padding:40px">
<h2>TimeFix Repo</h2>
<p><a href="cydia://url/https://cydia.saurik.com/api/share#?source=${URL}">Add to Cydia</a></p>
<p>Or in Cydia: Sources &rarr; Edit &rarr; Add &rarr; <code>${URL}</code></p>
</body></html>
HTML
echo "Done. Upload the contents of this folder to the web root of ${URL}"
