param(
  [Parameter(Mandatory=$false)] [string]$deviceHost = "192.168.6.102",
  [Parameter(Mandatory=$false)] [int]$durationSec = 120,
  [Parameter(Mandatory=$false)] [int]$sleepSec = 6
)

Write-Output "Stress test start: host=$deviceHost duration=${durationSec}s sleep=${sleepSec}s"
$end = (Get-Date).AddSeconds($durationSec)
while((Get-Date) -lt $end){
  try{
    $list = Invoke-RestMethod -Uri "http://$deviceHost/api/list" -Method Get -TimeoutSec 10
    if(-not $list){ Write-Output "No list returned"; break }
    $count = $list.Count
    if($count -eq 0){ Write-Output "Empty list"; break }
    $i = Get-Random -Minimum 0 -Maximum $count
    Write-Output "Playing index $i / $count"
    Invoke-RestMethod -Uri "http://$deviceHost/api/play_index?i=$i" -Method Get -TimeoutSec 10 | ConvertTo-Json
  } catch {
    Write-Output "Request error: $_"
  }
  Start-Sleep -Seconds $sleepSec
}
Write-Output "Stress test finished"