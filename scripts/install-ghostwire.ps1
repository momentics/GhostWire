<#
.SYNOPSIS
    Скачивает и устанавливает библиотеку GhostWire из официального релиза.
.DESCRIPTION
    Загружает бинарные файлы и заголовки GhostWire из GitHub Releases,
    проверяет контрольные суммы и распаковывает в libs/ghostwire/.
.PARAMETER Version
    Версия GhostWire для установки (по умолчанию 1.0.0).
.EXAMPLE
    .\scripts\install-ghostwire.ps1
    .\scripts\install-ghostwire.ps1 -Version "1.0.0"
#>

param(
    [string]$Version = "1.0.4"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$installDir = Join-Path $repoRoot (Join-Path "libs" "ghostwire")
$releaseBase = "https://github.com/momentics/GhostWire.dist/releases/download/$Version"

# Определяем платформу и архитекту
$os = $PSVersionTable.OS
if ($IsWindows -or ($PSVersionTable.PSVersion.Major -le 5)) {
    $platform = "windows"
    $arch = "x86_64"
    $binAsset = "ghostwire-${Version}-${platform}-${arch}.zip"
} elseif ($IsMacOS) {
    $platform = "macos"
    $arch = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString().ToLower()
    if ($arch -eq "arm64") { $arch = "aarch64" }
    $binAsset = "ghostwire-${Version}-${platform}-${arch}.tar.gz"
} elseif ($IsLinux) {
    $platform = "linux"
    $arch = "x86_64"
    $binAsset = "ghostwire-${Version}-${platform}-${arch}.tar.gz"
} else {
    Write-Error "Неподдерживаемая платформа"
    exit 1
}

$shaAsset = "ghostwire-${Version}.sha256"

function Download-File {
    param([string]$Url, [string]$OutFile)
    Write-Host "  Загрузка: $Url"
    Invoke-WebRequest -Uri $Url -OutFile $OutFile -UseBasicParsing
}

function Verify-Checksum {
    param([string]$File, [string]$ExpectedHash)
    $actualHash = (Get-FileHash -Path $File -Algorithm SHA256).Hash.ToLower()
    if ($actualHash -ne $ExpectedHash.ToLower()) {
        Write-Error "Контрольная сумма не совпадает для $File"
        return $false
    }
    return $true
}

try {
    Write-Host "Установка GhostWire v$Version..."

    # Создаем директорию
    if (Test-Path $installDir) {
        Remove-Item $installDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $installDir -Force | Out-Null

    $tempDir = Join-Path $env:TEMP "ghostwire-install"
    if (Test-Path $tempDir) {
        Remove-Item $tempDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $tempDir -Force | Out-Null

    # Скачиваем контрольные суммы
    Write-Host "`n[1/4] Загрузка контрольных сумм..."
    $shaFile = Join-Path $tempDir $shaAsset
    Download-File -Url "$releaseBase/$shaAsset" -OutFile $shaFile

    # Загружаем хеши в таблицу
    $shaContent = Get-Content $shaFile -Raw
    $hashes = @{}
    foreach ($line in $shaContent -split "`n") {
        $line = $line.Trim()
        if ($line -match '^([a-f0-9]+)\s+(.+)$') {
            $hashes[$matches[2]] = $matches[1]
        }
    }

    # Скачиваем бинарники
    Write-Host "`n[2/3] Загрузка бинарных файлов ($platform-$arch)..."
    $binFile = Join-Path $tempDir $binAsset
    Download-File -Url "$releaseBase/$binAsset" -OutFile $binFile

    # Верификация
    Write-Host "`n[3/3] Проверка контрольных сумм..."
    $binOk = $false

    foreach ($entry in $hashes.GetEnumerator()) {
        if ($entry.Key -eq $binAsset) {
            $binOk = Verify-Checksum -File $binFile -ExpectedHash $entry.Value
        }
    }

    if (-not $binOk) {
        Write-Error "Ошибка: контрольная сумма бинарных файлов не совпадает"
        exit 1
    }
    Write-Host "  Контрольные суммы совпали"

    # Распаковка
    Write-Host "`nРаспаковка файлов..."

    if ($binAsset.EndsWith(".zip")) {
        Expand-Archive -Path $binFile -DestinationPath $installDir -Force
    } else {
        # tar.gz для Linux/macOS
        tar -xzf $binFile -C $installDir
    }

    # Очистка
    Remove-Item $tempDir -Recurse -Force

    Write-Host "`nGhostWire v$Version успешно установлен в: $installDir"
    Write-Host "Структура:"
    Get-ChildItem $installDir -Recurse | ForEach-Object {
        $rel = $_.FullName.Replace($installDir, "").TrimStart('\')
        Write-Host "  $rel"
    }

} catch {
    Write-Error "Ошибка при установке GhostWire: $_"
    exit 1
}
