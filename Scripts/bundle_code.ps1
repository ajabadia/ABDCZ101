
# Script to bundle all source code into a single timestamped text file
# Usage: .\bundle_code.ps1

$sourceDir = ".\Source"
$timestamp = Get-Date -Format "yyyyMMddHHmm"
$outputFile = "$PWD\TOTALCODE$timestamp.txt"
$controlFile = "$PWD\CONTROL_FILESCODE$timestamp.txt"

# Extensions to include
$includeExtensions = @(".h", ".cpp", ".hpp")

Write-Host "Bundling code from $sourceDir to $outputFile..."
Write-Host "Logging status to $controlFile..."

# Create Writers
$mainWriter = [System.IO.StreamWriter]::new($outputFile, $false, [System.Text.Encoding]::UTF8)
$controlWriter = [System.IO.StreamWriter]::new($controlFile, $false, [System.Text.Encoding]::UTF8)

# Separator Constants
$separator = "`r`n================================================================================`r`n"
$separator2 = "================================================================================`r`n"

try {
    # Recursively find files
    Get-ChildItem -Path $sourceDir -Recurse | Where-Object { 
        !$_.PSIsContainer -and ($includeExtensions -contains $_.Extension)
    } | ForEach-Object {
        $filePath = $_.FullName
        $relativePath = Resolve-Path -Path $filePath -Relative
        $status = "KO" # Default
        
        try {
            # Read content (still safe to use Get-Content for reading)
            $content = Get-Content -Path $filePath -Raw
            
            if ([string]::IsNullOrWhiteSpace($content)) {
                $status = "EMPTY"
                # Log to control file
                $controlWriter.WriteLine("$relativePath : $status")
            }
            else {
                # Append to Bundle using StreamWriter
                $header = "FILE: $relativePath`r`n"
                
                $mainWriter.Write($separator)
                $mainWriter.Write($header)
                $mainWriter.Write($separator2)
                $mainWriter.Write($content)
                
                $status = "OK"
                # Log to control file
                $controlWriter.WriteLine("$relativePath : $status")
                
                Write-Host "Added: $relativePath"
            }
        }
        catch {
            $status = "ERROR: $_"
            $controlWriter.WriteLine("${relativePath} : $status")
            Write-Error "Failed to process ${relativePath}: $_"
        }
        Start-Sleep -Milliseconds 10
    }
}
finally {
    # Close writers explicitly
    $mainWriter.Close()
    $mainWriter.Dispose()
    
    $controlWriter.Close()
    $controlWriter.Dispose()
}

Write-Host "Done! Code bundle: $outputFile"
Write-Host "Control log: $controlFile"
