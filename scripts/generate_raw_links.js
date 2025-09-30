// scripts/generate_raw_links.js
const {execSync} = require("child_process");
const {writeFileSync} = require("fs");
const path = require("path");

function sh(cmd) { return execSync(cmd, {stdio : [ "ignore", "pipe", "pipe" ]}).toString().trim(); }

function detectOwnerRepo() {
  try {
    const remote = sh("git remote get-url origin");
    const m = remote.match(/github\.com[:/](.+?)\/([^.]+?)(?:\.git)?$/i);
    if (!m) throw new Error("Cannot parse origin URL");
    return {owner : m[1], repo : m[2]};
  } catch (e) {
    console.error("Nie udało się wykryć owner/repo z 'origin'.", e.message);
    process.exit(1);
  }
}

function main() {
  const branch = process.env.RAW_BRANCH || "main"; // można nadpisać w settings
  const {owner, repo} = detectOwnerRepo();

  // wszystkie pliki: śledzone + nowe, z poszanowaniem .gitignore
  const files = sh("git ls-files --cached --others --exclude-standard").split("\n").filter(Boolean).sort();

  const links = files.map(f => `https://raw.githubusercontent.com/${owner}/${repo}/${branch}/${f}`);

  const outfile = path.join(process.cwd(), "ALL_FILES_RAW.txt");
  writeFileSync(outfile, links.join("\n") + "\n", "utf8");

  console.log(`Zaktualizowano ${outfile} — ${links.length} plików (branch: ${branch})`);
}

main();