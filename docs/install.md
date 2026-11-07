# Installation

## Download Precompiled Binaries

The fastest way to get started is to download the precompiled binary for your operating system. Extract the archive, and move the `hm` (or `hm.exe`) file to a directory in your `PATH`.

<div style="display: flex; gap: 10px; margin-top: 20px; flex-wrap: wrap;">
  <a href="https://github.com/Gag91/Help_Make/releases/latest/download/hm-windows-x86_64.zip" style="display: inline-block; padding: 10px 20px; background-color: var(--vp-c-brand); color: white; border-radius: 4px; text-decoration: none; font-weight: bold;">Download for Windows</a>
  
  <a href="https://github.com/Gag91/Help_Make/releases/latest/download/hm-linux-x86_64.tar.gz" style="display: inline-block; padding: 10px 20px; background-color: var(--vp-c-brand); color: white; border-radius: 4px; text-decoration: none; font-weight: bold;">Download for Linux</a>
  
  <a href="https://github.com/Gag91/Help_Make/releases/latest/download/hm-macos-x86_64.tar.gz" style="display: inline-block; padding: 10px 20px; background-color: var(--vp-c-brand); color: white; border-radius: 4px; text-decoration: none; font-weight: bold;">Download for macOS</a>
</div>

<br>

## Install via Script (Automated)

If you want HelpMake to automatically download, build, and add itself to your `PATH`, use our install scripts:

[Linux]
```bash
curl -fsSL https://raw.githubusercontent.com/Gag91/Help_Make/main/install.sh | bash
```
[Windows]
```bash [Windows]
curl -fsSL -o install.bat https://raw.githubusercontent.com/Gag91/Help_Make/main/install.bat && .\install.bat
```