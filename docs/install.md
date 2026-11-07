# Installation

## Download Precompiled Binaries

The fastest way to get started is to download the precompiled binary for your operating system. Extract the archive, and move the `hm` (or `hm.exe`) file to a directory in your `PATH`.

<div style="display: flex; flex-wrap: wrap; gap: 12px; margin-top: 24px; margin-bottom: 24px; align-items: center;">
  <a href="https://github.com/Gag91/Help_Make/releases/latest/download/hm-windows-x86_64.zip" style="display: inline-flex; align-items: center; justify-content: center; height: 40px; padding: 0 24px; border-radius: 20px; font-size: 14px; font-weight: 600; text-decoration: none; white-space: nowrap; transition: color 0.25s, border-color 0.25s, background-color 0.25s; background-color: var(--vp-button-brand-bg); color: var(--vp-button-brand-text);">Download for Windows</a>
  
  <a href="https://github.com/Gag91/Help_Make/releases/latest/download/hm-linux-x86_64.tar.gz" style="display: inline-flex; align-items: center; justify-content: center; height: 40px; padding: 0 24px; border-radius: 20px; font-size: 14px; font-weight: 600; text-decoration: none; white-space: nowrap; transition: color 0.25s, border-color 0.25s, background-color 0.25s; background-color: var(--vp-button-brand-bg); color: var(--vp-button-brand-text);">Download for Linux</a>
  
  <a href="https://github.com/Gag91/Help_Make/releases/latest/download/hm-macos-x86_64.tar.gz" style="display: inline-flex; align-items: center; justify-content: center; height: 40px; padding: 0 24px; border-radius: 20px; font-size: 14px; font-weight: 600; text-decoration: none; white-space: nowrap; transition: color 0.25s, border-color 0.25s, background-color 0.25s; background-color: var(--vp-button-brand-bg); color: var(--vp-button-brand-text);">Download for macOS</a>
</div>

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