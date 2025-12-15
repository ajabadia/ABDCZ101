# Interactive Code Signing Script
# Automatically finds signtool, lists EXEs, lists Certs, and asks user to select.

# 1. Find Signtool
$searchPaths = @(
    "C:\Program Files (x86)\Windows Kits\10\bin\*\x64\signtool.exe",
    "C:\Program Files (x86)\Windows Kits\10\bin\*\x86\signtool.exe",
    "C:\Program Files (x86)\Windows Kits\8.1\bin\*\x64\signtool.exe"
)

$signtool = $null
foreach ($path in $searchPaths) {
    $found = Get-ChildItem -Path $path -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1
    if ($found) {
        $signtool = $found.FullName
        break
    }
}

if (-not $signtool) {
    Write-Error "CRITICAL: 'signtool.exe' not found. Ensure Windows SDK is installed."
    exit 1
}
Write-Host "Using Signtool: $signtool`n" -ForegroundColor Cyan

# 2. Select Executable
$exes = Get-ChildItem -Path . -Filter *.exe
if ($exes.Count -eq 0) {
    Write-Error "No .exe files found in current directory."
    exit 1
}

Write-Host "=== Select Executable to Sign ===" -ForegroundColor Green
for ($i = 0; $i -lt $exes.Count; $i++) {
    Write-Host "[$($i+1)] $($exes[$i].Name)"
}
$exeIndex = Read-Host "Enter number (default 1)"
if ([string]::IsNullOrWhiteSpace($exeIndex)) { $exeIndex = 1 }
$selectedExe = $exes[$exeIndex - 1]

if (-not $selectedExe) {
    Write-Error "Invalid selection."
    exit 1
}
Write-Host "Selected: $($selectedExe.Name)`n" -ForegroundColor Yellow

# 3. Select Certificate
$certs = Get-ChildItem Cert:\CurrentUser\My | Sort-Object Subject
if ($certs.Count -eq 0) {
    Write-Error "No certificates found in CurrentUser\My store."
    exit 1
}

Write-Host "=== Select Certificate ===" -ForegroundColor Green
for ($i = 0; $i -lt $certs.Count; $i++) {
    $c = $certs[$i]
    
    # Try to find a human-readable name
    $name = $c.FriendlyName
    if ([string]::IsNullOrWhiteSpace($name)) {
        # Fallback to CN
        $name = $c.Subject -replace "CN=", "" -replace ",.*", ""
    }
    
    # Issuer Name
    $issuer = $c.Issuer -replace "CN=", "" -replace ",.*", ""
    
    # Expiration
    $expiry = $c.NotAfter.ToString("yyyy-MM-dd")

    Write-Host "[$($i+1)] $name" -ForegroundColor White
    Write-Host "      Issuer: $issuer | Exp: $expiry | Thumb: $($c.Thumbprint.Substring(0, 8))..." -ForegroundColor DarkGray
}
$certIndex = Read-Host "Enter number (default 1)"
if ([string]::IsNullOrWhiteSpace($certIndex)) { $certIndex = 1 }
$selectedCert = $certs[$certIndex - 1]

if (-not $selectedCert) {
    Write-Error "Invalid certificate selection."
    exit 1
}
Write-Host "Selected: $($selectedCert.Subject) [$($selectedCert.Thumbprint)]`n" -ForegroundColor Yellow

# 4. Sign
Write-Host "Signing..." -ForegroundColor Cyan
& $signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /sha1 $selectedCert.Thumbprint "$($selectedExe.FullName)"

if ($LASTEXITCODE -eq 0) {
    Write-Host "SUCCESS: Signed ($($selectedExe.Name)) successfully." -ForegroundColor Green
}
else {
    Write-Error "Signing failed with exit code $LASTEXITCODE."
}
