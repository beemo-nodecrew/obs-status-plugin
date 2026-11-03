# OBS Status Overlay 설치/제거 스크립트
# 사용: install.cmd (설치), uninstall.cmd (제거)
param(
    [switch] $Uninstall
)

$ErrorActionPreference = 'Stop'
$PluginName = 'obs-status-overlay'
$PluginsDir = Join-Path $env:ProgramData 'obs-studio\plugins'
$Target = Join-Path $PluginsDir $PluginName
$Source = Join-Path $PSScriptRoot $PluginName

function Test-Admin {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

if (-not (Test-Admin)) {
    $arguments = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"")
    if ($Uninstall) { $arguments += '-Uninstall' }
    Start-Process -FilePath 'powershell.exe' -ArgumentList $arguments -Verb RunAs
    exit 0
}

try {
    if (Get-Process -Name 'obs64' -ErrorAction SilentlyContinue) {
        throw 'OBS Studio가 실행 중입니다. OBS를 종료한 뒤 다시 실행하세요.'
    }

    if (Test-Path $Target) {
        Remove-Item -Path $Target -Recurse -Force
    }

    if ($Uninstall) {
        Write-Host "제거 완료: $Target"
    } else {
        if (-not (Test-Path (Join-Path $Source "bin\64bit\$PluginName.dll"))) {
            throw "플러그인 파일을 찾을 수 없습니다: $Source"
        }
        New-Item -ItemType Directory -Path $PluginsDir -Force | Out-Null
        Copy-Item -Path $Source -Destination $Target -Recurse -Force
        Write-Host "설치 완료: $Target"
        Write-Host 'OBS 실행 후 도구 > 녹화 상태 오버레이 에서 설정하세요.'
    }
} catch {
    Write-Host "오류: $($_.Exception.Message)" -ForegroundColor Red
}

Read-Host '엔터를 누르면 창이 닫힙니다'
