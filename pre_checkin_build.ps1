#----------------------------------------------------------------------------
# Copyright (c) Microsoft Corp.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not 
# use this file except in compliance with the License.  You may obtain a copy 
# of the License at http://www.apache.org/licenses/LICENSE-2.0  
# 
# THIS CODE IS PROVIDED #AS IS# BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED 
# WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, 
# MERCHANTABLITY OR NON-INFRINGEMENT. 
#
# See the Apache Version 2.0 License for specific language governing 
# permissions and limitations under the License.
#---------------------------------------------------------------------------
# 
# C++ AMP standard algorithm library.
#
# This file contains the C++ AMP standard algorithms
#---------------------------------------------------------------------------

## Configure VS environment

function set-vsenv
{
    write-host "Setting up VS 2013 environment..."

    $script = "${env:ProgramFiles(x86)}\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"
    $tempFile = [IO.Path]::GetTempFileName()
 
    write-host "Creating setup script: $tempFile."
    write-host " Running $script..."
 
    cmd /c " `"$script`" && set > `"$tempFile`" " 

    Get-Content $tempFile | Foreach-Object {   
        if($_ -match "^(.*?)=(.*)$")  
        { 
            Set-Content "env:\$($matches[1])" $matches[2]  
        } 
    }
    Remove-Item $tempFile
}

## Tools paths and options

if (-not (test-path "${env:ProgramFiles(x86)}\Microsoft Visual Studio 12.0\VC"))
{
    write-host "`nVisual Studio 2013 not found at ${env:ProgramFiles(x86)}\Microsoft Visual Studio 12.0\VC" -fore red
    exit
}
if (-not (test-path "$env:VSINSTALLDIR"))
{
    set-vsenv
}

$stopwatch = New-Object System.Diagnostics.Stopwatch
$msbuild_exe = "$env:FrameworkDir$env:FrameworkVersion\msbuild.exe"
$msbuild_options = "/p:WARNINGS_AS_ERRORS=true /nologo /target:build /verbosity:m /filelogger /consoleloggerparameters:verbosity=m"

## Process arguments and configure builds

$run_options = "--gtest_color=yes --gtest_shuffle --gtest_filter=*:-*radix_sort*:*_direct3d_* --verbose "
if ($args -contains "/ref")
{
    $run_options += "--device ref"
    write-host "`nUnit tests will use REF accelerator." -fore yellow
}

$build_dir = split-path -parent $MyInvocation.MyCommand.Definition
$build_int = "$build_dir/Intermediate"
$build_bin = "$build_dir/Bin"

$vsvers = @( "12" )
$configs = @( "Debug", "Release" )
$platforms = @( "x64", "Win32" )

if ($args -contains "/test")
{
    $builds = @( ( New-Object 'Tuple[string, string, string]'("12", "Release", "Win32") ) )
    write-host "Test build: Building only Win32/Release." -fore yellow
}
else
{
    $builds = @()
    foreach ($ver in $vsvers)
    {
        foreach ($conf in $configs) 
        {
            foreach ($plat in $platforms)
            {
                $builds += New-Object 'Tuple[string, string, string]'($ver, $conf, $plat)
            }
        }
    }
}

## Run build

$stopwatch.Start()

## Clean tree...

write-host "== Clean         ===============================================================" -fore yellow

foreach ($p in @( $build_bin, $build_int ))
{
    if (test-path $p)
    { 
        Remove-Item -Recurse -Force $p
        write-host "  Cleaned:  $p"
    }
}

if ($args -contains "/clean") { exit }

## Build all targets...

write-host "`n== Build         ===============================================================" -fore yellow
write-host "Building the following configurations:" -fore yellow
foreach ($b in $builds)
{
    $ver = $b.Item1
    $conf = $b.Item2
    $plat = $b.Item3
    $sln = "amp_algorithms${ver}0.sln"

    write-host "  $sln ( $conf | $plat )"
}
$builds_expected = $builds.Count
$builds_ok = 0
$builds_run = 0

foreach  ($b in $builds)
{ 
    $ver = $b.Item1
    $conf = $b.Item2
    $plat = $b.Item3
    $sln = "amp_algorithms${ver}0.sln"
    $sln_log = "$build_int/v${ver}0_${plat}_${conf}_sln_build.log"

    foreach ($p in @( $build_int, $build_bin ))
    {
        if (-not (test-path $p)) { mkdir $p >> $null }
    }

    $builds_run ++
    write-host ("`n== Build {0:D2}\{1:D2} {2,-36} ============================" -f $builds_run, $builds_expected, "$sln $plat/$conf") -fore yellow
    $build_cmd = "$msbuild_exe $sln $msbuild_options /p:platformtoolset=v${ver}0 /p:VisualStudioVersion=${ver}.0 /p:platform=$plat /p:configuration=$conf /fileloggerparameters:logfile='$sln_log'"
    Invoke-Expression $build_cmd |
        foreach-object { if ( $_ -match "BUILD SUCCEEDED" ) { $builds_ok++; write-host $_ -fore green } else { write-host $_ } }
}

$stopwatch.Stop();
$elapsed = $stopwatch.Elapsed  
$BuildElapsedTime = [system.String]::Format("{0:00}m {1:00}s", $elapsed.Minutes, $elapsed.Seconds);

$builds_failed = ($builds_run - $builds_ok)
if ( $builds_failed -gt 0 )
{
    write-host "`n$builds_failed/$builds_run Builds FAILED!" -fore red
    $TestsElapsedTime = ""
}
else
{
    write-host "`n$builds_ok/$builds_run Builds completed." -fore green

    ## Run tests...

    write-host "`n== Run Tests     ===============================================================" -fore yellow

    $stopwatch.Reset();
    $stopwatch.Start();

    if ($args -contains "/ref")
    {
        write-host "Running tests with REF accelerator, this may take several minutes..." -fore yellow
    }

    foreach ($ver in $vsvers)
    {
        $test_exes = @( "amp_algorithms" )
        write-host "Running tests for Visual Studio ${ver}.0 ( $plat | $conf ) build..." -fore yellow

        foreach  ($t in $test_exes)
        {
            Invoke-Expression "$build_bin/v${ver}0/$plat/$conf/$t.exe $run_options" 
        }
    }
    $stopwatch.Stop();
    $elapsed = $stopwatch.Elapsed  
    $TestsElapsedTime = [System.String]::Format("{0:00}m {1:00}s", $elapsed.Minutes, $elapsed.Seconds);
}

write-host "`n================================================================================" -fore yellow
write-host "Build completed in: $BuildElapsedTime." -fore yellow
if ( $TestsElapsedTime -ne "" )
{
    write-host "Tests completed in: $TestsElapsedTime." -fore yellow
}
write-host
