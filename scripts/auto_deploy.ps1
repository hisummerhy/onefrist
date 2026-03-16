param(
  [string]$envName = 'esp32-s3-devkitc-1',
  [string]$uploadPort = 'COM8',
  [string]$monitorPort = 'COM7'
)

Write-Host "Auto-deploy: build env=$envName uploadPort=$uploadPort monitorPort=$monitorPort"

function Run-Tool($cmd){
  Write-Host "Running: $cmd"
  $proc = Start-Process -FilePath pwsh -ArgumentList "-NoProfile","-Command","$cmd" -NoNewWindow -PassThru -Wait -ErrorAction SilentlyContinue
  if($proc -and $proc.ExitCode -ne 0){
    Write-Host "Command failed with exit code $($proc.ExitCode)"
    return $false
  }
  return $true
}

# Prefer platformio from PATH; use pio if available
$pio = 'pio'

Write-Host "Building firmware..."
$buildCmd = "$pio run -e $envName"
$b = & $pio run -e $envName
if($LASTEXITCODE -ne 0){ Write-Host "Build failed (exit $LASTEXITCODE)"; exit $LASTEXITCODE }

Write-Host "Uploading to $uploadPort..."
$upCmd = "$pio run -e $envName --target upload --upload-port $uploadPort"
& $pio run -e $envName --target upload --upload-port $uploadPort
if($LASTEXITCODE -ne 0){ Write-Host "Upload failed (exit $LASTEXITCODE)"; exit $LASTEXITCODE }

Write-Host "Starting serial monitor on $monitorPort (new window)"
# Start a new PowerShell window and keep it open
Start-Process powershell -ArgumentList "-NoExit","-Command","pio device monitor -e $envName --port $monitorPort"

Write-Host "Auto-deploy finished"
