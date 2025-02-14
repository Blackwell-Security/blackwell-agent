# Check if there is an upgrade in progress
if (Test-Path ".\upgrade\upgrade_in_progress") {
    write-output "$(Get-Date -format u) - There is an upgrade in progress. Aborting..." >> .\upgrade\upgrade.log
    exit 1
}

write-output "0" | out-file ".\upgrade\upgrade_in_progress" -encoding ascii

# Delete previous upgrade.log
Remove-Item -Path ".\upgrade\upgrade.log" -ErrorAction SilentlyContinue

# Select powershell
if (Test-Path "$env:windir\sysnative") {
    write-output "$(Get-Date -format u) - Sysnative Powershell will be used to access the registry." >> .\upgrade\upgrade.log
    Set-Alias Start-NativePowerShell "$env:windir\sysnative\WindowsPowerShell\v1.0\powershell.exe"
} else {
    Set-Alias Start-NativePowerShell "$env:windir\System32\WindowsPowerShell\v1.0\powershell.exe"
}


function remove_upgrade_files {
    Remove-Item -Path ".\upgrade\*"  -Exclude "*.log", "upgrade_result" -ErrorAction SilentlyContinue
    Remove-Item -Path ".\blackwell-agent*.msi" -ErrorAction SilentlyContinue
    Remove-Item -Path ".\do_upgrade.ps1" -ErrorAction SilentlyContinue
}


function get_blackwell_installation_directory {
    Start-NativePowerShell {
        $path1 = "HKLM:\SOFTWARE\WOW6432Node\Blackwell, Inc.\Blackwell Agent"
        $key1 = "BlackwellInstallDir"

        $path2 = "HKLM:\SOFTWARE\WOW6432Node\ossec"
        $key2 = "Install_Dir"

        $BlackwellInstallDir = $null

        try {
            $BlackwellInstallDir = (Get-ItemProperty -Path $path1 -ErrorAction SilentlyContinue).$key1
        }
        catch {
            $BlackwellInstallDir = $null
        }

        if ($null -eq $BlackwellInstallDir) {
            try {
                $BlackwellInstallDir = (Get-ItemProperty -Path $path2 -ErrorAction SilentlyContinue).$key2
            }
            catch {
                $BlackwellInstallDir = $null
            }
        }

        if ($null -eq $BlackwellInstallDir) {
            Write-output "$(Get-Date -format u) - Couldn't find Blackwell in the registry. Upgrade will assume current path is correct" >> .\upgrade\upgrade.log
            $BlackwellInstallDir = (Get-Location).Path.TrimEnd('\')
        }

        return $BlackwellInstallDir
    }
}

# Check process status
function check-process {
    $process_id = (Get-Process blackwell-agent).id
    $counter = 10
    while($process_id -eq $null -And $counter -gt 0) {
        $counter--
        Start-Service -Name "Blackwell"
        Start-Sleep 2
        $process_id = (Get-Process blackwell-agent).id
    }
    write-output "$(Get-Date -format u) - Process ID: $($process_id)." >> .\upgrade\upgrade.log
}

# Check new version and restart the Blackwell service
function check-installation {

    $actual_version = (Get-Content VERSION)
    $counter = 5
    while($actual_version -eq $current_version -And $counter -gt 0) {
        write-output "$(Get-Date -format u) - Waiting for the Blackwell-Agent installation to end." >> .\upgrade\upgrade.log
        $counter--
        Start-Sleep 2
        $actual_version = (Get-Content VERSION)
    }
    write-output "$(Get-Date -format u) - Starting Blackwell-Agent service." >> .\upgrade\upgrade.log
    Start-Service -Name "Blackwell"
}

# Function to extract the version from the MSI using msiexec
function get_msi_version {
    $msiPath = (Get-Item ".\blackwell-agent*.msi").FullName
    write-output "$(Get-Date -format u) - Extracting the version from MSI file." >> .\upgrade\upgrade.log
    try {
        # Extracting the version using msiexec and waiting for it to complete
        Start-Process -FilePath "msiexec.exe" -ArgumentList "/a", "`"$msiPath`"", "/qn", "TARGETDIR=$env:TEMP", "/lv*", "`".\upgrade\msi_output.txt`"" -Wait

        $msi_version = Get-MSIProductVersion ".\upgrade\msi_output.txt"
        return $msi_version

    } catch {
        # Log any errors that occur during the process
        write-output "$(Get-Date -format u) - Couldn't extract MSI version. Error: $($_.Exception.Message)" >> .\upgrade\upgrade.log
        return $null
    }
}

function Get-MSIProductVersion {
    param (
        [string]$logFilePath
    )

    # Check if the log file exists
    if (-not (Test-Path $logFilePath)) {
        write-output "$(Get-Date -format u) - MSI log file not generated: $logFilePath" >> .\upgrade\upgrade.log
        return $null
    }

    try {
        # Get the line that contains "ProductVersion"
        $msi_version_info = Get-Content $logFilePath | Select-String "ProductVersion" | ForEach-Object { $_.Line }

        # Check if the version format is valid
        if (-not ($msi_version_info -match "ProductVersion\s*=\s*([0-9\.]+)")) {
            write-output "$(Get-Date -format u) - Invalid ProductVersion format in the MSI log: $logFilePath" >> .\upgrade\upgrade.log
            return $null
        }

        # Return the version with the 'v' prefix
        $product_version = "v$($matches[1])"
        return $product_version

    } catch {
        # Log any errors that occur
        write-output "$(Get-Date -format u) - Error extracting ProductVersion from MSI log: $($logFilePath). Error: $($_.Exception.Message)" >> .\upgrade\upgrade.log
        return $null
    }
}



# Stop UI and launch the MSI installer
function install {
    kill -processname win32ui -ErrorAction SilentlyContinue -Force
    Stop-Service -Name "Blackwell"
    Remove-Item .\upgrade\upgrade_result -ErrorAction SilentlyContinue
    write-output "$(Get-Date -format u) - Starting upgrade process." >> .\upgrade\upgrade.log

    try {
        $msiPath = (Get-Item ".\blackwell-agent*.msi").Name

        if ($msi_new_version -ne $null -and $msi_new_version -eq $current_version) {
            write-output "$(Get-Date -format u) - Reinstalling the same version." >> .\upgrade\upgrade.log
        }
        
        Start-Process -FilePath "msiexec.exe" -ArgumentList @("/i", $msiPath, '-quiet', '-norestart', '-log', 'installer.log') -Wait -NoNewWindow

    } catch {
        write-output "$(Get-Date -format u) - Installation failed: $($_.Exception.Message)" >> .\upgrade\upgrade.log
        return $false
    }

    return $true
}

# Check that the Blackwell installation runs on the expected path
$blackwellDir = get_blackwell_installation_directory
$normalizedBlackwellDir = $blackwellDir.TrimEnd('\')
$currentDir = (Get-Location).Path.TrimEnd('\')

if ($normalizedBlackwellDir -ne $currentDir) {
    Write-Output "$(Get-Date -format u) - Current working directory is not the Blackwell installation directory. Aborting." >> .\upgrade\upgrade.log
    Write-output "2" | out-file ".\upgrade\upgrade_result" -encoding ascii
    remove_upgrade_files
    exit 1
}

# Get current version
$current_version = (Get-Content VERSION)
write-output "$(Get-Date -format u) - Current version: $($current_version)." >> .\upgrade\upgrade.log

# Get new msi version
$msi_new_version = get_msi_version
if ($msi_new_version -ne $null) {
  write-output "$(Get-Date -format u) - MSI new version: $($msi_new_version)." >> .\upgrade\upgrade.log
} else {
  write-output "$(Get-Date -format u) - Could not find version in MSI file." >> .\upgrade\upgrade.log
}


# Ensure no other instance of msiexec is running by stopping them
Get-Process msiexec | Stop-Process -ErrorAction SilentlyContinue -Force

# Install
install
check-installation

write-output "$(Get-Date -format u) - Installation finished." >> .\upgrade\upgrade.log

check-process

# Wait for agent state to be cleaned
Start-Sleep 10

# Check status file
function Get-AgentStatus {
    Select-String -Path '.\blackwell-agent.state' -Pattern "^status='(.+)'" | %{$_.Matches[0].Groups[1].value}
}

$status = Get-AgentStatus
$counter = 30
while($status -ne "connected"  -And $counter -gt 0) {
    $counter--
    Start-Sleep 2
    $status = Get-AgentStatus
}
Write-Output "$(Get-Date -Format u) - Reading status file: status='$status'." >> .\upgrade\upgrade.log

if ($status -ne "connected") {
    write-output "$(Get-Date -format u) - Upgrade failed." >> .\upgrade\upgrade.log
    write-output "2" | out-file ".\upgrade\upgrade_result" -encoding ascii
}
else {
    write-output "0" | out-file ".\upgrade\upgrade_result" -encoding ascii
    write-output "$(Get-Date -format u) - Upgrade finished successfully." >> .\upgrade\upgrade.log
    $new_version = (Get-Content VERSION)
    write-output "$(Get-Date -format u) - New version: $($new_version)." >> .\upgrade\upgrade.log
}

remove_upgrade_files

exit 0
